/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fds_config.hpp>
#include <fds_process.h>
#include "NetSession.h"
#include <dlt.h>
#include <ObjectId.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <net/net_utils.h>
#include <fdsp/DMSvc.h>
#include <fdsp/SMSvc.h>
#include <am-platform.h>
#include <fdsp_utils.h>

#include "requests/requests.h"

#include "lib/StatsCollector.h"
#include "AmCache.h"
#include "AmDispatcher.h"
#include "AmProcessor.h"
#include "am-tx-mgr.h"
#include "StorHvCtrl.h"
#include "StorHvDataPlace.h"
#include "StorHvQosCtrl.h" 
#include "StorHvVolumes.h"

using namespace std;
using namespace FDS_ProtocolInterface;

StorHvCtrl *storHvisor;
std::atomic_uint nextIoReqId;

StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       SysParams *params,
                       sh_comm_modes _mode,
                       fds_uint32_t sm_port_num,
                       fds_uint32_t dm_port_num,
                       fds_uint32_t instanceId)
    : mode(_mode),
    counters_("AM", g_fdsprocess->get_cntrs_mgr().get()),
    om_client(nullptr)
{
    std::string  omIpStr;
    fds_uint32_t omConfigPort;
    std::string node_name = "localhost-sh";
    omConfigPort = 0;
    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());

    /*
     * Parse out cmdline options here.
     * TODO: We're parsing some options here and
     * some in ubd. We need to unify this.
     */
    if (mode == NORMAL) {
        omIpStr = config.get_abs<string>("fds.plat.om_ip");
        omConfigPort = config.get_abs<int>("fds.plat.om_port");
    }
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--om_ip=", 8) == 0) {
            if (mode == NORMAL) {
                /*
                 * Only use the OM's IP in the normal
                 * mode. We don't need it in test modes.
                 */
                omIpStr = argv[i] + 8;
            }
        } else if (strncmp(argv[i], "--om_port=", 10) == 0) {
            if (mode == NORMAL)
                omConfigPort = strtoul(argv[i] + 10, NULL, 0);
        }  else if (strncmp(argv[i], "--node_name=", 12) == 0) {
            node_name = argv[i] + 12;
        }
        /*
         * We don't complain here about other args because
         * they may have been processed already but not
         * removed from argc/argv
         */
    }

    my_node_name = node_name;

    sysParams = params;

    disableVcc =  config.get_abs<bool>("fds.am.testing.disable_vcc");

    LOGNORMAL << "StorHvCtrl - Constructing the Storage Hvisor";

    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;

    rpcSessionTbl = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_HVISOR));

    LOGNOTIFY << "StorHvCtrl - My IP: " << net::get_local_ip(config.get_abs<std::string>("fds.nic_if"));

    /*  Create the QOS Controller object */
    fds_uint32_t qos_threads = config.get<int>("qos_threads");
    qos_ctrl = new StorHvQosCtrl(qos_threads,
				 fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET,
                                 GetLog());

    // Check the AM standalone toggle
    toggleStandAlone = config.get_abs<bool>("fds.am.testing.toggleStandAlone");
    if (toggleStandAlone) {
        LOGWARN << "Starting SH CTRL in stand alone mode";
    }

    /* create OMgr client if in normal mode */
    if (!toggleStandAlone) {
        LOGNORMAL << "StorHvCtrl - Will create and initialize OMgrClient";

        /*
         * Pass 0 as the data path port since the SH is not
         * listening on that port.
         */
        om_client = new OMgrClient(FDSP_STOR_HVISOR,
                                   omIpStr,
                                   omConfigPort,
                                   node_name,
                                   GetLog(),
                                   rpcSessionTbl,
                                   &gl_AmPlatform,
                                   instanceId);
        om_client->initialize();

        qos_ctrl->registerOmClient(om_client); /* so it will start periodically pushing perfstats to OM */
        om_client->startAcceptingControlMessages();

        StatsCollector::singleton()->registerOmClient(om_client);
        fds_bool_t print_qos_stats = config.get_abs<bool>("fds.am.testing.print_qos_stats");
        fds_bool_t disableStreamingStats = config.get_abs<bool>("fds.am.testing.toggleDisableStreamingStats");
        if (print_qos_stats) {
            StatsCollector::singleton()->enableQosStats("AM");
        }
        if (!disableStreamingStats) {
            StatsCollector::singleton()->startStreaming(NULL, NULL);
        }
    }

    DMTManagerPtr dmtMgr;
    DLTManagerPtr dltMgr;
    if (toggleStandAlone) {
        dmtMgr = boost::make_shared<DMTManager>(1);
        dltMgr = boost::make_shared<DLTManager>();
    } else {
        dmtMgr = om_client->getDmtManager();
        dltMgr = om_client->getDltManager();
    }

    /* TODO: for now StorHvVolumeTable constructor will create
     * volume 1, revisit this soon when we add multi-volume support
     * in other parts of the system */
    vol_table = new StorHvVolumeTable(this, GetLog());

    // Init rand num generator
    // TODO(Andrew): Move this to platform process so everyone gets it
    // and make AM extend from platform process
    randNumGen = RandNumGenerator::ptr(new RandNumGenerator(RandNumGenerator::getRandSeed()));

    // Init the AM transaction manager
    amTxMgr = std::make_shared<AmTxManager>("AM Transaction Manager Module");
    // Init the AM cache manager
    amCache = std::make_shared<AmCache>("AM Cache Manager Module");

    // Init the dispatcher layer
    // TODO(Andrew): Decide if AM or AmProcessor should own
    // this layer.
    amDispatcher = AmDispatcher::shared_ptr(
        new AmDispatcher("AM Dispatcher Module",
                         dltMgr,
                         dmtMgr));
    // Init the processor layer
    amProcessor = AmProcessor::unique_ptr(
        new AmProcessor("AM Processor Module",
                        amDispatcher,
                        qos_ctrl,
                        vol_table,
                        amTxMgr,
                        amCache));

    LOGNORMAL << "StorHvCtrl - StorHvCtrl basic infra init successfull ";

    if (toggleStandAlone) {
        dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NORMAL_MODE,
                                                         NULL);
    } else {
        LOGNORMAL <<"StorHvCtrl -  Entring Normal Data placement mode";
        dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NORMAL_MODE,
                                                         om_client);
    }
}

/*
 * Constructor uses comm with DM and SM if no mode provided.
 */
StorHvCtrl::StorHvCtrl(int argc, char *argv[], SysParams *params)
        : StorHvCtrl(argc, argv, params, NORMAL, 0, 0) {
}

StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       SysParams *params,
                       sh_comm_modes _mode,
                       fds_uint32_t instanceId)
        : StorHvCtrl(argc, argv, params, _mode, 0, 0, instanceId) {
}

StorHvCtrl::~StorHvCtrl()
{
    delete vol_table;
    delete dataPlacementTbl;
    if (om_client)
        delete om_client;
    delete qos_ctrl;
}

SysParams* StorHvCtrl::getSysParams() {
    return sysParams;
}

void StorHvCtrl::StartOmClient() {
    /*
     * Start listening for OM control messages
     * Appropriate callbacks were setup by data placement and volume table objects
     */
    if (om_client) {
        LOGNOTIFY << "StorHvCtrl - Started accepting control messages from OM";
        om_client->registerNodeWithOM(&gl_AmPlatform);
    }
}

Error StorHvCtrl::sendTestBucketToOM(const std::string& bucket_name,
                                        const std::string& access_key_id,
                                        const std::string& secret_access_key) {
    Error err(ERR_OK);
    int om_err = 0;
    LOGNORMAL << "bucket: " << bucket_name;

    // send test bucket message to OM
    FDSP_VolumeInfoTypePtr vol_info(new FDSP_VolumeInfoType());
    initVolInfo(vol_info, bucket_name);
    om_err = om_client->testBucket(bucket_name,
                                               vol_info,
                                               true,
                                               access_key_id,
                                               secret_access_key);
    if (om_err != 0) {
        err = Error(ERR_INVALID_ARG);
    }
    return err;
}

void StorHvCtrl::initVolInfo(FDSP_VolumeInfoTypePtr vol_info,
                             const std::string& bucket_name) {
    vol_info->vol_name = std::string(bucket_name);
    vol_info->tennantId = 0;
    vol_info->localDomainId = 0;
    vol_info->globDomainId = 0;

    // Volume capacity is in MB
    vol_info->capacity = (1024*10);  // for now presetting to 10GB
    vol_info->maxQuota = 0;
    vol_info->volType = FDSP_VOL_S3_TYPE;

    vol_info->defReplicaCnt = 0;
    vol_info->defWriteQuorum = 0;
    vol_info->defReadQuorum = 0;
    vol_info->defConsisProtocol = FDSP_CONS_PROTO_STRONG;

    vol_info->volPolicyId = 50;  // default S3 policy desc ID
    vol_info->archivePolicyId = 0;
    vol_info->placementPolicy = 0;
    vol_info->appWorkload = FDSP_APP_WKLD_TRANSACTION;
    vol_info->mediaPolicy = FDSP_MEDIA_POLICY_HDD;
}

/**
 * Function called when volume waiting queue is drained.
 * When it's called a volume has just been attached and
 * we can call the callback to tell any waiters that the
 * volume is now ready.
 */
void
StorHvCtrl::attachVolume(AmRequest *amReq) {
    // Get request from qos object
    fds_verify(amReq != NULL);
    AttachVolBlobReq *blobReq = static_cast<AttachVolBlobReq *>(amReq);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_ATTACH_VOL);

    LOGDEBUG << "Attach for volume " << blobReq->volume_name
             << " complete. Notifying waiters";
    blobReq->cb->call(ERR_OK);
}

void
StorHvCtrl::enqueueAttachReq(const std::string& volumeName,
                             CallbackPtr cb) {
    LOGDEBUG << "Attach request for volume " << volumeName;

    // check if volume is already attached
    fds_volid_t volId = invalid_vol_id;
    if (invalid_vol_id != (volId = vol_table->getVolumeUUID(volumeName))) {
        LOGDEBUG << "Volume " << volumeName
                 << " with UUID " << volId
                 << " already attached";
        cb->call(ERR_OK);
        return;
    }

    // Create a noop request to put into wait queue
    AttachVolBlobReq *blobReq = new AttachVolBlobReq(volId, volumeName, cb);

    // Enqueue this request to process the callback
    // when the attach is complete
    vol_table->addBlobToWaitQueue(volumeName, blobReq);

    fds_verify(sendTestBucketToOM(volumeName,
                                  "",  // The access key isn't used
                                  "") == ERR_OK); // The secret key isn't used
}

Error
StorHvCtrl::pushBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);
    Error err(ERR_OK);

    PerfTracer::tracePointBegin(blobReq->e2e_req_perf_ctx); 
    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx); 

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
    fds_volid_t volId = blobReq->io_vol_id;

    StorHvVolume *shVol = vol_table->getLockedVolume(volId);
    if ((shVol == NULL) || (shVol->volQueue == NULL)) {
        if (shVol)
            shVol->readUnlock();
        LOGERROR << "Volume and queueus are NOT setup for volume " << volId;
        err = ERR_INVALID_ARG;
        PerfTracer::tracePointEnd(blobReq->qos_perf_ctx); 
        delete blobReq;
        return err;
    }
    /*
     * TODO: We should handle some sort of success/failure here?
     */
    qos_ctrl->enqueueIO(volId, blobReq);
    shVol->readUnlock();

    LOGDEBUG << "Queued IO for vol " << volId;

    return err;
}

void
StorHvCtrl::enqueueBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);

    // check if volume is attached to this AM
    if (invalid_vol_id == (blobReq->io_vol_id = vol_table->getVolumeUUID(blobReq->volume_name))) {
        vol_table->addBlobToWaitQueue(blobReq->volume_name, blobReq);
        fds_verify(sendTestBucketToOM(blobReq->volume_name,
                                      "",  // The access key isn't used
                                      "") == ERR_OK); // The secret key isn't used
        return;
    }

    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx);

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);

    fds_verify(qos_ctrl->enqueueIO(blobReq->io_vol_id, blobReq) == ERR_OK);
}

void
processBlobReq(AmRequest *amReq) {
    fds::PerfTracer::tracePointEnd(amReq->qos_perf_ctx);

    fds_verify(amReq->io_module == FDS_IOType::STOR_HV_IO);
    fds_verify(amReq->magicInUse() == true);

    switch (amReq->io_type) {
        case fds::FDS_START_BLOB_TX:
            storHvisor->amProcessor->startBlobTx(amReq);
            break;

        case fds::FDS_COMMIT_BLOB_TX:
            storHvisor->amProcessor->commitBlobTx(amReq);
            break;

        case fds::FDS_ABORT_BLOB_TX:
            storHvisor->amProcessor->abortBlobTx(amReq);
            break;

        case fds::FDS_ATTACH_VOL:
            storHvisor->attachVolume(amReq);
            break;

        case fds::FDS_IO_READ:
        case fds::FDS_GET_BLOB:
            storHvisor->amProcessor->getBlob(amReq);
            break;

        case fds::FDS_IO_WRITE:
        case fds::FDS_PUT_BLOB_ONCE:
        case fds::FDS_PUT_BLOB:
            storHvisor->amProcessor->putBlob(amReq);
            break;

        case fds::FDS_SET_BLOB_METADATA:
            storHvisor->amProcessor->setBlobMetadata(amReq);
            break;

        case fds::FDS_GET_VOLUME_METADATA:
            storHvisor->amProcessor->getVolumeMetadata(amReq);
            break;

        case fds::FDS_DELETE_BLOB:
            storHvisor->amProcessor->deleteBlob(amReq);
            break;

        case fds::FDS_STAT_BLOB:
            storHvisor->amProcessor->statBlob(amReq);
            break;

        case fds::FDS_VOLUME_CONTENTS:
            storHvisor->amProcessor->volumeContents(amReq);
            break;

        default :
            LOGCRITICAL << "unimplemented request: " << amReq->io_type;
            amReq->cb->call(ERR_NOT_IMPLEMENTED);
            break;
    }
}
