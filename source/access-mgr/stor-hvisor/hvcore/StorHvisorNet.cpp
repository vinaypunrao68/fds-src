/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include <hash/MurmurHash3.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetSession.h"
#include <net/net_utils.h>
#include <am-platform.h>


StorHvCtrl *storHvisor;

using namespace std;
using namespace FDS_ProtocolInterface;

void CreateSHMode(int argc,
                  char *argv[],
                  fds_bool_t test_mode,
                  fds_uint32_t sm_port,
                  fds_uint32_t dm_port)
{

    fds::Module *io_dm_vec[] = {
        nullptr
    };

    fds::ModuleVector  io_dm(argc, argv, io_dm_vec);

    if (test_mode == true) {
        storHvisor = new StorHvCtrl(argc, argv, io_dm.get_sys_params(),
                                    StorHvCtrl::TEST_BOTH, sm_port, dm_port);
    } else {
        storHvisor = new StorHvCtrl(argc, argv, io_dm.get_sys_params(),
                                    StorHvCtrl::NORMAL);
    }

    /*
     * Start listening for OM control messages
     * Appropriate callbacks were setup by data placement and volume table objects
     */
    storHvisor->StartOmClient();
    storHvisor->qos_ctrl->runScheduler();

    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << "StorHvisorNet - Created storHvisor " << storHvisor;
}

void DeleteStorHvisor()
{
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << " StorHvisorNet -  Deleting the StorHvisor";
    delete storHvisor;
}

void ctrlCCallbackHandler(int signal)
{
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << "StorHvisorNet -  Received Ctrl C " << signal;
    // SAN   storHvisor->_communicator->shutdown();
    DeleteStorHvisor();
}


StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       SysParams *params,
                       sh_comm_modes _mode,
                       fds_uint32_t sm_port_num,
                       fds_uint32_t dm_port_num)
    : mode(_mode),
    counters_("AM", g_fdsprocess->get_cntrs_mgr().get())
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

    initHandlers();
    my_node_name = node_name;

    sysParams = params;

    disableVcc =  config.get_abs<bool>("fds.am.testing.disable_vcc");

    LOGNORMAL << "StorHvisorNet - Constructing the Storage Hvisor";

    /* create OMgr client if in normal mode */
    om_client = NULL;
    LOGNORMAL << "StorHvisorNet - Will create and initialize OMgrClient";

    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;

    rpcSessionTbl = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_HVISOR));

    myIp = net::get_local_ip(config.get_abs<std::string>("fds.nic_if"));
    assert(myIp.empty() == false);
    LOGNOTIFY << "StorHvisorNet - My IP: " << myIp;

    dPathRespCback.reset(new FDSP_DataPathRespCbackI());
    mPathRespCback.reset(new FDSP_MetaDataPathRespCbackI());
    /*
     * Pass 0 as the data path port since the SH is not
     * listening on that port.
     */
    cout << " om config port : " << omConfigPort << ", om IP " << omIpStr << "\n";
    om_client = new OMgrClient(FDSP_STOR_HVISOR,
                               omIpStr,
                               omConfigPort,
                               node_name,
                               GetLog(),
                               rpcSessionTbl,
                               &gl_AmPlatform);
    if (om_client) {
        om_client->initialize();
    }
    else {
        LOGERROR << "StorHvisorNet - Failed to create OMgrClient, will not receive any OM events";
    }


    /* register handlers for receiving responses to admin requests */
    om_client->registerBucketStatsCmdHandler(bucketStatsRespHandler);

    /*  Create the QOS Controller object */
    fds_uint32_t qos_threads = config.get<int>("qos_threads");
    qos_ctrl = new StorHvQosCtrl(qos_threads,
				 fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET, GetLog());
    qos_ctrl->registerOmClient(om_client); /* so it will start periodically pushing perfstats to OM */
    om_client->startAcceptingControlMessages();

    /* TODO: for now StorHvVolumeTable constructor will create
     * volume 1, revisit this soon when we add multi-volume support
     * in other parts of the system */
    vol_table = new StorHvVolumeTable(this, GetLog());

    // Init rand num generator
    // TODO(Andrew): Move this to platform process so everyone gets it
    // and make AM extend from platform process
    randNumGen = RandNumGenerator::ptr(new RandNumGenerator(RandNumGenerator::getRandSeed()));

    // Check the AM processing path toggle
    toggleNewPath = config.get_abs<bool>("fds.am.testing.toggleNewPath");
    // Init the AM transaction manager
    amTxMgr = AmTxManager::shared_ptr(new AmTxManager("AM Transaction Manager Module"));
    // Init the AM cache manager
    amCache = AmCache::shared_ptr(new AmCache("AM Cache Manager Module"));

    // Init the dispatcher layer
    // TODO(Andrew): Decide if AM or AmProcessor should own
    // this layer.
    amDispatcher = AmDispatcher::shared_ptr(
        new AmDispatcher("AM Dispatcher Module",
                         om_client->getDltManager(),
                         om_client->getDmtManager()));
    // Init the processor layer
    amProcessor = AmProcessor::unique_ptr(
        new AmProcessor("AM Processor Module",
                        amDispatcher,
                        qos_ctrl,
                        vol_table,
                        amTxMgr,
                        amCache));

    chksumPtr =  new checksum_calc();

    /*
     * Set basic thread properties.
     */

    LOGNORMAL << "StorHvisorNet - StorHvCtrl basic infra init successfull ";

    StatsCollector::singleton()->registerOmClient(om_client);
    fds_bool_t print_qos_stats = config.get_abs<bool>("fds.am.testing.print_qos_stats");
    if (print_qos_stats) {
        StatsCollector::singleton()->enableQosStats("AM");
    }
    StatsCollector::singleton()->startStreaming(NULL, NULL);

    /*
     * Parse options out of config file
     */

    /*
     * Setup RPC endpoints based on comm mode.
     */
    std::string dataMgrIPAddress;
    int dataMgrPortNum;
    std::string storMgrIPAddress;
    int storMgrPortNum;
    if ((mode == DATA_MGR_TEST) ||
        (mode == TEST_BOTH)) {
        /*
         * If a port_num to use is set use it,
         * otherwise pull from config file.
         */
        if (dm_port_num != 0) {
            dataMgrPortNum = dm_port_num;
        } else {
            dataMgrPortNum = config.get<int>("fds.dm.PortNumber");
        }
        dataMgrIPAddress = config.get<string>("fds.dm.IPAddress");
        storHvisor->rpcSessionTbl->
                startSession<netMetaDataPathClientSession>(dataMgrIPAddress,
                                                           (fds_int32_t)dataMgrPortNum,
                                                           FDS_ProtocolInterface::FDSP_DATA_MGR,0,storHvisor->mPathRespCback);
    }
    if ((mode == STOR_MGR_TEST) ||
        (mode == TEST_BOTH)) {
        if (sm_port_num != 0) {
            storMgrPortNum = sm_port_num;
        } else {
            storMgrPortNum  = config.get<int>("fds.sm.PortNumber");
        }
        storMgrIPAddress  = config.get<string>("fds.sm.IPAddress");
        storHvisor->rpcSessionTbl->
                startSession<netDataPathClientSession>(storMgrIPAddress,
                                                       (fds_int32_t)storMgrPortNum,
                                                       FDS_ProtocolInterface::FDSP_STOR_MGR,0,storHvisor->dPathRespCback);
    }


    if ((mode == DATA_MGR_TEST) ||
        (mode == TEST_BOTH)) {
        /*
         * TODO: Currently we always add the DM IP in the DM and BOTH test modes.
         */
        fds_uint32_t ip_num  = rpcSessionTbl->ipString2Addr(dataMgrIPAddress);
        dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NO_OM_MODE,
                                                    ip_num,
                                                    storMgrPortNum,
                                                    dataMgrPortNum,
                                                    om_client);
    } else if (mode == STOR_MGR_TEST) {
        fds_uint32_t ip_num = rpcSessionTbl->ipString2Addr(storMgrIPAddress);
        dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NO_OM_MODE,
                                                    ip_num,
                                                    storMgrPortNum,
                                                    dataMgrPortNum,
                                                    om_client);
    } else {
        LOGNORMAL <<"StorHvisorNet -  Entring Normal Data placement mode";
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
                       sh_comm_modes _mode)
        : StorHvCtrl(argc, argv, params, _mode, 0, 0) {
}

StorHvCtrl::~StorHvCtrl()
{
    delete vol_table;
    delete dataPlacementTbl;
    if (om_client)
        delete om_client;
    delete qos_ctrl;
}

void StorHvCtrl::initHandlers() {
    handlers[fds::FDS_STAT_BLOB]            = new StatBlobHandler(this);
    handlers[fds::FDS_GET_VOLUME_METADATA]  = new GetVolumeMetaDataHandler(this);
    handlers[fds::FDS_LIST_BUCKET]          = new GetBucketHandler(this);
    handlers[fds::FDS_DELETE_BLOB]          = new DeleteBlobHandler(this);
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
        LOGNOTIFY << "StorHvisorNet - Started accepting control messages from OM";
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
