/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fds_config.hpp>
#include <fds_process.h>
#include <dlt.h>
#include <ObjectId.h>
#include <net/SvcRequestPool.h>
#include <net/net_utils.h>
#include <fdsp/DMSvc.h>
#include <fdsp/SMSvc.h>
#include <fdsp_utils.h>

#include "requests/requests.h"

#include "lib/StatsCollector.h"
#include "lib/OMgrClient.h"
#include "AccessMgr.h"
#include "AmProcessor.h"
#include "StorHvCtrl.h"
#include "StorHvQosCtrl.h"

using namespace std;
using namespace FDS_ProtocolInterface;

StorHvCtrl *storHvisor;
std::atomic_uint nextIoReqId;

StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       SysParams *params,
                       sh_comm_modes _mode,
                       fds_uint32_t sm_port_num,
                       fds_uint32_t dm_port_num)
    : mode(_mode),
    counters_("AM", g_fdsprocess->get_cntrs_mgr().get()),
    om_client(nullptr)
{
    std::string node_name = "localhost-sh";
    FdsConfigAccessor config(g_fdsprocess->get_conf_helper());

    sysParams = params;

    disableVcc =  config.get_abs<bool>("fds.am.testing.disable_vcc");

    LOGNORMAL << "StorHvCtrl - Constructing the Storage Hvisor";

    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;

    LOGNOTIFY << "StorHvCtrl - My IP: " << net::get_local_ip(config.get_abs<std::string>("fds.nic_if"));

    /*  Create the QOS Controller object */
    fds_uint32_t qos_threads = config.get<int>("qos_threads");
    qos_ctrl = std::make_shared<StorHvQosCtrl>(qos_threads,
                                               fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET,
                                               GetLog());

    // Check the AM standalone toggle
    auto standalone = config.get_abs<bool>("fds.am.testing.standalone");
    if (standalone) {
        LOGWARN << "Starting SH CTRL in stand alone mode";
    }

    /*
     * Pass 0 as the data path port since the SH is not
     * listening on that port.
     */
    om_client = new OMgrClient(FDSP_ACCESS_MGR,
			       node_name,
			       GetLog());
    if (standalone) {
	om_client->setNoNetwork(true);
    } else {
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
    if (standalone) {
        dmtMgr = boost::make_shared<DMTManager>(1);
        dltMgr = boost::make_shared<DLTManager>();
    } else {
        dmtMgr = om_client->getDmtManager();
        dltMgr = om_client->getDltManager();
    }

    // Init rand num generator
    // TODO(Andrew): Move this to platform process so everyone gets it
    // and make AM extend from platform process
    randNumGen = RandNumGenerator::ptr(new RandNumGenerator(RandNumGenerator::getRandSeed()));

    // Init the processor layer
    amProcessor = AmProcessor::unique_ptr(
        new AmProcessor("AM Processor Module", qos_ctrl, dltMgr, dmtMgr));

    LOGNORMAL << "StorHvCtrl - StorHvCtrl basic infra init successfull ";
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
                       sh_comm_modes _mode)
        : StorHvCtrl(argc, argv, params, _mode, 0, 0) {
}

StorHvCtrl::~StorHvCtrl()
{
    if (om_client)
        delete om_client;
}

SysParams* StorHvCtrl::getSysParams() {
    return sysParams;
}

void StorHvCtrl::StartOmClient() {
    // Call the dispatcher startup function here since this
    // legacy class doesn't actually extend from Module but
    // needs a member's startup to be called.
    amProcessor->mod_startup();
}

Error StorHvCtrl::sendTestBucketToOM(const std::string& bucket_name,
                                        const std::string& access_key_id,
                                        const std::string& secret_access_key) {
    Error err(ERR_OK);
    int om_err = 0;

    LOGNORMAL << "bucket: " << bucket_name;

    // send test bucket message to OM
    om_err = om_client->testBucket(bucket_name,
                                   true,
                                   access_key_id,
                                   secret_access_key);
    if (om_err != 0) {
        err = Error(ERR_INVALID_ARG);
    }
    return err;
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
    if (am->isShuttingDown()) {
        cb->call(ERR_SHUTTING_DOWN);
        return;
    }

    // check if volume is already attached
    fds_volid_t volId = invalid_vol_id;
    if (invalid_vol_id != (volId = amProcessor->getVolumeUUID(volumeName))) {
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
    addBlobToWaitQueue(volumeName, blobReq);

    fds_verify(sendTestBucketToOM(volumeName,
                                  "",  // The access key isn't used
                                  "") == ERR_OK); // The secret key isn't used
}

void
StorHvCtrl::enqueueBlobReq(AmRequest *blobReq) {
    if (am->isShuttingDown()) {
        blobReq->cb->call(ERR_SHUTTING_DOWN);
        return;
    }
    fds_verify(blobReq->magicInUse() == true);

    // check if volume is attached to this AM
    if (invalid_vol_id == (blobReq->io_vol_id = amProcessor->getVolumeUUID(blobReq->volume_name))) {
        addBlobToWaitQueue(blobReq->volume_name, blobReq);
        fds_verify(sendTestBucketToOM(blobReq->volume_name,
                                      "",  // The access key isn't used
                                      "") == ERR_OK); // The secret key isn't used
        return;
    }

    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx);

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);

    fds_verify(qos_ctrl->enqueueIO(blobReq->io_vol_id, blobReq) == ERR_OK);
}

/*
 * Add blob request to wait queue because a blob is waiting for OM
 * to attach bucjets to AM; once vol table receives vol attach event,
 * it will move all requests waiting in the queue for that bucket
 * to appropriate qos queue
 */
void StorHvCtrl::addBlobToWaitQueue(const std::string& bucket_name,
                                    AmRequest* blob_req)
{
    fds_verify(blob_req->magicInUse() == true);
    LOGDEBUG << "VolumeTable -- adding blob to wait queue, waiting for "
        << "bucket " << bucket_name;

    /*
     * Pack to qos req
     * TODO: We're hard coding 885 as the request ID to
     * pass to QoS. SHCtrl has a request counter but
     * we can't see if from here...Let's fix that eventually.
     */
    blob_req->io_req_id = 885;

    wait_rwlock.write_lock();
    wait_blobs_it_t it = wait_blobs.find(bucket_name);
    if (it != wait_blobs.end()) {
        /* we already have vector of waiting blobs for this bucket name */
        (it->second).push_back(blob_req);
    } else {
        /* we need to start a new vector */
        wait_blobs[bucket_name].push_back(blob_req);
    }
    wait_rwlock.write_unlock();

    // If the volume got attached in the meantime, we need
    // to move the requests ourselves.
    fds_volid_t volid = amProcessor->getVolumeUUID(bucket_name);
    if (volid != invalid_vol_id) {
        LOGDEBUG << " volid " << std::hex << volid << std::dec
            << " bucket " << bucket_name << " got attached";
        moveWaitBlobsToQosQueue(volid,
                                bucket_name,
                                ERR_OK);
    }
}

void StorHvCtrl::moveWaitBlobsToQosQueue(fds_volid_t vol_uuid,
                                         const std::string& vol_name,
                                         Error error)
{
    Error err = error;
    bucket_wait_vec_t blobs;

    wait_rwlock.read_lock();
    wait_blobs_it_t it = wait_blobs.find(vol_name);
    if (it != wait_blobs.end()) {
        /* we have a wait queue of blobs for this volume name */
        blobs.swap(it->second);
        wait_blobs.erase(it);
    }
    wait_rwlock.read_unlock();

    if (blobs.size() == 0) {
        LOGDEBUG << "VolumeTable::moveWaitBlobsToQueue -- "
            << "no blobs waiting for bucket " << vol_name;
        return; // no blobs waiting
    }

    if ( err.ok() )
    {
        auto shVol = amProcessor->getVolume(vol_uuid);
        if (!shVol) {
            LOGERROR << "Volume and  Queueus are NOT setup :" << vol_uuid;
            err = ERR_INVALID_ARG;
        }
        if (err.ok()) {
            for (uint i = 0; i < blobs.size(); ++i) {
                LOGDEBUG << "VolumeTable - moving blob to qos queue of vol  " << std::hex
                    << vol_uuid << std::dec << " vol_name " << vol_name;
                // since we did not know the volume id before, volid for this io is set to 0
                // so set its volume id now to actual volume id
                AmRequest* req = blobs[i];
                fds_verify(req != NULL);
                req->io_vol_id = vol_uuid;

                fds::PerfTracer::tracePointBegin(req->qos_perf_ctx);
                storHvisor->qos_ctrl->enqueueIO(vol_uuid, req);
            }
            blobs.clear();
        }
    }

    if ( !err.ok()) {
        /* we haven't pushed requests to qos queue either because we already
         * got 'error' parameter with !err.ok() or we couldn't find volume
         * so complete blobs in 'blobs' vector with error (if there are any) */
        for (uint i = 0; i < blobs.size(); ++i) {
            AmRequest* blobReq = blobs[i];
            blobs[i] = nullptr;
            // Hard coding a result!
            LOGERROR << "some issue : " << err;
            LOGWARN << "Calling back with error since request is waiting"
                << " for volume " << blobReq->io_vol_id
                << " that doesn't exist";
            blobReq->cb->call(FDSN_StatusEntityDoesNotExist);
            delete blobReq;
        }
        blobs.clear();
    }
}



void
processBlobReq(AmRequest *amReq) {
    fds::PerfTracer::tracePointEnd(amReq->qos_perf_ctx);

    fds_verify(amReq->io_module == FDS_IOType::STOR_HV_IO);
    fds_verify(amReq->magicInUse() == true);

    /*
     * Drain the queue if we are shutting down.
     */
    if (am->isShuttingDown()) {
        Error err(ERR_SHUTTING_DOWN);
        amReq->cb->call(err);
        return;
    }

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

        case fds::FDS_STAT_VOLUME:
            storHvisor->amProcessor->statVolume(amReq);
            break;

        case fds::FDS_SET_VOLUME_METADATA:
            storHvisor->amProcessor->setVolumeMetadata(amReq);
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
