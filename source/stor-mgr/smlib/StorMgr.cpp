/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <map>
#include <string>
#include <set>
#include <list>
#include <iostream>
#include <thread>
#include <functional>

#include <PerfTrace.h>
#include <ObjMeta.h>
#include <SmObjDb.h>
#include <policy_rpc.h>
#include <policy_tier.h>
#include <StorMgr.h>
#include <NetSession.h>
#include <fds_obj_cache.h>
#include <fds_timestamp.h>
#include <fdsp_utils.h>
#include <net/net_utils.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>

using diskio::DataTier;

namespace fds {

fds_bool_t  stor_mgr_stopping = false;
fds_bool_t  fds_data_verify = true;

#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))

ObjectStorMgr *objStorMgr;

/*
 * SM's FDSP interface member functions
 */
ObjectStorMgrI::ObjectStorMgrI()
{
}

ObjectStorMgrI::~ObjectStorMgrI()
{
}

void
ObjectStorMgrI::PutObject(FDSP_MsgHdrTypePtr& msgHdr,
                          FDSP_PutObjTypePtr& putObj) {
    // This should never be called
    fds_assert(1 == 0);
}

void
ObjectStorMgrI::GetObject(FDSP_MsgHdrTypePtr& msgHdr,
                          FDSP_GetObjTypePtr& getObj)
{
    // This should never be called
    fds_assert(1 == 0);
}

void
ObjectStorMgrI::DeleteObject(FDSP_MsgHdrTypePtr& msgHdr,
                             FDSP_DeleteObjTypePtr& delObj)
{
    // This should never be called
    fds_assert(1 == 0);
}

void
ObjectStorMgrI::OffsetWriteObject(FDSP_MsgHdrTypePtr& msg_hdr,
                                  FDSP_OffsetWriteObjTypePtr& offset_write_obj) {
    // This should never be called
    fds_assert(1 == 0);
}

void
ObjectStorMgrI::RedirReadObject(FDSP_MsgHdrTypePtr &msg_hdr,
                                FDSP_RedirReadObjTypePtr& redir_read_obj) {
    // This should never be called
    fds_assert(1 == 0);
}

void ObjectStorMgrI::GetObjectMetadata(
        boost::shared_ptr<FDSP_GetObjMetadataReq>& metadata_req) {  // NOLINT
    // This should never be called
    fds_assert(1 == 0);
}
void ObjectStorMgrI::GetObjectMetadataCb(const Error &err,
        SmIoReadObjectMetadata *read_data)
{
    // This should never be called
    fds_assert(1 == 0);
}

void ObjectStorMgrI::GetTokenMigrationStats(FDSP_TokenMigrationStats& _return,
            boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)
{
    // This should never be called
    fds_assert(1 == 0);
}

/**
 * Storage manager member functions
 * 
 * TODO: The number of test vols, the
 * totalRate, and number of qos threads
 * are being hard coded in the initializer
 * list below.
 */
ObjectStorMgr::ObjectStorMgr(CommonModuleProviderIf *modProvider)
    : Module("sm"),
      modProvider_(modProvider),
      totalRate(6000),  // will be over-written using node capability
      qosThrds(100),  // will be over-written from config
      qosOutNum(10)
{
    // NOTE: Don't put much stuff in the constuctor.  Move any construction
    // into mod_init()
}

void ObjectStorMgr::setModProvider(CommonModuleProviderIf *modProvider) {
    modProvider_ = modProvider;
}

ObjectStorMgr::~ObjectStorMgr() {
    LOGDEBUG << " Destructing  the Storage  manager";
    shuttingDown = true;

    delete smObjDb;
    /*
     * Clean up the QoS system. Need to wait for I/Os to
     * complete and deregister each volume. The volume info
     * is freed when the table is deleted.
     * TODO: We should prevent further volume registration and
     * accepting network I/Os while shutting down.
     */
    if (volTbl) {
        std::list<fds_volid_t> volIds = volTbl->getVolList();
        for (std::list<fds_volid_t>::iterator vit = volIds.begin();
             vit != volIds.end();
             vit++) {
            qosCtrl->quieseceIOs((*vit));
            qosCtrl->deregisterVolume((*vit));
        }
    }
    // delete perfStats;

    /*
     * TODO: Assert that the waiting req map is empty.
     */

    delete qosCtrl;

    delete writeBackThreads;
    delete dirtyFlashObjs;
    delete tierEngine;
    delete rankEngine;

    delete volTbl;
    delete objStorMutex;
    delete omJrnl;
}

int
ObjectStorMgr::mod_init(SysParams const *const param) {
    shuttingDown = false;
    numWBThreads = 1;
    maxDirtyObjs = 10000;

    // Init  the log infra
    GetLog()->setSeverityFilter(fds_log::getLevelFromName(
        modProvider_->get_fds_config()->get<std::string>("fds.sm.log_severity")));

    LOGDEBUG << "Constructing the Object Storage Manager";
    objStorMutex = new fds_mutex("Object Store Mutex");

    /*
     * Will setup OM comm during run()
     */
    omClient = NULL;

    return 0;
}

void ObjectStorMgr::mod_startup()
{
    // todo: clean up the code below.  It's doing too many things here.
    // Refactor into functions or make it part of module vector

    std::string     myIp;

    modProvider_->proc_fdsroot()->\
        fds_mkdir(modProvider_->proc_fdsroot()->dir_user_repo_objs().c_str());
    std::string obj_dir = modProvider_->proc_fdsroot()->dir_user_repo_objs();

    /*
     * Setup the tier related members
     */
    dirtyFlashObjs = new ObjQueue(maxDirtyObjs);
    fds_verify(dirtyFlashObjs->is_lock_free() == true);
    writeBackThreads = new fds_threadpool(numWBThreads);

    /*
     * stats class init
     */
    objStats = &gl_objStats;

    // Create leveldb
    smObjDb = new  SmObjDb(this, obj_dir, g_fdslog);
    // init the checksum verification class
    chksumPtr =  new checksum_calc();

    /* Token state db */
    tokenStateDb_.reset(new kvstore::TokenStateDB());
    // TODO(Rao): token state db population should be done based on
    // om events.  For now hardcoding
    for (fds_token_id tok = 0; tok < 256; tok++) {
        tokenStateDb_->addToken(tok);
    }

    counters_.reset(new SMCounters("SM", modProvider_->get_cntrs_mgr().get()));

    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone") == false) {
        /* Set up FDSP RPC endpoints */
        nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));
        myIp = net::get_local_ip(modProvider_->get_fds_config()->get<std::string>("fds.nic_if"));
        setup_datapath_server(myIp);

        /*
         * Register this node with OM.
         */
        LOGNOTIFY << "om ip: " << *modProvider_->get_plf_manager()->plf_get_om_ip()
                  << " port: " << modProvider_->get_plf_manager()->plf_get_om_ctrl_port();
        omClient = new OMgrClient(FDSP_STOR_MGR,
                                  *modProvider_->get_plf_manager()->plf_get_om_ip(),
                                  modProvider_->get_plf_manager()->plf_get_om_ctrl_port(),
                                  "localhost-sm",
                                  GetLog(),
                                  nst_, modProvider_->get_plf_manager());
    }

    /*
     * Create local volume table. Create after omClient
     * is initialized, because it needs to register with the
     * omClient. Create before register with OM because
     * the OM vol event receivers depend on this table.
     */
    volTbl = new StorMgrVolumeTable(this, GetLog());

    // Init the object store
    // TODO(Andrew): The object store should be executed as part
    // of the module initialization since it needs to discover
    // any prior data and possibly perform recovery before allowing
    // another module layer to come up above it.
    objectStore = ObjectStore::unique_ptr(new ObjectStore("SM Object Store Module",
                                                          this,
                                                          volTbl));
    objectStore->mod_init(mod_params);

    /* Create tier related classes -- has to be after volTbl is created */
    FdsRootDir::fds_mkdir(modProvider_->proc_fdsroot()->dir_fds_var_stats().c_str());
    std::string obj_stats_dir = modProvider_->proc_fdsroot()->dir_fds_var_stats();

    // over-write data verify from config
    fds_data_verify = modProvider_->get_fds_config()->get<bool>("fds.sm.data_verify");

    /**
     * Config for Rank Engine
     */
    fds_uint32_t rank_tbl_size = DEFAULT_RANK_TBL_SIZE;
    fds_uint32_t hot_threshold = DEFAULT_HOT_THRESHOLD;
    fds_uint32_t cold_threshold = DEFAULT_COLD_THRESHOLD;
    fds_uint32_t rank_freq_sec = DEFAULT_RANK_FREQUENCY;
    // if tier testing is on, config will over-write default tier params
    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.test_tier")) {
        rank_tbl_size = modProvider_->get_fds_config()->get<int>("fds.sm.testing.rank_tbl_size");
        rank_freq_sec = modProvider_->get_fds_config()->get<int>("fds.sm.testing.rank_freq_sec");
        hot_threshold = modProvider_->get_fds_config()->get<int>("fds.sm.testing.hot_threshold");
        cold_threshold = modProvider_->get_fds_config()->get<int>("fds.sm.testing.cold_threshold");
    }

    rankEngine = new ObjectRankEngine(obj_stats_dir, rank_tbl_size, rank_freq_sec,
                                      hot_threshold, cold_threshold, volTbl, objStats);
    tierEngine = new TierEngine(TierEngine::FDS_TIER_PUT_ALGO_BASIC_RANK,
                                volTbl, rankEngine, g_fdslog);

    fds_uint32_t max_cache_sz_mb = modProvider_->get_fds_config()->get<int>(
        "fds.sm.cache.default_cache_size");
    vol_max_cache_sz_mb_ = modProvider_->get_fds_config()->get<int>(
        "fds.sm.cache.default_vol_max_cache_size");
    vol_min_cache_sz_mb_ = modProvider_->get_fds_config()->get<int>(
        "fds.sm.cache.default_vol_min_cache_size");
    objCache = new FdsObjectCache(1024 * 1024 * max_cache_sz_mb,
                                  slab_allocator_type_default,
                                  eviction_policy_type_default,
                                  g_fdslog);

    // TODO(???): join this thread
    std::thread *stats_thread = new std::thread(log_ocache_stats);

    // qos defaults
    qosThrds = modProvider_->get_fds_config()->get<int>(
        "fds.sm.qos.default_qos_threads");
    qosOutNum = modProvider_->get_fds_config()->get<int>(
        "fds.sm.qos.default_outstanding_io");

    // Temporary boolean to determine which stubs to execute
    // Mainly to toggle between two execution paths: old and new.
    // This is to mitigate risk with new code paths.
    execNewStubs = modProvider_->get_fds_config()->get<bool>(
        "fds.sm.testing.exec_new_stubs");
    LOGDEBUG << "execNewStubs flag=" << execNewStubs;

    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone") == false) {
        /*
         * Register/boostrap from OM
         */
        omClient->initialize();
        omClient->registerEventHandlerForNodeEvents(
            (node_event_handler_t)nodeEventOmHandler);
        omClient->registerEventHandlerForMigrateEvents(
            (migration_event_handler_t)migrationEventOmHandler);
        omClient->registerEventHandlerForDltCloseEvents(
            (dltclose_event_handler_t) dltcloseEventHandler);
        omClient->omc_srv_pol = &sg_SMVolPolicyServ;
        omClient->startAcceptingControlMessages();
        omClient->registerNodeWithOM(modProvider_->get_plf_manager());

        clust_comm_mgr_.reset(new ClusterCommMgr(omClient));

        omc_srv_pol = &sg_SMVolPolicyServ;
        setup_migration_svc(obj_dir);
    }

    testUturnAll    = modProvider_->get_fds_config()->get<bool>("fds.sm.testing.uturn_all");
    testUturnPutObj = modProvider_->get_fds_config()->get<bool>("fds.sm.testing.uturn_putobj");
}

//
// Finishing initialization -- at this point we can access platform service
// and get info like node capabilities we need for properly configuring QoS
// control. So here we initialize everything depending on QoS control
//
void ObjectStorMgr::mod_enable_service()
{
    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone") == false) {
        const NodeUuid *mySvcUuid = Platform::plf_get_my_svc_uuid();
        NodeAgent::pointer sm_node = Platform::plf_sm_nodes()->agent_info(*mySvcUuid);
        fpi::StorCapMsg stor_cap;
        sm_node->init_stor_cap_msg(&stor_cap);

        // note that qos dispatcher in SM/DM uses total rate just to assign
        // guaranteed slots, it still will dispatch more IOs if there is more
        // perf capacity available (based on how fast IOs return). So setting
        // totalRate to disk_iops_min does not actually restrict the SM from
        // servicing more IO if there is more capacity (eg.. because we have
        // cache and SSDs)
        totalRate = stor_cap.disk_iops_min;
    }

    /*
     * Setup QoS related members.
     */
    qosCtrl = new SmQosCtrl(this,
                            qosThrds,
                            FDS_QoSControl::FDS_DISPATCH_WFQ,
                            GetLog());
    qosCtrl->runScheduler();

    /* Set up the journal */
    omJrnl = new TransJournal<ObjectID, ObjectIdJrnlEntry>(qosCtrl, GetLog());

    // create queue for system tasks
    sysTaskQueue = new SmVolQueue(FdsSysTaskQueueId,
                                  256,
                                  getSysTaskIopsMax(),
                                  getSysTaskIopsMin(),
                                  getSysTaskPri());

    qosCtrl->registerVolume(FdsSysTaskQueueId,
                            sysTaskQueue);
    // TODO(Rao): Size it appropriately
    objCache->vol_cache_create(FdsSysTaskQueueId, 1024 * 1024 * vol_min_cache_sz_mb_,
                               1024 * 1024 * vol_max_cache_sz_mb_);

    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone") == false) {
        // Enable stats collection in SM for stats streaming
        StatsCollector::singleton()->registerOmClient(omClient);
        StatsCollector::singleton()->startStreaming(
            std::bind(&ObjectStorMgr::sampleSMStats, this, std::placeholders::_1), NULL);
    }

    /*
     * Create local variables for test mode
     */
    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.test_mode") == true) {
        /*
         * Create test volumes.
         */
        VolumeDesc*  testVdb;
        std::string testVolName;
        int numTestVols = modProvider_->get_fds_config()->\
                          get<int>("fds.sm.testing.test_volume_cnt");
        for (fds_int32_t testVolId = 1; testVolId < numTestVols + 1; testVolId++) {
            testVolName = "testVol" + std::to_string(testVolId);
            /*
             * We're using the ID as the min/max/priority
             * for the volume QoS.
             */
            /* high max iops so that unit tests does not take forever to finish */
            testVdb = new VolumeDesc(testVolName,
                                     testVolId,
                                     8+ testVolId,
                                     10000,
                                     testVolId);
            fds_assert(testVdb != NULL);
            testVdb->volType = FDSP_VOL_BLKDEV_TYPE;
            if ( (testVolId % 4) == 0)
                testVdb->mediaPolicy = FDSP_MEDIA_POLICY_HDD;
            else if ( (testVolId % 3) == 1)
                testVdb->mediaPolicy = FDSP_MEDIA_POLICY_SSD;
            else if ( (testVolId % 3) == 2)
                testVdb->mediaPolicy = FDSP_MEDIA_POLICY_HYBRID;
            else
                testVdb->mediaPolicy = FDSP_MEDIA_POLICY_HYBRID_PREFCAP;

            volEventOmHandler(testVolId,
                              testVdb,
                              FDS_VOL_ACTION_CREATE,
                              FDS_ProtocolInterface::FDSP_NOTIFY_VOL_NO_FLAG,
                              FDS_ProtocolInterface::FDSP_ERR_OK);

            delete testVdb;
        }
    }

    /*
     * Kick off the writeback thread(s)
     */
    writeBackThreads->schedule(writeBackFunc, this);
}

void ObjectStorMgr::mod_shutdown()
{
    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone")) {
        return;  // no migration or netsession
    }
    migrationSvc_->mod_shutdown();
    nst_->endAllSessions();
    nst_.reset();
}

void ObjectStorMgr::setup_datapath_server(const std::string &ip)
{
    ObjectStorMgrI *osmi = new ObjectStorMgrI();
    datapath_handler_.reset(osmi);

    int myIpInt = netSession::ipString2Addr(ip);
    std::string node_name = "_SM";
    // TODO(???): Ideally createServerSession should take a shared pointer
    // for datapath_handler.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    // TODO(???): Figure out who cleans up datapath_session_
    datapath_session_ = nst_->createServerSession<netDataPathServerSession>(
        myIpInt,
        modProvider_->get_plf_manager()->plf_get_my_data_port(),
        node_name,
        FDSP_STOR_HVISOR,
        datapath_handler_);
}

void ObjectStorMgr::setup_migration_svc(const std::string& obj_dir)
{
    migrationSvc_.reset(new FdsMigrationSvc(this,
                                            FdsConfigAccessor(modProvider_->get_fds_config(),
                                                              "fds.sm.migration."),
                                            obj_dir,
                                            GetLog(),
                                            nst_,
                                            clust_comm_mgr_,
                                            tokenStateDb_));
    migrationSvc_->mod_startup();
}

int ObjectStorMgr::run()
{
    nst_->listenServer(datapath_session_);
    return 0;
}

void
ObjectStorMgr::addSvcMap(const NodeUuid    &svcUuid,
                         const SessionUuid &sessUuid) {
    svcSessLock.write_lock();
    LOGNORMAL << "NodeUuid: " << svcUuid.uuid_get_val() << ", Session Uuid: " << sessUuid;
    svcSessMap[svcUuid] = sessUuid;
    svcSessLock.write_unlock();
}

ObjectStorMgr::SessionUuid
ObjectStorMgr::getSvcSess(const NodeUuid &svcUuid) {
    SessionUuid sessId;
    svcSessLock.read_lock();
    fds_verify(svcSessMap.count(svcUuid) > 0);
    sessId = svcSessMap[svcUuid];
    svcSessLock.read_unlock();

    return sessId;
}

DPRespClientPtr
ObjectStorMgr::fdspDataPathClient(const std::string& session_uuid) {
    return datapath_session_->getRespClient(session_uuid);
}

const TokenList&
ObjectStorMgr::getTokensForNode(const NodeUuid &uuid) const {
    return omClient->getTokensForNode(uuid);
}

void
ObjectStorMgr::getTokensForNode(TokenList *tl,
                                const NodeUuid &uuid,
                                fds_uint32_t index) {
    return omClient->getCurrentDLT()->getTokens(tl,
                                                uuid,
                                                index);
}

fds_uint32_t
ObjectStorMgr::getTotalNumTokens() const {
    return omClient->getCurrentDLT()->getNumTokens();
}

NodeUuid
ObjectStorMgr::getUuid() const {
    return omClient->getUuid();
}

const DLT* ObjectStorMgr::getDLT() {
    return omClient->getCurrentDLT();
}

fds_bool_t ObjectStorMgr::amIPrimary(const ObjectID& objId) {
    if (modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone") == true) {
        return true;  // TODO(Anna) add test DLT and use my svc uuid = 1
    }
    DltTokenGroupPtr nodes = omClient->getDLTNodesForDoidKey(objId);
    fds_verify(nodes->getLength() > 0);
    const NodeUuid *mySvcUuid = modProvider_->get_plf_manager()->plf_get_my_svc_uuid();
    return (*mySvcUuid == nodes->get(0));
}

fds_token_id ObjectStorMgr::getTokenId(const ObjectID& objId) {
    return omClient->getCurrentDLT()->getToken(objId);
}

kvstore::TokenStateDBPtr ObjectStorMgr::getTokenStateDb()
{
    return tokenStateDb_;
}

bool ObjectStorMgr::isTokenInSyncMode(const fds_token_id &tokId) {
    kvstore::TokenStateInfo::State state;
    tokenStateDb_->getTokenState(tokId, state);
    return (state == kvstore::TokenStateInfo::SYNCING ||
            state == kvstore::TokenStateInfo::PULL_REMAINING);
}

void ObjectStorMgr::migrationEventOmHandler(bool dlt_type)
{
    auto curDlt = objStorMgr->omClient->getCurrentDLT();
    auto prevDlt = objStorMgr->omClient->getPreviousDLT();
    std::set<fds_token_id> tokens =
            DLT::token_diff(objStorMgr->getUuid(), curDlt, prevDlt);
    // TODO(Rao): hack to not exercise migration code path.  Once
    // we enable multinode enable this code path
    tokens.clear();
    prevDlt = curDlt;
    ////////////////////////////////////////////////////////////

    GLOGNORMAL << " tokens to copy size: " << tokens.size();

    if (tokens.size() > 0) {
        for (auto t : tokens) {
            /* This says we own the token, but don't have any data for it */
            objStorMgr->getTokenStateDb()->\
                    setTokenState(t, kvstore::TokenStateInfo::UNINITIALIZED);
        }
        objStorMgr->tok_migrated_for_dlt_ = true;
        /* Issue bulk copy request */
        MigSvcBulkCopyTokensReqPtr copy_req(new MigSvcBulkCopyTokensReq());
        copy_req->tokens = tokens;
        // Send migration request to migration service
        copy_req->migsvc_resp_cb = std::bind(
            &ObjectStorMgr::migrationSvcResponseCb,
            objStorMgr,
            std::placeholders::_1,
            std::placeholders::_2);
        FdsActorRequestPtr copy_far(new FdsActorRequest(
            FAR_ID(MigSvcBulkCopyTokensReq), copy_req));
        objStorMgr->migrationSvc_->send_actor_request(copy_far);
    } else {
        /* Nothing to migrate case */
        if (curDlt == prevDlt) {
            /* This is the first node.  Set all tokens owned by this node to
             * healthy.
             * TODO(Rao): This is hacky.  We need a better way to test first
             * node case.
             */
            GLOGDEBUG << "No dlt change.  Nothing to migrate.  Setting all tokens in"
                    " current dlt to healthy";
            auto tokens = curDlt->getTokens(objStorMgr->getUuid());
            for (auto t : tokens) {
                /* This says we own the token, but don't have any data for it */
                objStorMgr->getTokenStateDb()->\
                        setTokenState(t, kvstore::TokenStateInfo::HEALTHY);
            }
        }
        objStorMgr->tok_migrated_for_dlt_ = false;
        objStorMgr->migrationSvcResponseCb(Error(ERR_OK), TOKEN_COPY_COMPLETE);
    }
}

void ObjectStorMgr::handleDltUpdate() {
    // until we start getting dlt from platform, we need to path dlt
    // width to object store, so that we can correctly map object ids
    // to SM tokens
    const DLT* curDlt = objStorMgr->omClient->getCurrentDLT();
    objStorMgr->objectStore->handleNewDlt(curDlt);
}

void ObjectStorMgr::dltcloseEventHandler(FDSP_DltCloseTypePtr& dlt_close,
        const std::string& session_uuid)
{
    fds_verify(objStorMgr->cached_dlt_close_.second == nullptr);
    objStorMgr->cached_dlt_close_.first = session_uuid;
    objStorMgr->cached_dlt_close_.second = dlt_close;

    MigSvcSyncCloseReqPtr close_req(new MigSvcSyncCloseReq());
    close_req->sync_close_ts = get_fds_timestamp_ms();

    FdsActorRequestPtr close_far(new FdsActorRequest(
            FAR_ID(MigSvcSyncCloseReq), close_req));

    objStorMgr->migrationSvc_->send_actor_request(close_far);

    GLOGNORMAL << "Received ioclose. Time: " << close_req->sync_close_ts;

    /* It's possible no tokens were migrated.  In this we case we simulate
     * MIGRATION_OP_COMPLETE.
     */
    if (objStorMgr->tok_migrated_for_dlt_ == false) {
        objStorMgr->migrationSvcResponseCb(ERR_OK, MIGRATION_OP_COMPLETE);
    }
}

void ObjectStorMgr::migrationSvcResponseCb(const Error& err,
        const MigrationStatus& status) {
    if (status == TOKEN_COPY_COMPLETE) {
        LOGNORMAL << "Token copy complete";
        omClient->sendMigrationStatusToOM(err);
    } else if (status == MIGRATION_OP_COMPLETE) {
        LOGNORMAL << "Token migration complete";
        LOGNORMAL << migrationSvc_->mig_cntrs.toString();
        LOGNORMAL << counters_->toString();

        /* Notify OM */
        // TODO(Rao): We are notifying OM that sync is complete by responding
        // back to sync/io close.  This is bit of a hack.  In future we should
        // have a proper way to notify OM
        if (objStorMgr->cached_dlt_close_.second) {
            omClient->sendDLTCloseAckToOM(objStorMgr->cached_dlt_close_.second,
                    objStorMgr->cached_dlt_close_.first);
            objStorMgr->cached_dlt_close_.first.clear();
            objStorMgr->cached_dlt_close_.second.reset();
        }
        objStorMgr->tok_migrated_for_dlt_ = false;
    }
}

void ObjectStorMgr::nodeEventOmHandler(int node_id,
                                       unsigned int node_ip_addr,
                                       int node_state,
                                       fds_uint32_t node_port,
                                       FDS_ProtocolInterface::FDSP_MgrIdType node_type)
{
    GLOGDEBUG << "ObjectStorMgr - Node event Handler " << node_id
              << " Node IP Address " <<  node_ip_addr;
    switch (node_state) {
        case FDS_Node_Up :
            GLOGNOTIFY << "ObjectStorMgr - Node UP event NodeId "
                       << node_id << " Node IP Address " <<  node_ip_addr;
            break;
        case FDS_Node_Down:
        case FDS_Node_Rmvd:
            GLOGNOTIFY << " ObjectStorMgr - Node Down event NodeId: "
                       << node_id << " node IP addr" << node_ip_addr;
            break;
        case FDS_Start_Migration:
            GLOGNOTIFY << " ObjectStorMgr - Start Migration  event NodeId: "
                       << node_id << " node IP addr" << node_ip_addr;
            break;
    }
}

/*
 * Note this function is generally run in the context
 * of an Ice thread.
 */
Error
ObjectStorMgr::volEventOmHandler(fds_volid_t  volumeId,
                                 VolumeDesc  *vdb,
                                 int          action,
                                 FDSP_NotifyVolFlag vol_flag,
                                 FDSP_ResultType result) {
    StorMgrVolume* vol = NULL;
    Error err(ERR_OK);
    fds_assert(vdb != NULL);

    switch (action) {
        case FDS_VOL_ACTION_CREATE :
            GLOGNOTIFY << "Received create for vol "
                       << "[" << std::hex << volumeId << std::dec << ", "
                       << vdb->getName() << "]";
            /*
             * Needs to reference the global SM object
             * since this is a static function.
             */
            err = objStorMgr->volTbl->registerVolume(*vdb);
            if (err.ok()) {
                vol = objStorMgr->volTbl->getVolume(volumeId);
                fds_assert(vol != NULL);
                err = objStorMgr->qosCtrl->registerVolume(vdb->isSnapshot() ?
                        vdb->qosQueueId : vol->getVolId(),
                        static_cast<FDS_VolumeQueue*>(vol->getQueue().get()));
                if (err.ok()) {
                    objStorMgr->objCache->
                            vol_cache_create(volumeId,
                                             1024 * 1024 * objStorMgr->vol_min_cache_sz_mb_,
                                             1024 * 1024 * objStorMgr->vol_max_cache_sz_mb_);
                } else {
                    // most likely axceeded min iops
                    objStorMgr->volTbl->deregisterVolume(volumeId);
                }
            }
            if (!err.ok()) {
                GLOGERROR << "Registration failed for vol id " << std::hex << volumeId
                          << std::dec << " error: " << err.GetErrstr();
            }
            break;
        case FDS_VOL_ACTION_DELETE:
            GLOGNOTIFY << "Received delete for vol "
                       << "[" << std::hex << volumeId << std::dec << ", "
                       << vdb->getName() << "]";
            objStorMgr->qosCtrl->quieseceIOs(volumeId);
            objStorMgr->qosCtrl->deregisterVolume(volumeId);
            objStorMgr->volTbl->deregisterVolume(volumeId);
            break;
        case fds_notify_vol_mod:
            GLOGNOTIFY << "Received modify for vol "
                       << "[" << std::hex << volumeId << std::dec << ", "
                       << vdb->getName() << "]";

            vol = objStorMgr->volTbl->getVolume(volumeId);
            fds_assert(vol != NULL);
            if (vol->voldesc->mediaPolicy != vdb->mediaPolicy) {
                GLOGWARN << "Modify volume requested to modify media policy "
                         << "- Not supported yet! Not modifying media policy";
            }

            vol->voldesc->modifyPolicyInfo(vdb->iops_min, vdb->iops_max, vdb->relativePrio);
            err = objStorMgr->qosCtrl->modifyVolumeQosParams(vol->getVolId(),
                                                             vdb->iops_min,
                                                             vdb->iops_max,
                                                             vdb->relativePrio);
            if ( !err.ok() )  {
                GLOGERROR << "Modify volume policy failed for vol " << vdb->getName()
                          << std::hex << volumeId << std::dec << " error: "
                          << err.GetErrstr();
            }
            break;
        default:
            fds_panic("Unknown (corrupt?) volume event recieved!");
    }

    return err;
}

//
// sample SM-specific stats to the collector
//
void ObjectStorMgr::sampleSMStats(fds_uint64_t timestamp) {
    LOGDEBUG << "Sampling SM stats";
    std::map<std::string, int64_t> counter_map;
    /*
    FdsCounters* counters = g_cntrs_mgr->get_counters(PERF_COUNTERS_NAME);
    counters->toMap(counter_map);
    for (std::map<std::string, int64_t>::const_iterator cit = counter_map.cbegin();
         cit != counter_map.cend();
         ++cit) {
        LOGDEBUG << "COUNTER: " << cit->first << " : " << cit->second;
    }
    */
    std::list<fds_volid_t> volIds = volTbl->getVolList();
    for (std::list<fds_volid_t>::iterator vit = volIds.begin();
         vit != volIds.end();
         vit++) {
        if (volTbl->isSnapshot(*vit)) {
            continue;
        }
        fds_uint64_t dedup_bytes = volTbl->getDedupBytes(*vit);
        LOGDEBUG << "Volume " << std::hex << *vit << std::dec
                 << " deduped bytes " << dedup_bytes;
        StatsCollector::singleton()->recordEvent(*vit,
                                                 timestamp,
                                                 STAT_SM_CUR_DEDUP_BYTES,
                                                 dedup_bytes);
    }
}

void ObjectStorMgr::writeBackFunc(ObjectStorMgr *parent) {
    ObjectID   *objId;
    fds_bool_t  nonEmpty;
    Error       err;

    while (1) {
        if (parent->isShuttingDown()) {
            break;
        }

        /*
         * Find an object to write back.
         * TODO: Should add a bulk-object interface.
         */
        objId = NULL;
        nonEmpty = parent->popDirtyFlash(&objId);
        if (nonEmpty == false) {
            /*
             * If the queue is empty sleep for some
             * period of time.
             * Note I have no idea if this is the
             * correct amount of time or not.
             */
            // GLOGDEBUG << "Nothing dirty in flash, going to sleep...";
            sleep(5);
            continue;
        }
        fds_verify(objId != NULL);

        /*
         * Blocking call to write back (mirror) this object
         * to disk.
         * TODO: Mark the object as being accessed to ensure
         * it's not evicted by another thread in the meantime.
         */
        err = parent->writeBackObj(*objId);
        fds_verify(err == ERR_OK);

        delete objId;
    }
}

Error ObjectStorMgr::writeBackObj(const ObjectID &objId)
{
    Error err(ERR_OK);
    OpCtx opCtx(OpCtx::WRITE_BACK, 0);
    fds_volid_t  vol = 0;

    LOGDEBUG << "Writing back object " << objId
             << " from flash to disk";

    /*
     * Read back the object from flash.
     * TODO: We should pin the object in cache when writing
     * to flash so that we can read it back from memory rather
     * than flash.
     */
    ObjectBuf objData;
    ObjMetaData objMap;
    err = readObject(SmObjDb::NON_SYNC_MERGED, objId, objMap, objData);
    if (err != ERR_OK) {
        return err;
    }

    /*
     * Write object back to disk tier.
     */
    err = writeObjectToTier(opCtx, objId, objData, vol, diskio::diskTier);
    if (err != ERR_OK) {
        return err;
    }

    /*
     * Mark the object as 'clean' somewhere.
     */

    return err;
}

void ObjectStorMgr::unitTest() {
    Error err(ERR_OK);

    LOGDEBUG << "Running unit test";

    /*
     * Create fake objects
     */
    std::string object_data("Hi, I'm object data.");
    FDSP_PutObjTypePtr put_obj_req(new FDSP_PutObjType());

    put_obj_req->volume_offset = 0;
    put_obj_req->data_obj_id.digest[0] = 0x11;
    put_obj_req->data_obj_id.digest[1] = 0x12;
    put_obj_req->data_obj_id.digest[2] = 0x13;
    put_obj_req->data_obj_id.digest[3] = 0x14;
    put_obj_req->data_obj_len = object_data.length() + 1;
    put_obj_req->data_obj = object_data;

    /*
     * Create fake volume ID
     */
    fds_uint64_t vol_id   = 0xbeef;
    fds_uint32_t num_objs = 1;
    FDSP_MsgHdrTypePtr msgHdr = static_cast<FDSP_MsgHdrTypePtr>(new FDSP_MsgHdrType);

    /*
     * Write fake object.
     * Note: we're just adding a hard coded 0 for
     * the request ID.
     */
    err = enqPutObjectReq(msgHdr, put_obj_req, vol_id, 0, num_objs);
    if (err != ERR_OK) {
        LOGERROR << "Failed to put object ";
        // delete put_obj_req;
        return;
    }
    // delete put_obj_req;

    FDSP_GetObjTypePtr get_obj_req(new FDSP_GetObjType());
    memset(reinterpret_cast<char *>(&(get_obj_req->data_obj_id)),
           0x00,
           sizeof(get_obj_req->data_obj_id));
    get_obj_req->data_obj_id.digest[0] = 0x21;
    get_obj_req->data_obj_id.digest[1] = 0x22;
    get_obj_req->data_obj_id.digest[2] = 0x23;
    get_obj_req->data_obj_id.digest[3] = 0x24;
    err = enqGetObjectReq(msgHdr, get_obj_req, vol_id, 1, num_objs);
    // delete get_obj_req;
}

Error
ObjectStorMgr::writeObjectMetaData(const OpCtx &opCtx,
        const ObjectID& objId,
        fds_uint32_t obj_size,
        obj_phy_loc_t  *obj_phy_loc,
        fds_bool_t     relocate_flag,
        DataTier       fromTier,
        meta_vol_io_t  *vol) {
    Error err(ERR_OK);

    ObjMetaData objMap;

    LOGDEBUG << "Appending new location for object " << objId;

    smObjDb->lock(objId);
    /*
     * Get existing object locations
     */
    err = readObjMetaData(objId, objMap);
    if (err != ERR_OK && err != ERR_NOT_FOUND) {
        LOGERROR << "Failed to read existing object locations"
                    << " during location write";
        smObjDb->unlock(objId);
        return err;
     } else if (err == ERR_NOT_FOUND) {
            /*
             * Assume this error means the key just did not exist.
             * TODO: Add an err to differention "no key" from "failed read".
             */
            objMap.initialize(objId, obj_size);
            LOGDEBUG << "Not able to read existing object locations"
                    << ", assuming no prior entry existed";
            err = ERR_OK;
     }
     // Lock the SM object DB

    /*
     * Add new location to existing locations
     */
    objMap.updatePhysLocation(obj_phy_loc);

    fds_bool_t update_dup = false;
    std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
    switch (opCtx.type) {
      case OpCtx::RELOCATE :
           objMap.removePhyLocation(fromTier);
           break;
      case OpCtx::WRITE_BACK:
      case OpCtx::GC_COPY:
           break;
      default:
          if (amIPrimary(objId)) {
              objMap.getVolsRefcnt(vols_refcnt);
          }
          update_dup = true;
          objMap.updateAssocEntry(objId, (fds_volid_t)vol->vol_uuid);
          break;
    }

    err = smObjDb->put(opCtx, objId, objMap);
    if (err == ERR_OK) {
        if (update_dup) {
            volTbl->updateDupObj((fds_volid_t)vol->vol_uuid, objId, obj_size,
                                 true, vols_refcnt);
        }
        LOGDEBUG << "Updating object location for object "
                << objId << " to " << objMap;
    } else {
        LOGERROR << "Failed to put object " << objId
                << " into odb with error " << err;
    }
    smObjDb->unlock(objId);

    return err;
}

/*
 * Reads all object locations
 */
Error
ObjectStorMgr::readObjMetaData(const ObjectID &objId,
                               ObjMetaData& objMap)
{
    Error err = smObjDb->get(objId, objMap);
    if (err == ERR_OK) {
        LOGDEBUG << objId
                 << " refcnt:" << objMap.getRefCnt()
                 << " dataexists:" << objMap.dataPhysicallyExists();
        /* While sync is going on we can have metadata but object could be missing */
        if (!objMap.dataPhysicallyExists()) {
            fds_verify(isTokenInSyncMode(getDLT()->getToken(objId)));
            err = ERR_SM_OBJECT_DATA_MISSING;
        }
    } else {
        LOGDEBUG << "unable to read object meta: " << objId;
    }
    return err;
}

Error
ObjectStorMgr::deleteObjectMetaData(const OpCtx &opCtx,
                                    const ObjectID& objId, fds_volid_t vol_id,
                                    ObjMetaData& objMap) {
    Error err(ERR_OK);

    smObjDb->lock(objId);
    /*
     * Get existing object locations
     */
    err = readObjMetaData(objId, objMap);
    if (err != ERR_OK && err != ERR_NOT_FOUND) {
        LOGERROR << "Failed to read existing object locations"
                << " during location write";
        smObjDb->unlock(objId);
        return err;
    } else if (err == ERR_NOT_FOUND) {
        /*
         * Assume this error means the key just did not exist.
         * TODO: Add an err to differention "no key" from "failed read".
         */
        LOGDEBUG << "Not able to read existing object locations"
                << ", assuming no prior entry existed";
        err = ERR_OK;
        smObjDb->unlock(objId);
        return err;
    }

    /*
     * Set the ref_cnt to 0, which will be the delete marker for the Garbage collector
     */
    std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
    if (amIPrimary(objId)) {
        objMap.getVolsRefcnt(vols_refcnt);
    }

    objMap.deleteAssocEntry(objId, vol_id, fds::util::getTimeStampMillis());
    err = smObjDb->put(opCtx, objId, objMap);
    if (err == ERR_OK) {
        volTbl->updateDupObj(vol_id, objId, objMap.getObjSize(),
                             false, vols_refcnt);
        LOGDEBUG << "Setting the delete marker for object "
                << objId << " to " << objMap;
    } else {
        LOGERROR << "Failed to put object " << objId
                << " into odb with error " << err;
    }
    smObjDb->unlock(objId);

    return err;
}

Error
ObjectStorMgr::readObject(const SmObjDb::View& view, const ObjectID& objId,
        ObjMetaData& objMetadata,
        ObjectBuf& objData) {
    diskio::DataTier tier;
    return readObject(view, objId, objMetadata, objData, tier, 0);
}

Error
ObjectStorMgr::readObject(const SmObjDb::View& view, const ObjectID& objId,
        ObjectBuf& objData) {
    ObjMetaData objMetadata;
    diskio::DataTier tier;
    return readObject(view, objId, objMetadata, objData, tier, 0);
}

/*
 * Note the tierUsed parameter is an output param.
 * It gets set in the function and the prior
 * value is unused.
 * It is only guaranteed to be set if success
 * is returned
 */
Error
ObjectStorMgr::readObject(const SmObjDb::View& view,
                          const ObjectID   &objId,
                          ObjMetaData      &objMetadata,
                          ObjectBuf        &objData,
                          diskio::DataTier &tierUsed,
                          fds_volid_t volId) {
    Error err(ERR_OK);

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    diskio::DataTier tierToUse;

    /*
     * TODO: Why to we create another oid structure here?
     * Just pass a ref to objId?
     */
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());

    // create a blocking request object
    disk_req = new SmPlReq(vio, oid, static_cast<ObjectBuf *>(&objData), true);

    /*
     * Read all of the object's locations
     */
    {
        PerfContext tmp_pctx(SM_OBJ_METADATA_DB_READ, volId, "volume:" + std::to_string(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = readObjMetaData(objId, objMetadata);
    }
    if (err == ERR_OK) {
        /*
         * Read obj from flash if we can
         */
        if (objMetadata.onFlashTier()) {
            tierToUse = diskio::flashTier;
        } else {
            /*
             * Read obj from disk
             */
            tierToUse = diskio::diskTier;
        }
        // Update the request with the tier info from disk
        disk_req->setTier(tierToUse);
        disk_req->set_phy_loc(objMetadata.getObjPhyLoc(tierToUse));
        tierUsed = tierToUse;

        LOGDEBUG << "Reading object " << objId << " from "
                << ((disk_req->getTier() == diskio::diskTier) ? "disk" : "flash")
                 << " tier, volume " << std::hex << volId << std::dec;
        objData.size = objMetadata.getObjSize();
        objData.data.resize(objData.size, 0);
        // Now Read the object buffer from the disk
        {
            PerfContext tmp_pctx(SM_OBJ_DATA_DISK_READ, volId, "volume:" + std::to_string(volId));
            SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
            err = dio_mgr.disk_read(disk_req);
        }
        if (err != ERR_OK) {
            LOGDEBUG << " Disk Read Err: " << err;
            delete disk_req;
            return err;
        }
        if (tierUsed == diskio::flashTier) {
            PerfTracer::incr(GET_SSD_OBJ, volId, "volume:" + std::to_string(volId));
        }

        if (fds_data_verify) {
           ObjectID onDiskObjId;
           // Recompute ObjecId for the on-disk object buffer
           onDiskObjId = ObjIdGen::genObjectId(objData.data.c_str(),
                                       objData.data.size());
           LOGDEBUG << " Disk Read ObjectId: " << onDiskObjId.ToHex().c_str() << " err  " << err;
           if (onDiskObjId != objId) {
                err = ERR_ONDISK_DATA_CORRUPT;
                fds_panic("Encountered a on-disk data corruption checking requsted object %s \n != %s. Bailing out now!",  // NOLINT
                          objId.ToHex().c_str(), onDiskObjId.ToHex().c_str());
           }
       }
    }
    delete disk_req;
    return err;
}

/**
 * Checks if an object ID already exists. An object
 * data buffer is passed so that inline buffer
 * comparison needs to be made.
 *
 * An err value of ERR_DUPLICATE is returned if the object
 * is a duplicate, ERR_OK if it is not a duplicate, and an
 * error if something else when wrong during the lookup.
 */
Error
ObjectStorMgr::checkDuplicate(const ObjectID&  objId,
        const ObjectBuf& objCompData, ObjMetaData &objMap) {
    Error err(ERR_OK);

    ObjectBuf objGetData;

    /*
     * We need to fix this once diskmanager keeps track of object size
     * and allocates buffer automatically.
     * For now, we will pass the fixed block size for size and preallocate
     * memory for that size.
     */

    objGetData.size = 0;
    objGetData.data = "";

    err = readObject(SmObjDb::NON_SYNC_MERGED, objId, objMap, objGetData);

    if (err == ERR_OK) {
        /*
         * Perform an inline check that the data is the same.
         * This uses the std::string comparison function.
         * TODO: We need a better solution. This is going to
         * be crazy slow to always do an inline check.
         */
        if (objGetData.data == objCompData.data) {
            err = ERR_DUPLICATE;
        } else {
            /*
             * Handle hash-collision - insert the next collision-id+obj-id
             */
            if (fds_data_verify) {
              ObjectID putBufObjId;
              putBufObjId = ObjIdGen::genObjectId(objCompData.data.c_str(),
                                       objCompData.data.size());
              LOGDEBUG << " Network-RPC ObjectId: " << putBufObjId.ToHex().c_str()
                       << " err  " << err;
              if (putBufObjId != objId) {
                  err = ERR_NETWORK_CORRUPT;
              }
            } else {
                err = ERR_HASH_COLLISION;
                fds_panic("Encountered a hash collision checking object %s. Bailing out now!",
                            objId.ToHex().c_str());
            }
        }
    } else if (err == ERR_NOT_FOUND) {
        /*
         * This error indicates the DB entry was empty
         * so we can reset the error to OK.
         */
        err = ERR_OK;
    }

    return err;
}

/*
 * Note the tier parameter is an output param.
 * It gets set in the function and the prior
 * value is unused.
 * It is only guaranteed to be set if success
 * is returned.
 */
Error
ObjectStorMgr::writeObject(const OpCtx &opCtx,
                           const ObjectID   &objId,
                           const ObjectBuf  &objData,
                           fds_volid_t       volId,
                           diskio::DataTier &tier) {
    /*
     * Ask the tiering engine which tier to place this object
     */
    tier = tierEngine->selectTier(objId, volId);
    return writeObjectToTier(opCtx, objId, objData, volId, tier);
}

/**
 * Based on bWriteMetaData writes/updates the object metadata.
 * Then writes object data to tier
 * @param objId
 * @param objData
 * @param tier
 * @return
 */
Error
ObjectStorMgr::writeObjectToTier(const OpCtx &opCtx,
        const ObjectID  &objId,
        const ObjectBuf &objData,
        fds_volid_t volId,
        diskio::DataTier tier) {
    Error err(ERR_OK);

    fds_verify((tier == diskio::diskTier) ||
               (tier == diskio::flashTier));

    // TODO(Rao): Use writeObjectDataToTier() here

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    fds_bool_t      pushOk;

    vio.vol_uuid = volId;
    /*
     * TODO: Why to we create another oid structure here?
     * Just pass a ref to objId?
     */
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());

    LOGDEBUG << "Writing object " << objId << " into the "
             << ((tier == diskio::diskTier) ? "disk" : "flash")
             << " tier";

    /* Disk write */
    disk_req = new SmPlReq(vio, oid,
                           const_cast<ObjectBuf *>(&objData),
                           true, tier);  // blocking call
    {
        // TODO(matteo): move ctx in disk_req
        PerfContext tmp_pctx(SM_OBJ_DATA_DISK_WRITE,
                             volId, "volume:" + std::to_string(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        // TODO(Matteo) inside or around this function, it could be non blocking
        err = dio_mgr.disk_write(disk_req);
    }
    if (err != ERR_OK) {
        LOGDEBUG << " 1. Disk Write Err: " << err;
        delete disk_req;
        return err;
    }
    {
        // TODO(Matteo): look inside. This is for metadata.
        // Initially we probably want just leveldb get and put
        PerfContext tmp_pctx(SM_OBJ_METADATA_DB_WRITE, volId, "volume:" + std::to_string(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = writeObjectMetaData(opCtx, objId, objData.data.length(),
                disk_req->req_get_phy_loc(), false, diskio::diskTier, &vio);
    }
    StorMgrVolume *vol = volTbl->getVolume(volId);
    if ((err == ERR_OK) &&
        (tier == diskio::flashTier)) {
         vol->ssdByteCnt += objId.GetLen();
         PerfTracer::incr(PUT_SSD_OBJ, volId, "volume:" + std::to_string(volId));
         if (vol->voldesc->mediaPolicy != FDSP_MEDIA_POLICY_SSD) {
           /*
            * If written to flash, add to dirty flash list
            */
           pushOk = dirtyFlashObjs->push(new ObjectID(objId));
           fds_verify(pushOk == true);
        }
    } else if ((err == ERR_OK) &&
        (tier == diskio::diskTier)) {
        // TODO(Anna) if this is a write-back thread, then volId passed to this method
        // is currently 0. In reality, an object maybe associated with multiple volumes,
        // so we need to think whose counters we update (probably all volumes?)
        // For now, counters will not properly work here.
         PerfTracer::incr(PUT_HDD_OBJ, volId, "volume:" + std::to_string(volId));
         if (vol) {
             vol->hddByteCnt += objId.GetLen();
         }
    }

    delete disk_req;
    return err;
}

/**
 * Writes the object data to storage tier.
 * @param objId
 * @param objData
 * @param tier
 * @param phys_loc - on return phys_loc is returned
 * @return
 */
Error
ObjectStorMgr::writeObjectDataToTier(const ObjectID  &objId,
        const ObjectBuf &objData,
        diskio::DataTier tier,
        obj_phy_loc_t& phys_loc) {
    Error err(ERR_OK);

    fds_verify((tier == diskio::diskTier) ||
               (tier == diskio::flashTier));

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    fds_bool_t      pushOk;

    /*
     * TODO: Why to we create another oid structure here?
     * Just pass a ref to objId?
     */
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());

    LOGDEBUG << "Writing object data " << objId << " into the "
             << ((tier == diskio::diskTier) ? "disk" : "flash")
             << " tier";

    /* Disk write */
    disk_req = new SmPlReq(vio, oid,
                           const_cast<ObjectBuf *>(&objData),
                           true, tier);  // blocking call
    err = dio_mgr.disk_write(disk_req);
    if (err != ERR_OK) {
        LOGDEBUG << " 1. Disk Write Err: " << err;
        delete disk_req;
        return err;
    }

    if (tier == diskio::flashTier) {
        /*
         * If written to flash, add to dirty flash list
         */
        pushOk = dirtyFlashObjs->push(new ObjectID(objId));
        fds_verify(pushOk == true);
    }
    phys_loc = *disk_req->req_get_phy_loc();
    delete disk_req;

    return err;
}

Error
ObjectStorMgr::relocateObject(const ObjectID &objId,
                              diskio::DataTier from_tier,
                              diskio::DataTier to_tier) {
    Error err(ERR_OK);
    ObjectBuf objGetData;
    OpCtx opCtx(OpCtx::RELOCATE, 0);
    ObjMetaData objMeta;

    objGetData.size = 0;
    objGetData.data = "";

    err = readObject(SmObjDb::NON_SYNC_MERGED, objId, objMeta, objGetData);

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;

    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());

    disk_req = new SmPlReq(vio, oid,
                           static_cast<ObjectBuf *>(&objGetData),
                           true, to_tier);
    err = dio_mgr.disk_write(disk_req);
    if (err != ERR_OK) {
        LOGDEBUG << " 2. Disk Write Err: " << err;
        delete disk_req;
        return err;
    }
    err = writeObjectMetaData(opCtx, objId, objGetData.data.length(),
            disk_req->req_get_phy_loc(), true, from_tier, &vio);

    LOGDEBUG << "relocateObject " << objId << " into the "
             << ((to_tier == diskio::diskTier) ? "disk" : "flash")
             << " tier";
    delete disk_req;
    // Delete the object
    return err;
}

/*------------------------------------------------------------------------- ------------
 * FDSP Protocol internal processing 
 -------------------------------------------------------------------------------------*/

/**
 * Process a single object put.
 * @param the request structure ptr
 * @return any associated error
 */
Error
ObjectStorMgr::putObjectInternalSvc(SmIoPutObjectReq *putReq) {
    Error err(ERR_OK);
    const ObjectID&  objId    = putReq->getObjId();
    fds_volid_t volId         = putReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;
    ObjBufPtrType objBufPtr = NULL;
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(putReq->getTransId());
    OpCtx opCtx(OpCtx::PUT, putReq->origin_timestamp);
    bool new_buff_allocated = false, added_cache = false;

    ObjMetaData objMap;
    // objStorMutex->lock();

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);
    fds_assert(putReq->origin_timestamp != 0);

#ifdef OBJCACHE_ENABLE
    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));

        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, putReq->perfNameStr);

        // Now check for dedup here.
        if (objBufPtr->data == putReq->data_obj) {
            err = ERR_DUPLICATE;
        } else {
            PerfTracer::incr(HASH_COLLISION, volId, putReq->perfNameStr);
            /*
             * Handle hash-collision - insert the next collision-id+obj-id
             */
            err = ERR_HASH_COLLISION;
        }
        objCache->object_release(volId, objId, objBufPtr);
    }

    // Find if this object is a duplicate
    if (err == ERR_OK) {
        // Nothing in cache. Let's allocate a new buf from cache mgr and copy over the data

        objBufPtr = objCache->object_alloc(volId, objId, putReq->data_obj.size());
        // TODO(Rao): We should use std move here
        memcpy(const_cast<char *>(objBufPtr->data.c_str()),
               reinterpret_cast<const void *>(putReq->data_obj.c_str()),
               putReq->data_obj.size());
        new_buff_allocated = true;

        err = checkDuplicate(objId,
                *objBufPtr, objMap);
    }

    if (err == ERR_DUPLICATE) {
        PerfTracer::incr(DUPLICATE_OBJ, volId, putReq->perfNameStr);
        if (new_buff_allocated) {
            added_cache = true;
            objCache->object_add(volId, objId, objBufPtr, false);
            objCache->object_release(volId, objId, objBufPtr);
        }
        LOGDEBUG << "Put dup:  " << err
                << ", returning success";

        smObjDb->lock(objId);
        // Update  the AssocEntry for dedupe-ing
        err = readObjMetaData(objId, objMap);
        if (err.ok()) {
            std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
            if (amIPrimary(objId)) {
                objMap.getVolsRefcnt(vols_refcnt);
            }

            objMap.updateAssocEntry(objId, volId);
            err = smObjDb->put(opCtx, objId, objMap);
            if (err == ERR_OK) {
                dedupeByteCnt += objId.GetLen();
                volTbl->updateDupObj(volId, objId, putReq->data_obj.size(),
                                     true, vols_refcnt);
                LOGDEBUG << "Dedupe object Assoc Entry for object "
                         << objId << " to " << objMap;
            } else {
                LOGERROR << "Failed to put ObjMetaData " << objId
                         << " into odb with error " << err;
            }
        } else {
            LOGERROR << "Failed to read ObjMetaData from db" << err;
        }
        smObjDb->unlock(objId);

        /*
         * Reset the err to OK to ack the metadata update.
         */
        err = ERR_OK;
    } else if (err != ERR_OK) {
        if (new_buff_allocated) {
            objCache->object_release(volId, objId, objBufPtr);
            objCache->object_delete(volId, objId);
        }
        LOGERROR << "Failed to check object duplicate status on put: "
                 << err;
    } else {
        /*
         * Write the object and record which tier it went to
         */        
        SCOPED_PERF_TRACEPOINT_CTX(putReq->opLatencyCtx);

        err = writeObject(opCtx, objId, *objBufPtr, volId, tierUsed);
        if (err.ok()) {
            LOGDEBUG << "Successfully put object " << objId;
            /* if we successfully put to flash -- notify ranking engine */
            if (tierUsed == diskio::flashTier) {
                StorMgrVolume *vol = volTbl->getVolume(volId);
                fds_verify(vol);
                rankEngine->rankAndInsertObject(objId, *(vol->voldesc));
            }
        }

        if (err != fds::ERR_OK) {
            PerfTracer::incr(putReq->opReqFailedPerfEventType, volId, putReq->perfNameStr);
            objCache->object_release(volId, objId, objBufPtr);
            objCache->object_delete(volId, objId);
            objCache->object_release(volId, objId, objBufPtr);
        } else if (added_cache == false) {
            objCache->object_add(volId, objId, objBufPtr, false);
        }


        /*
         * Stores a reverse mapping from the volume's disk location
         * to the oid at that location.
         */
        /*
          TODO: Comment this back in!
          volTbl->createVolIndexEntry(putReq.io_vol_id,
          put_obj_req->volume_offset,
          put_obj_req->data_obj_id,
          put_obj_req->data_obj_len);
        */
    }
#else
        objBufPtr = objCache->object_alloc(volId, objId, putReq->data_obj.size());
        memcpy(const_cast<void *>(objBufPtr->data.c_str()),
               reinterpret_cast<const void *>(putReq->data_obj.c_str()),
               putReq->data_obj.size());

        err = checkDuplicate(objId,
                *objBufPtr, objMap);
        if (err == ERR_DUPLICATE) {
          // Update  the AssocEntry for dedupe-ing
          LOGDEBUG << " Object  ID: " << objId << " objMap: " << objMap;
          smObjDb->lock(objId);
          err = readObjMetaData(objId, objMap);
          /* At this point object metadata must exist */
          fds_verify(err == ERR_OK);

          std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
          if (amIPrimary(objId)) {
              objMap.getVolsRefcnt(vols_refcnt);
          }

          objMap.updateAssocEntry(objId, volId);
          err = smObjDb->put(opCtx, objId, objMap);
          if (err.ok()) {
              volTbl->updateDupObj(volId, objId, putReq->data_obj.size(),
                                   true, vols_refcnt);
          }
          smObjDb->unlock(objId);
          if (err == ERR_OK) {
              LOGDEBUG << "Dedupe object Assoc Entry for object "
                      << objId << " to " << objMap;
          } else {
              LOGERROR << "Failed to put ObjMetaData " << objId
                      << " into odb with error " << err;
          }

          err = ERR_OK;
        } else {
         SCOPED_PERF_TRACEPOINT_CTX(putReq->opLatencyCtx);
         err = writeObject(opCtx, objId, *objBufPtr, volId, tierUsed);
         if (err.ok()) {
             LOGDEBUG << "Successfully put object " << objId;
             /* if we successfully put to flash -- notify ranking engine */
             if (tierUsed == diskio::flashTier) {
                 StorMgrVolume *vol = volTbl->getVolume(volId);
                 fds_verify(vol);
                 rankEngine->rankAndInsertObject(objId, *(vol->voldesc));
             }
         } else {
             PerfTracer::incr(putReq->opReqFailedPerfEventType, volId, putReq->perfNameStr);
         }
       }
       objCache->object_release(volId, objId, objBufPtr);
       objCache->object_delete(volId, objId);

#endif
    // objStorMutex->unlock();

    qosCtrl->markIODone(*putReq,
                        tierUsed,
                        amIPrimary(objId));

    PerfTracer::tracePointEnd(putReq->opReqLatencyCtx);

    omJrnl->release_transaction(putReq->getTransId());

    putReq->response_cb(err, putReq);

    return err;
}

//
// This is an empty interface stub for getObjectInternalSvc.
// It's intent is to plug in a new implementation of future
// architecture into this interface for testing and evaluation.
Error
ObjectStorMgr::putObjectInternalSvcV2(SmIoPutObjectReq *putReq)
{
    Error err(ERR_OK);
    const ObjectID&  objId    = putReq->getObjId();
    fds_volid_t volId         = putReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    LOGDEBUG << "Executing putObjectInternalSvcV2";

    PerfTracer::tracePointBegin(putReq->opReqLatencyCtx);
    PerfTracer::tracePointBegin(putReq->opLatencyCtx);

    // TODO(Andrew): Remove this copy. The network should allocated
    // a shared ptr structure so that we can directly store that, even
    // after the network message is freed.
    err = objectStore->putObject(volId,
                                 objId,
                                 boost::make_shared<const std::string>(
                                     putReq->putObjectNetReq->data_obj));
    qosCtrl->markIODone(*putReq,
                        tierUsed,
                        amIPrimary(objId));

    PerfTracer::tracePointEnd(putReq->opLatencyCtx);
    PerfTracer::tracePointEnd(putReq->opReqLatencyCtx);

    putReq->response_cb(err, putReq);
    return err;
}


/**
 * Process a single object put.
 * @param the request structure ptr
 * @return any associated error
 */
Error
ObjectStorMgr::putObjectInternal(SmIoReq* putReq) {
    Error err(ERR_OK);
    const ObjectID&  objId    = putReq->getObjId();
    fds_volid_t volId         = putReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;
    ObjBufPtrType objBufPtr = NULL;
    const FDSP_PutObjTypePtr& putObjReq = putReq->getPutObjReq();
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(putReq->getTransId());
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = jrnlEntry->getMsgHdr();
    OpCtx opCtx(OpCtx::PUT, msgHdr->origin_timestamp);
    bool new_buff_allocated = false, added_cache = false;

    ObjMetaData objMap;
    // objStorMutex->lock();

#ifdef OBJCACHE_ENABLE
    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));

        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, putReq->perfNameStr);

        // Now check for dedup here.
        if (objBufPtr->data == putObjReq->data_obj) {
            PerfTracer::incr(DUPLICATE_OBJ, volId, putReq->perfNameStr);
            err = ERR_DUPLICATE;
        } else {
            /*
             * Handle hash-collision - insert the next collision-id+obj-id
             */
            err = ERR_HASH_COLLISION;
        }
        objCache->object_release(volId, objId, objBufPtr);
    }

    // Find if this object is a duplicate
    if (err == ERR_OK) {
        // Nothing in cache. Let's allocate a new buf from cache mgr and copy over the data

        objBufPtr = objCache->object_alloc(volId, objId, putObjReq->data_obj.size());
        memcpy(const_cast<char *>(objBufPtr->data.c_str()),
               reinterpret_cast<const void *>(putObjReq->data_obj.c_str()),
               putObjReq->data_obj.size());
        new_buff_allocated = true;

        err = checkDuplicate(objId,
                *objBufPtr, objMap);
    }

    if (err == ERR_DUPLICATE) {
        if (new_buff_allocated) {
            added_cache = true;
            objCache->object_add(volId, objId, objBufPtr, false);
            objCache->object_release(volId, objId, objBufPtr);
        }
        LOGDEBUG << "Put dup:  " << err
                << ", returning success";

        smObjDb->lock(objId);
        // Update  the AssocEntry for dedupe-ing
        err = readObjMetaData(objId, objMap);
        if (err.ok()) {
            std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
            if (amIPrimary(objId)) {
                objMap.getVolsRefcnt(vols_refcnt);
            }

            objMap.updateAssocEntry(objId, volId);
            err = smObjDb->put(opCtx, objId, objMap);
            if (err == ERR_OK) {
                volTbl->updateDupObj(volId, objId, putObjReq->data_obj.size(),
                                     true, vols_refcnt);
                LOGDEBUG << "Dedupe object Assoc Entry for object "
                         << objId << " to " << objMap;
            } else {
                LOGERROR << "Failed to put ObjMetaData " << objId
                         << " into odb with error " << err;
            }
        } else {
            LOGERROR << "Failed to read ObjMetaData from db" << err;
        }
        smObjDb->unlock(objId);

        /*
         * Reset the err to OK to ack the metadata update.
         */
        err = ERR_OK;
    } else if (err != ERR_OK) {
        if (new_buff_allocated) {
            objCache->object_release(volId, objId, objBufPtr);
            objCache->object_delete(volId, objId);
        }
        LOGERROR << "Failed to check object duplicate status on put: "
                 << err;
    } else {
        /*
         * Write the object and record which tier it went to
         */
        SCOPED_PERF_TRACEPOINT_CTX(putReq->opLatencyCtx);
        err = writeObject(opCtx, objId, *objBufPtr, volId, tierUsed);

        if (err != fds::ERR_OK) {
            objCache->object_release(volId, objId, objBufPtr);
            objCache->object_delete(volId, objId);
            objCache->object_release(volId, objId, objBufPtr);
            LOGDEBUG << "Successfully put object " << objId;
            /* if we successfully put to flash -- notify ranking engine */
            if (tierUsed == diskio::flashTier) {
                StorMgrVolume *vol = volTbl->getVolume(volId);
                fds_verify(vol);
                rankEngine->rankAndInsertObject(objId, *(vol->voldesc));
            }
        } else if (added_cache == false) {
            objCache->object_add(volId, objId, objBufPtr, false);
        }



        /*
         * Stores a reverse mapping from the volume's disk location
         * to the oid at that location.
         */
        /*
          TODO: Comment this back in!
          volTbl->createVolIndexEntry(putReq.io_vol_id,
          put_obj_req->volume_offset,
          put_obj_req->data_obj_id,
          put_obj_req->data_obj_len);
        */
    }
#else
        objBufPtr = objCache->object_alloc(volId, objId, putObjReq->data_obj.size());
        memcpy(const_cast<void *>(objBufPtr->data.c_str()),
               reinterpret_cast<const void *>(putObjReq->data_obj.c_str()),
               putObjReq->data_obj.size());

        err = checkDuplicate(objId,
                *objBufPtr, objMap);
        if (err == ERR_DUPLICATE) {
          // Update  the AssocEntry for dedupe-ing
          LOGDEBUG << " Object  ID: " << objId << " objMap: " << objMap;
          smObjDb->lock(objId);
          err = readObjMetaData(objId, objMap);
          /* At this point object metadata must exist */
          fds_verify(err == ERR_OK);

          std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
          if (amIPrimary(objId)) {
              objMap.getVolsRefcnt(vols_refcnt);
          }

          objMap.updateAssocEntry(objId, volId);
          err = smObjDb->put(opCtx, objId, objMap);
          if (err.ok()) {
              volTbl->updateDupObj(volId, objId, putObjReq->data_obj.size(),
                                   true, vols_refcnt);
          }
          smObjDb->unlock(objId);
          if (err == ERR_OK) {
              LOGDEBUG << "Dedupe object Assoc Entry for object "
                      << objId << " to " << objMap;
          } else {
              LOGERROR << "Failed to put ObjMetaData " << objId
                      << " into odb with error " << err;
          }

          err = ERR_OK;
        } else {
         SCOPED_PERF_TRACEPOINT_CTX(putReq->opLatencyCtx);
         err = writeObject(opCtx, objId, *objBufPtr, volId, tierUsed);

          if (err != fds::ERR_OK) {
              LOGDEBUG << "Successfully put object " << objId;
              /* if we successfully put to flash -- notify ranking engine */
              if (tierUsed == diskio::flashTier) {
                  StorMgrVolume *vol = volTbl->getVolume(volId);
                  fds_verify(vol);
                  rankEngine->rankAndInsertObject(objId, *(vol->voldesc));
              }
         }
       }
       objCache->object_release(volId, objId, objBufPtr);
       objCache->object_delete(volId, objId);

#endif
    // objStorMutex->unlock();

    qosCtrl->markIODone(*putReq,
                        tierUsed,
                        amIPrimary(objId));

    PerfTracer::tracePointEnd(putReq->opReqLatencyCtx);

    /*
     * Prepare a response to send back.
     */

    FDSP_PutObjTypePtr putObj(new FDSP_PutObjType());
    putObj->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    putObj->data_obj_len          = putObjReq->data_obj_len;

    if (err == ERR_OK) {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
        msgHdr->err_code = err.getFdspErr();
    } else {
        PerfTracer::incr(putReq->opReqFailedPerfEventType, volId, putReq->perfNameStr);
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
        msgHdr->err_code = err.getFdspErr();
    }

    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_RSP;
    swapMgrId(msgHdr);
    try {
        fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
    } catch(att::TTransportException& e) {
        LOGERROR << "error during network call: " << e.what();
    } catch(...) {
        LOGERROR << "Unexpected exception during network call";
        throw;
    }
    omJrnl->release_transaction(putReq->getTransId());
    LOGDEBUG << "Sent async PutObj response after processing " << objId;

    /*
     * Free the IO request structure that
     * was allocated when the request was
     * enqueued.
     */
    delete putReq;

    return err;
}

/**
 * Creates a transaction for obj_id.  If there is no pending transaction for
 * obj_id then io is enqueued to qos scheduler.  If there is a pending
 * transaction, io is queued in transaction journal and will be scheduled
 * when the pending transaction is complete.
 * @param obj_id
 * @param ioReq
 * @param trans_id
 * @return
 */
Error
ObjectStorMgr::enqTransactionIo(FDSP_MsgHdrTypePtr msgHdr,
                                const ObjectID& obj_id,
                                SmIoReq *ioReq, TransJournalId &trans_id)
{
    GLOGDEBUG << "enqTransactionIo " << ioReq->getTransId() << endl;
    PerfTracer::tracePointBegin(ioReq->opReqLatencyCtx);

    // TODO(Rao): Refactor create_transaction so that it just takes key and cb as
    // params
    Error err = omJrnl->create_transaction(obj_id,
                                           static_cast<FDS_IOType *>(ioReq), trans_id,
                                           std::bind(&ObjectStorMgr::create_transaction_cb, this,
                                                     msgHdr, ioReq, std::placeholders::_1));
    if (err != ERR_OK && err != ERR_TRANS_JOURNAL_REQUEST_QUEUED) {
        PerfTracer::tracePointEnd(ioReq->opReqLatencyCtx);
        PerfTracer::incr(ioReq->opReqFailedPerfEventType, ioReq->getVolId(), ioReq->perfNameStr);

        return err;
    }

    if (err == ERR_TRANS_JOURNAL_REQUEST_QUEUED) {
        return ERR_OK;
    }

    StorMgrVolume* smVol = volTbl->getVolume(ioReq->getVolId());
    fds_assert(smVol);

    err = qosCtrl->enqueueIO(smVol->getQueue()->getVolUuid(), ioReq);
    if (err != ERR_OK) {
        PerfTracer::tracePointEnd(ioReq->opReqLatencyCtx);
        PerfTracer::incr(ioReq->opReqFailedPerfEventType, ioReq->getVolId(), ioReq->perfNameStr);
    }
    return err;
}

/**
 * This callback is invoked after creating a trasaction.
 * Though it's ugly, it's necessary so we can do the necessary work after
 * creating transaction under lock
 * @param msgHdr
 * @param ioReq
 * @param trans_id
 */
void ObjectStorMgr::create_transaction_cb(FDSP_MsgHdrTypePtr msgHdr,
                                          SmIoReq *ioReq, TransJournalId trans_id)
{
    ioReq->setTransId(trans_id);
    if (msgHdr.get()) {
        ObjectIdJrnlEntry *jrnlEntry = omJrnl->get_transaction_nolock(trans_id);
        jrnlEntry->setMsgHdr(msgHdr);
    }
}

Error
ObjectStorMgr::enqPutObjectReq(FDSP_MsgHdrTypePtr msgHdr,
                               FDSP_PutObjTypePtr putObjReq,
                               fds_volid_t        volId,
                               fds_uint32_t       am_transId,
                               fds_uint32_t       numObjs) {
    fds::Error err(fds::ERR_OK);
    TransJournalId trans_id;
    ObjectID obj_id(putObjReq->data_obj_id.digest);

    fds_verify(msgHdr->origin_timestamp != 0);

    for (fds_uint32_t obj_num = 0; obj_num < numObjs; obj_num++) {
        /*
         * Allocate the ioReq structure. The pointer will get copied
         * into the queue and freed later once the IO completes.
         */
        SmIoReq *ioReq = new SmIoReq(putObjReq->data_obj_id.digest,
                                     // putObjReq->data_obj,
                                     putObjReq,
                                     volId,
                                     FDS_IO_WRITE,
                                     am_transId);
        err = enqTransactionIo(msgHdr, obj_id, ioReq, trans_id);
        if (err != ERR_OK) {
            /*
             * Just return if we see an error. The way the function is setup
             * it's a bit unnatrual to return errors for multiple IOs so
             * we'll just stop at the first error we see to make sure it
             * doesn't get lost.
             */
            LOGERROR << "Unable to enqueue putObject request "
                     << am_transId << ":" << trans_id;
            delete ioReq;
            return err;
        }
        LOGDEBUG << "Successfully enqueued putObject request "
                 << am_transId << ":" << trans_id;
    }

    return err;
}

Error
ObjectStorMgr::deleteObjectInternal(SmIoReq* delReq) {
    Error err(ERR_OK);
    const ObjectID&  objId    = delReq->getObjId();
    fds_volid_t volId         = delReq->getVolId();
    ObjBufPtrType objBufPtr = NULL;
    const FDSP_DeleteObjTypePtr& delObjReq = delReq->getDeleteObjReq();
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(delReq->getTransId());
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = jrnlEntry->getMsgHdr();
    OpCtx opCtx(OpCtx::DELETE, msgHdr->origin_timestamp);
    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    ObjMetaData objMetadata;
    meta_obj_id_t   oid;

    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, delReq->perfNameStr);
        objCache->object_release(volId, objId, objBufPtr);
        objCache->object_delete(volId, objId);
    }

    PerfTracer::tracePointBegin(delReq->opLatencyCtx);
    /*
     * Delete the object, decrement refcnt of the assoc entry & overall refcnt
     */
    {
        PerfContext tmp_pctx(SM_OBJ_MARK_DELETED, volId, "volume:" + std::to_string(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = deleteObjectMetaData(opCtx, objId, volId, objMetadata);
    }
    if (err.ok()) {
        LOGDEBUG << "Successfully delete object " << objId
                 << " refcnt = " << objMetadata.getRefCnt();

        if (objMetadata.getRefCnt() < 1) {
            // tell persistent layer we deleted the object so that garbage collection
            // knows how much disk space we need to clean
            memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
            PerfContext tmp_pctx(SM_OBJ_MARK_DELETED, volId, "volume:" + std::to_string(volId));
            SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
            if (objMetadata.onTier(diskio::diskTier)) {
                dio_mgr.disk_delete_obj(&oid,
                                        objMetadata.getObjSize(),
                                        objMetadata.getObjPhyLoc(diskio::diskTier));
            } else if (objMetadata.onTier(diskio::flashTier)) {
                dio_mgr.disk_delete_obj(&oid,
                                        objMetadata.getObjSize(),
                                        objMetadata.getObjPhyLoc(diskio::flashTier));
            }
        }
    } else {
        LOGERROR << "Failed to delete object " << objId << ", " << err;
    }

    qosCtrl->markIODone(*delReq,
                        diskio::diskTier);

    PerfTracer::tracePointEnd(delReq->opLatencyCtx);
    PerfTracer::tracePointEnd(delReq->opReqLatencyCtx);

    /*
     * Prepare a response to send back.
     */

    FDSP_DeleteObjTypePtr delObj(new FDS_ProtocolInterface::FDSP_DeleteObjType());
    delObj->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    delObj->data_obj_len          = delObjReq->data_obj_len;

    if (err == ERR_OK) {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
        msgHdr->err_code = err.getFdspErr();
    } else {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
        msgHdr->err_code = err.getFdspErr();
    }

    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_OBJ_RSP;
    swapMgrId(msgHdr);
    try {
        fdspDataPathClient(msgHdr->session_uuid)->DeleteObjectResp(msgHdr, delObj);
    } catch(att::TTransportException& e) {
        LOGERROR << "error during network call: " << e.what();
    }
    omJrnl->release_transaction(delReq->getTransId());
    LOGDEBUG << "Sent async DelObj response after processing";

    /*
     * Free the IO request structure that
     * was allocated when the request was
     * enqueued.
     */
    delete delReq;

    return err;
}

Error
ObjectStorMgr::deleteObjectInternalSvc(SmIoDeleteObjectReq* delReq) {
    Error err(ERR_OK);
    const ObjectID&  objId    = delReq->getObjId();
    fds_volid_t volId         = delReq->getVolId();
    ObjBufPtrType objBufPtr = NULL;
    const FDSP_DeleteObjTypePtr& delObjReq = delReq->getDeleteObjReq();
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(delReq->getTransId());
    OpCtx opCtx(OpCtx::DELETE, delReq->origin_timestamp);

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    ObjMetaData objMetadata;
    meta_obj_id_t   oid;

    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, delReq->perfNameStr);
        objCache->object_release(volId, objId, objBufPtr);
        objCache->object_delete(volId, objId);
    }

    PerfTracer::tracePointBegin(delReq->opLatencyCtx);
    /*
     * Delete the object, decrement refcnt of the assoc entry & overall refcnt
     */
    {
        PerfContext tmp_pctx(SM_OBJ_MARK_DELETED, volId, "volume:" + std::to_string(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = deleteObjectMetaData(opCtx, objId, volId, objMetadata);
    }
    if (err.ok()) {
        LOGDEBUG << "Successfully delete object " << objId
                 << " refcnt = " << objMetadata.getRefCnt();

        if (objMetadata.getRefCnt() < 1) {
            // tell persistent layer we deleted the object so that garbage collection
            // knows how much disk space we need to clean
            memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
            PerfContext tmp_pctx(SM_OBJ_MARK_DELETED, volId, "volume:" + std::to_string(volId));
            SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
            if (objMetadata.onTier(diskio::diskTier)) {
                dio_mgr.disk_delete_obj(&oid, objMetadata.getObjSize(),
                                        objMetadata.getObjPhyLoc(diskio::diskTier));
            } else if (objMetadata.onTier(diskio::flashTier)) {
                dio_mgr.disk_delete_obj(&oid, objMetadata.getObjSize(),
                                        objMetadata.getObjPhyLoc(diskio::flashTier));
            }
        }
    } else {
        LOGERROR << "Failed to delete object " << objId << ", " << err;
    }
    qosCtrl->markIODone(*delReq, diskio::diskTier);

    PerfTracer::tracePointEnd(delReq->opLatencyCtx);
    PerfTracer::tracePointEnd(delReq->opReqLatencyCtx);

    omJrnl->release_transaction(delReq->getTransId());
    delReq->response_cb(err, delReq);

    return err;
}

//
// This is an empty interface stub for getObjectInternalSvc.
// It's intent is to plug in a new implementation of future
// architecture into this interface for testing and evaluation.
Error
ObjectStorMgr::deleteObjectInternalSvcV2(SmIoDeleteObjectReq* delReq)
{
    Error err(ERR_OK);
    const ObjectID&  objId    = delReq->getObjId();
    fds_volid_t volId         = delReq->getVolId();

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    LOGDEBUG << "Executing deleteObjectInternalSvcV2";

    PerfTracer::tracePointBegin(delReq->opReqLatencyCtx);
    PerfTracer::tracePointBegin(delReq->opLatencyCtx);

    err = objectStore->deleteObject(volId, objId);

    qosCtrl->markIODone(*delReq, diskio::diskTier);

    PerfTracer::tracePointEnd(delReq->opLatencyCtx);
    PerfTracer::tracePointEnd(delReq->opReqLatencyCtx);

    delReq->response_cb(err, delReq);
    return err;
}

Error
ObjectStorMgr::addObjectRefInternalSvc(SmIoAddObjRefReq* addObjRefReq) {
    fds_assert(0 != addObjRefReq);
    fds_assert(0 != addObjRefReq->getSrcVolId());
    fds_assert(0 != addObjRefReq->getDestVolId());

    Error rc = ERR_OK;

    if (addObjRefReq->objIds().empty()) {
        return rc;
    }

    PerfTracer::tracePointBegin(addObjRefReq->opReqLatencyCtx);
    PerfTracer::tracePointBegin(addObjRefReq->opLatencyCtx);

    for (auto objId : addObjRefReq->objIds()) {
        ObjectID oid = ObjectID(objId.digest);
        rc = objectStore->copyAssociation(addObjRefReq->getSrcVolId(),
                addObjRefReq->getDestVolId(), oid);
        if (!rc.ok()) {
            LOGERROR << "Failed to add association entry for object " << oid <<
                    "in to odb with err " << rc;
            PerfTracer::incr(addObjRefReq->opReqFailedPerfEventType,
                    addObjRefReq->getSrcVolId(), addObjRefReq->perfNameStr);
            break;
        }
    }

    qosCtrl->markIODone(*addObjRefReq, diskio::maxTier, true);
    PerfTracer::tracePointEnd(addObjRefReq->opLatencyCtx);
    PerfTracer::tracePointEnd(addObjRefReq->opReqLatencyCtx);

    addObjRefReq->response_cb(rc, addObjRefReq);

    return rc;
}

void
ObjectStorMgr::PutObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
                         const FDSP_PutObjTypePtr& put_obj_req) {
    Error err(ERR_OK);

    // Verify the integrity of the FDSP msg using chksums
    //
    // stor_mgr_verify_msg(fdsp_msg);
    //
    ObjectID oid(put_obj_req->data_obj_id.digest);

    LOGDEBUG << "PutObject Obj ID: " << oid
             << ", glob_vol_id: " << fdsp_msg->glob_volume_id
             << ", for request ID: " << fdsp_msg->req_cookie
             << ", Num Objs: " << fdsp_msg->num_objects;
    err = enqPutObjectReq(fdsp_msg, put_obj_req,
                          fdsp_msg->glob_volume_id,
                          fdsp_msg->req_cookie,
                          fdsp_msg->num_objects);
    if (err != ERR_OK) {
        fdsp_msg->result = FDSP_ERR_FAILED;
        fdsp_msg->err_code = err.getFdspErr();
    } else {
        fdsp_msg->result = FDSP_ERR_OK;
        fdsp_msg->err_code = err.getFdspErr();
    }

    counters_->put_reqs.incr();
}


void
ObjectStorMgr::DeleteObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
                            const FDSP_DeleteObjTypePtr& del_obj_req) {
    Error err(ERR_OK);

    // Verify the integrity of the FDSP msg using chksums
    //
    // stor_mgr_verify_msg(fdsp_msg);
    //
    ObjectID oid(del_obj_req->data_obj_id.digest);

    LOGDEBUG << "DeleteObject Obj ID: " << oid
             << ", glob_vol_id: " << fdsp_msg->glob_volume_id
             << ", for request ID: " << fdsp_msg->req_cookie
             << ", Num Objs: " << fdsp_msg->num_objects;
    err = enqDeleteObjectReq(fdsp_msg, del_obj_req,
                             fdsp_msg->glob_volume_id,
                             fdsp_msg->req_cookie);
    if (err != ERR_OK) {
        fdsp_msg->result = FDSP_ERR_FAILED;
        fdsp_msg->err_code = err.getFdspErr();
    } else {
        fdsp_msg->result = FDSP_ERR_OK;
        fdsp_msg->err_code = err.getFdspErr();
    }

    counters_->del_reqs.incr();
}

Error
ObjectStorMgr::getObjectInternalSvc(SmIoGetObjectReq *getReq) {
    Error            err(ERR_OK);
    const ObjectID  &objId = getReq->getObjId();
    fds_volid_t volId      = getReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;
    ObjBufPtrType objBufPtr = NULL;

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    PerfTracer::tracePointBegin(getReq->opLatencyCtx);

    /*
     * We need to fix this once diskmanager keeps track of object size
     * and allocates buffer automatically.
     * For now, we will pass the fixed block size for size and preallocate
     * memory for that size.
     */

    // objStorMutex->lock();
#ifdef OBJCACHE_ENABLE
    objBufPtr = objCache->object_retrieve(volId, objId);
#endif

    if (!objBufPtr) {
        ObjectBuf objData;
        ObjMetaData objMetadata;
        objData.size = 0;
        objData.data = "";
        err = readObject(SmObjDb::SYNC_MERGED, objId, objMetadata, objData, tierUsed, volId);
        if (err == fds::ERR_OK) {
            objBufPtr = objCache->object_alloc(volId, objId, objData.size);
            memcpy(const_cast<char *>(objBufPtr->data.c_str()),
                   reinterpret_cast<const void *>(objData.data.c_str()),
                   objData.size);
            objCache->object_add(volId, objId, objBufPtr, false);  // read data is always clean

            // ACL: If this Volume never put this object, then it should not access the object
            if (!objMetadata.isVolumeAssociated(volId)) {
            // TODO(bao): Need a way to ignore acl to run checker process
#define _IGNORE_ACL_CHECK_FOR_TEST_
#ifdef _IGNORE_ACL_CHECK_FOR_TEST_
                err = fds::ERR_OK;
#else
               err = ERR_UNAUTH_ACCESS;
               LOGDEBUG << "Volume " << volId << " unauth-access of object " << objId
                // << " and data " << objData.data
                << " for request ID " << getReq->io_req_id;
#endif
            }
        }
    } else {
        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, getReq->perfNameStr);
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));
    }

    // objStorMutex->unlock();

    if (err != fds::ERR_OK) {
        PerfTracer::incr(getReq->opReqFailedPerfEventType, volId, getReq->perfNameStr);
        LOGERROR << "Failed to get object " << objId
                 << " with error " << err;
        getReq->obj_data.data.clear();
    } else {
        LOGDEBUG << "Successfully got object " << objId
                 << " for request ID " << getReq->io_req_id
                 << " from tier " << tierUsed << ", volume "
                 << std::hex << volId << std::dec;
        // TODO(Rao): Use std move here
        getReq->obj_data.data = objBufPtr->data;
    }

    qosCtrl->markIODone(*getReq, tierUsed, amIPrimary(objId));

    PerfTracer::tracePointEnd(getReq->opLatencyCtx);
    PerfTracer::tracePointEnd(getReq->opReqLatencyCtx);

    /*
     * Prepare a response to send back.
     */
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(getReq->getTransId());
    omJrnl->release_transaction(getReq->getTransId());

    objStats->updateIOpathStats(getReq->getVolId(), getReq->getObjId());
    volTbl->updateVolStats(getReq->getVolId());

    objCache->object_release(volId, objId, objBufPtr);

    getReq->response_cb(err, getReq);

    return err;
}

//
// This is an empty interface stub for getObjectInternalSvc.
// It's intent is to plug in a new implementation of future
// architecture into this interface for testing and evaluation.
Error
ObjectStorMgr::getObjectInternalSvcV2(SmIoGetObjectReq *getReq)
{
    Error err(ERR_OK);
    const ObjectID&  objId    = getReq->getObjId();
    fds_volid_t volId         = getReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    LOGDEBUG << "Executing getObjectInternalSvcV2";

    PerfTracer::tracePointBegin(getReq->opReqLatencyCtx);
    PerfTracer::tracePointBegin(getReq->opLatencyCtx);

    boost::shared_ptr<const std::string> objData =
            objectStore->getObject(volId,
                                   objId,
                                   err);
    if (err.ok()) {
        // TODO(Andrew): Remove this copy. The network should allocated
        // a shared ptr structure so that we can directly store that, even
        // after the network message is freed.
        getReq->getObjectNetResp->data_obj = *objData;
    }

    qosCtrl->markIODone(*getReq, tierUsed, amIPrimary(objId));

    PerfTracer::tracePointEnd(getReq->opLatencyCtx);
    PerfTracer::tracePointEnd(getReq->opReqLatencyCtx);

    getReq->response_cb(err, getReq);
    return err;
}

Error
ObjectStorMgr::getObjectInternal(SmIoReq *getReq) {
    Error            err(ERR_OK);
    const ObjectID  &objId = getReq->getObjId();
    fds_volid_t volId      = getReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;
    const FDSP_GetObjTypePtr& getObjReq = getReq->getGetObjReq();
    ObjBufPtrType objBufPtr = NULL;

    PerfTracer::tracePointBegin(getReq->opLatencyCtx);

    /*
     * We need to fix this once diskmanager keeps track of object size
     * and allocates buffer automatically.
     * For now, we will pass the fixed block size for size and preallocate
     * memory for that size.
     */

    // objStorMutex->lock();
    objBufPtr = objCache->object_retrieve(volId, objId);

    if (!objBufPtr) {
        ObjectBuf objData;
        ObjMetaData objMetadata;
        objData.size = 0;
        objData.data = "";
        err = readObject(SmObjDb::SYNC_MERGED, objId, objMetadata, objData, tierUsed, volId);
        if (err == fds::ERR_OK) {
            objBufPtr = objCache->object_alloc(volId, objId, objData.size);
            memcpy(const_cast<char *>(objBufPtr->data.c_str()),
                   reinterpret_cast<const void *>(objData.data.c_str()),
                   objData.size);
            objCache->object_add(volId, objId, objBufPtr, false);  // read data is always clean
            // ACL: If this Volume never put this object, then it should not access the object
            if (!objMetadata.isVolumeAssociated(volId)) {
            // TODO(bao): Need a way to ignore acl to run checker process
#define _IGNORE_ACL_CHECK_FOR_TEST_
#ifdef _IGNORE_ACL_CHECK_FOR_TEST_
                err = fds::ERR_OK;
#else
               err = ERR_UNAUTH_ACCESS;
               LOGDEBUG << "Volume " << volId << " unauth-access of object " << objId
                // << " and data " << objData.data
                << " for request ID " << getReq->io_req_id;
#endif
            }
        }
    } else {
        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, getReq->perfNameStr);
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));
    }

    // objStorMutex->unlock();

    if (err != fds::ERR_OK) {
        PerfTracer::incr(getReq->opReqFailedPerfEventType, volId, getReq->perfNameStr);
        LOGERROR << "Failed to get object " << objId
                 << " with error " << err;
        /*
         * Set the data to empty so we don't return
         * garbage.
         */
    } else {
        LOGDEBUG << "Successfully got object " << objId
                // << " and data " << objData.data
                 << " for request ID " << getReq->io_req_id;
    }

    qosCtrl->markIODone(*getReq, tierUsed, amIPrimary(objId));

    PerfTracer::tracePointEnd(getReq->opLatencyCtx);
    PerfTracer::tracePointEnd(getReq->opReqLatencyCtx);

    /*
     * Prepare a response to send back.
     */
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(getReq->getTransId());
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = jrnlEntry->getMsgHdr();

    /*
     * This does an additional object copy into the network buffer.
     */
    FDS_ProtocolInterface::FDSP_GetObjTypePtr
            getObj(new FDS_ProtocolInterface::FDSP_GetObjType());

    getObj->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    getObj->data_obj                 = (err == ERR_OK)? objBufPtr->data:"";
    getObj->data_obj_len             = (err == ERR_OK)? objBufPtr->size:0;

    if (err == ERR_OK) {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
        msgHdr->err_code = err.getFdspErr();
    } else {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
        msgHdr->err_code = err.getFdspErr();
    }
    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_RSP;
    swapMgrId(msgHdr);

    DPRespClientPtr client = fdspDataPathClient(msgHdr->session_uuid);
    if (client == NULL) {
        // We may not know about this session uuid because it may be
        // a redirected(proxied) get from anothe SM. Let's try and
        // locate our session uuid based on the service uuid
        NodeUuid svcUuid(msgHdr->src_service_uuid.uuid);
        SessionUuid sessUuid = getSvcSess(svcUuid);
        client = fdspDataPathClient(sessUuid);
        fds_verify(client != NULL);
        LOGWARN << "Doing a get() redirection to AM with service UUID "
                << svcUuid;
    }
    try {
       client->GetObjectResp(msgHdr, getObj);
    } catch(att::TTransportException& e) {
        LOGERROR << "error during network call: " << e.what();
    }
    LOGDEBUG << "Sent async GetObj response after processing";
    omJrnl->release_transaction(getReq->getTransId());

    objStats->updateIOpathStats(getReq->getVolId(), getReq->getObjId());
    volTbl->updateVolStats(getReq->getVolId());

    objStats->updateIOpathStats(getReq->getVolId(), getReq->getObjId());
    volTbl->updateVolStats(getReq->getVolId());

    objCache->object_release(volId, objId, objBufPtr);

    /*
     * Free the IO request structure that
     * was allocated when the request was
     * enqueued.
     */
    delete getReq;

    return err;
}

Error
ObjectStorMgr::enqDeleteObjectReq(FDSP_MsgHdrTypePtr msgHdr,
                                  FDSP_DeleteObjTypePtr delObjReq,
                                  fds_volid_t        volId,
                                  fds_uint32_t       am_transId) {
    Error err(ERR_OK);
    TransJournalId trans_id;
    ObjectID obj_id(delObjReq->data_obj_id.digest);

    fds_verify(msgHdr->origin_timestamp != 0);

    /*
     * Allocate and enqueue an IO request. The request is freed
     * when the IO completes.
     */
    SmIoReq *ioReq = new SmIoReq(delObjReq->data_obj_id.digest,
                                 // "",
                                 delObjReq,
                                 volId,
                                 FDS_DELETE_BLOB,
                                 msgHdr->req_cookie);

    err =  enqTransactionIo(msgHdr, obj_id, ioReq, trans_id);

    if (err != fds::ERR_OK) {
        LOGERROR << "Unable to enqueue delObject request "
                 << am_transId << ":" << trans_id;
        delete ioReq;
        return err;
    }
    LOGDEBUG << "Successfully enqueued delObject request "
             << am_transId << ":" << trans_id;

    return err;
}

NodeAgentDpClientPtr
ObjectStorMgr::getProxyClient(ObjectID& oid,
                              const FDSP_MsgHdrTypePtr& msg) {
    const DLT* dlt = getDLT();
    NodeUuid uuid = 0;

    // TODO(Andrew): Why is it const if we const_cast it?
    FDSP_MsgHdrTypePtr& fdsp_msg = const_cast<FDSP_MsgHdrTypePtr&> (msg);
    // get the first Node that is not ME!!
    DltTokenGroupPtr nodes = dlt->getNodes(oid);
    for (uint i = 0; i < nodes->getLength(); i++) {
        if (nodes->get(i) != getUuid()) {
            uuid = nodes->get(i);
            break;
        }
    }

    fds_verify(uuid != getUuid());

    LOGDEBUG << "obj " << oid << " not located here "
             << getUuid() << " proxy_count " << fdsp_msg->proxy_count;
    LOGDEBUG << "proxying request to " << uuid;
    fds_int32_t node_state = -1;

    NodeAgent::pointer node = modProvider_->get_plf_manager()->plf_node_inventory()->
            dc_get_sm_nodes()->agent_info(uuid);
    SmAgent::pointer sm = agt_cast_ptr<SmAgent>(node);
    NodeAgentDpClientPtr smClient = sm->get_sm_client();

    // Increment the proxy count so the receiver knows
    // this is a proxied request
    fdsp_msg->proxy_count++;
    return smClient;
}

/*
 * TODO: The error response is currently returned via the msgHdr.
 * We should change the function to return an error. That's a
 * clearer interface.
 */
void
ObjectStorMgr::GetObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
                         const FDSP_GetObjTypePtr& get_obj_req) {
    // Verify the integrity of the FDSP msg using chksums
    //
    // stor_mgr_verify_msg(fdsp_msg);
    //
    Error err(ERR_OK);
    ObjectID oid(get_obj_req->data_obj_id.digest);

    LOGDEBUG << "GetObject XID: " << fdsp_msg->req_cookie
             << ", Obj ID: " << oid
             << ", glob_vol_id: " << fdsp_msg->glob_volume_id
             << ", Num Objs: " << fdsp_msg->num_objects;

    // check if we have the object or else we need to proxy it
    // TODO(Andrew): We should actually be checking if we're in
    // a sync mode for this token, not just blindly redirecting...
    if (!smObjDb->dataPhysicallyExists(oid)) {
        LOGWARN << "object id " << oid << " is not here , find it else where";

        NodeAgentDpClientPtr client = getProxyClient(oid, fdsp_msg);
        fds_verify(client != NULL);
        client->GetObject(*fdsp_msg, *get_obj_req);

        counters_->proxy_gets.incr();
        return;
    }

    err = enqGetObjectReq(fdsp_msg, get_obj_req,
                          fdsp_msg->glob_volume_id,
                          fdsp_msg->req_cookie,
                          fdsp_msg->num_objects);
    if (err != ERR_OK) {
        fdsp_msg->result = FDSP_ERR_FAILED;
        fdsp_msg->err_code = err.getFdspErr();
    } else {
        fdsp_msg->result = FDSP_ERR_OK;
        fdsp_msg->err_code = err.getFdspErr();
    }

    counters_->get_reqs.incr();
}

Error
ObjectStorMgr::enqGetObjectReq(FDSP_MsgHdrTypePtr msgHdr,
                               FDSP_GetObjTypePtr getObjReq,
                               fds_volid_t        volId,
                               fds_uint32_t       am_transId,
                               fds_uint32_t       numObjs) {
    Error err(ERR_OK);
    TransJournalId trans_id;
    ObjectID obj_id(getObjReq->data_obj_id.digest);

    /*
     * Allocate and enqueue an IO request. The request is freed
     * when the IO completes.
     */
    SmIoReq *ioReq = new SmIoReq(getObjReq->data_obj_id.digest,
                                 // "",
                                 getObjReq,
                                 volId,
                                 FDS_IO_READ,
                                 am_transId);

    err =  enqTransactionIo(msgHdr, obj_id, ioReq, trans_id);

    if (err != fds::ERR_OK) {
        LOGERROR << "Unable to enqueue getObject request "
                 << am_transId << ":" << trans_id;
        getObjReq->data_obj_len = 0;
        getObjReq->data_obj.assign("");
        delete ioReq;
        return err;
    }
    LOGDEBUG << "Successfully enqueued getObject request "
             << am_transId << ":" << trans_id;

    return err;
}

/**
 * @brief Enqueues a message into storage manager for processing
 *
 * @param volId
 * @param ioReq
 *
 * @return 
 */
Error ObjectStorMgr::enqueueMsg(fds_volid_t volId, SmIoReq* ioReq)
{
    Error err(ERR_OK);
    ObjectID objectId;
    TransJournalId trans_id;

    ioReq->setVolId(volId);

    switch (ioReq->io_type) {
        case FDS_SM_WRITE_TOKEN_OBJECTS:
        case FDS_SM_READ_TOKEN_OBJECTS:
        {
            StorMgrVolume* smVol = volTbl->getVolume(ioReq->getVolId());
            fds_assert(smVol);
            err = qosCtrl->enqueueIO(smVol->getQueue()->getVolUuid(),
                    static_cast<FDS_IOType*>(ioReq));
            break;
        }
        case FDS_SM_COMPACT_OBJECTS:
        case FDS_SM_SNAPSHOT_TOKEN:
        {
            /*
            StorMgrVolume* smVol = volTbl->getVolume(ioReq->getVolId());
            fds_assert(smVol);
            err = qosCtrl->enqueueIO(smVol->getQueue()->getVolUuid(),
                    static_cast<FDS_IOType*>(ioReq));
            */
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        }
        /* Following are messages that require io synchronization at object
         * id level via transaction table
         */
        case FDS_SM_GET_OBJECT:
            // This is a toggle to execute either a legacy code path or a new
            // code path
            if (execNewStubs == true) {
                StorMgrVolume* smVol = volTbl->getVolume(ioReq->getVolId());
                fds_assert(smVol);
                err = qosCtrl->enqueueIO(smVol->getQueue()->getVolUuid(),
                        static_cast<FDS_IOType*>(ioReq));
            } else {
                objectId = static_cast<SmIoGetObjectReq *>(ioReq)->getObjId();
                err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            }
            break;
        case FDS_SM_PUT_OBJECT:
            // This is a toggle to execute either a legacy code path or a new
            // code path
            if (execNewStubs == true) {
                err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            } else {
                objectId = static_cast<SmIoPutObjectReq *>(ioReq)->getObjId();
                err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            }
            break;
        case FDS_SM_ADD_OBJECT_REF:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_DELETE_OBJECT:
            // This is a toggle to execute either a legacy code path or a new
            // code path
            if (execNewStubs == true) {
                err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            } else {
                objectId = static_cast<SmIoDeleteObjectReq *>(ioReq)->getObjId();
                err = enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            }
            break;
        case FDS_SM_SYNC_APPLY_METADATA:
            objectId.SetId(static_cast<SmIoApplySyncMetadata*>(ioReq)->md.object_id.digest);
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            break;
        case FDS_SM_SYNC_RESOLVE_SYNC_ENTRY:
            objectId = static_cast<SmIoResolveSyncEntry*>(ioReq)->object_id;
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            break;
        case FDS_SM_APPLY_OBJECTDATA:
            objectId = static_cast<SmIoApplyObjectdata*>(ioReq)->obj_id;
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            break;
        case FDS_SM_READ_OBJECTDATA:
            objectId = static_cast<SmIoReadObjectdata*>(ioReq)->getObjId();
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            break;
        case FDS_SM_READ_OBJECTMETADATA:
            objectId = static_cast<SmIoReadObjectMetadata*>(ioReq)->getObjId();
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            break;
        default:
            LOGERROR << "Unknown message: " << ioReq->io_type;
            fds_panic("Unknown message");
    }

    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
    }

    return err;
}


/**
 * @brief Does a bulk put of objects+metadata from obj_list
 * @param the request structure ptr
 * @return any associated error
 */
void
ObjectStorMgr::putTokenObjectsInternal(SmIoReq* ioReq)
{
    Error err(ERR_OK);
    SmIoPutTokObjectsReq *putTokReq = static_cast<SmIoPutTokObjectsReq*>(ioReq);
    FDSP_MigrateObjectList &objList = putTokReq->obj_list;

    for (auto obj : objList) {
        ObjectID objId(obj.meta_data.object_id.digest);

        ObjMetaData objMetadata;
        err = smObjDb->get(objId, objMetadata);
        if (err != ERR_OK && err != ERR_NOT_FOUND) {
            fds_assert(!"ObjMetadata read failed");
            LOGERROR << "Failed to write the object: " << objId;
            break;
        }

        err = ERR_OK;

        /* Apply metadata */
        objMetadata.apply(obj.meta_data);

        /* Update token healthy timestamp
         * TODO(Rao):  Ideally this code belongs in SmObjDb::put()
         */
        getTokenStateDb()->updateHealthyTS(putTokReq->token_id,
                objMetadata.getModificationTs());


        LOGDEBUG << objMetadata.logString();

        if (objMetadata.dataPhysicallyExists()) {
            /* write metadata */
            smObjDb->put(OpCtx(OpCtx::COPY), objId, objMetadata);
            /* No need to write the object data.  It already exits */
            continue;
        }

        /* Moving the data to not incur copy penalty */
        DBG(std::string temp_data = obj.data);
        ObjectBuf objData;
        objData.data = std::move(obj.data);
        objData.size = objData.data.size();
        fds_assert(temp_data == objData.data);
        obj.data.clear();

        /* write data to storage tier */
        obj_phy_loc_t phys_loc;
        err = writeObjectDataToTier(objId, objData, diskio::diskTier, phys_loc);
        if (err != ERR_OK) {
            LOGERROR << "Failed to write the object: " << objId;
            break;
        }

        /* write the metadata */
        objMetadata.updatePhysLocation(&phys_loc);
        smObjDb->put(OpCtx(OpCtx::COPY), objId, objMetadata);
        counters_->put_tok_objs.incr();
    }

    /* Mark the request as complete */
    qosCtrl->markIODone(*putTokReq,
                        diskio::diskTier);

    putTokReq->response_cb(err, putTokReq);
    /* NOTE: We expect the caller to free up ioReq */
}

void
ObjectStorMgr::getTokenObjectsInternal(SmIoReq* ioReq)
{
    Error err(ERR_OK);
    SmIoGetTokObjectsReq *getTokReq = static_cast<SmIoGetTokObjectsReq*>(ioReq);
    smObjDb->iterRetrieveObjects(getTokReq->token_id,
                                 getTokReq->max_size, getTokReq->obj_list, getTokReq->itr);

    /* Mark the request as complete */
    qosCtrl->markIODone(*getTokReq,
                        diskio::diskTier);

    getTokReq->response_cb(err, getTokReq);
    /* NOTE: We expect the caller to free up ioReq */
}
/**
 * Takes snapshot of sm object metadata db identifed by
 * token
 * @param ioReq
 */
void
ObjectStorMgr::snapshotTokenInternal(SmIoReq* ioReq)
{
    Error err(ERR_OK);
    SmIoSnapshotObjectDB *snapReq = static_cast<SmIoSnapshotObjectDB*>(ioReq);

    if (execNewStubs) {
        objectStore->snapshotMetadata(snapReq->token_id,
                                      snapReq->smio_snap_resp_cb);
    } else {
        leveldb::DB *db;
        leveldb::ReadOptions options;
        smObjDb->snapshot(snapReq->token_id, db, options);
        snapReq->smio_snap_resp_cb(err, snapReq, options, db);
    }
    /* Mark the request as complete */
    qosCtrl->markIODone(*snapReq,
                        diskio::diskTier);
}

void
ObjectStorMgr::applySyncMetadataInternal(SmIoReq* ioReq)
{
    SmIoApplySyncMetadata *applyMdReq =  static_cast<SmIoApplySyncMetadata*>(ioReq);
    Error e = smObjDb->putSyncEntry(ObjectID(applyMdReq->md.object_id.digest),
                                    applyMdReq->md, applyMdReq->dataExists);
    if (e != ERR_OK) {
        fds_assert(!"error");
        LOGERROR << "Error in applying sync metadata.  Object Id: "
                 << applyMdReq->md.object_id.digest;
    }
    /* Mark the request as complete */
    qosCtrl->markIODone(*applyMdReq,
            diskio::diskTier);
    omJrnl->release_transaction(applyMdReq->getTransId());

    applyMdReq->smio_sync_md_resp_cb(e, applyMdReq);
}

Error
ObjectStorMgr::condCopyObjectInternal(const ObjectID &objId,
                                      diskio::DataTier tier)
{
    Error err(ERR_OK);
    ObjMetaData objMetadata;
    ObjectBuf objData;
    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();

    err = readObjMetaData(objId, objMetadata);
    if (!err.ok()) {
        LOGERROR << "failed to read metadata for obj " << objId
                 << " error " << err;
        return err;
    }

    if (!TokenCompactor::isDataGarbage(objMetadata, tier)) {
        OpCtx opCtx(OpCtx::GC_COPY, 0);
        meta_obj_id_t   oid;
        meta_vol_io_t   vio;  // passing to disk_req but not used
        SmPlReq     *disk_req = NULL;

        LOGDEBUG << "Will copy " << objId << " to new file on tier " << tier;

        // not garbage, read obj data
        memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
        disk_req = new SmPlReq(vio, oid, static_cast<ObjectBuf *>(&objData), true);
        disk_req->setTier(tier);
        disk_req->set_phy_loc(objMetadata.getObjPhyLoc(tier));
        objData.size = objMetadata.getObjSize();
        objData.data.resize(objData.size, 0);
        err = dio_mgr.disk_read(disk_req);
        if (!err.ok()) {
            LOGERROR << " Disk Read Err: " << err;
            delete disk_req;
            return err;
        }

        // disk write will write it to the new file
        err = dio_mgr.disk_write(disk_req);
        if (!err.ok()) {
            LOGERROR << " Disk Write Err: " << err;
            delete disk_req;
            return err;
        }

        // ok if concurrent reads happen for this obj before
        // updating metadata, because object is still in both places

        // update metadata
        err = writeObjectMetaData(opCtx, objId, objData.data.length(),
                                  disk_req->req_get_phy_loc(), false, tier, &vio);
        if (!err.ok()) {
            LOGERROR << "Failed to update metadata for obj " << objId;
        }

        delete disk_req;
    } else {
        // not going to copy obj
        LOGNORMAL << "Will garbage-collect " << objId << " on tier " << tier;
        // remove entry from index db if data + meta is garbage
        if (TokenCompactor::isGarbage(objMetadata)) {
            LOGNORMAL << "Removing metadata for " << objId;
            smObjDb->lock(objId);
            err = smObjDb->remove(objId);
            smObjDb->unlock(objId);
        }
    }

    return err;
}

void
ObjectStorMgr::compactObjectsInternal(SmIoReq* ioReq)
{
    Error err(ERR_OK);
    SmIoCompactObjects *cobjs_req =  static_cast<SmIoCompactObjects*>(ioReq);
    fds_verify(cobjs_req != NULL);

    for (fds_uint32_t i = 0; i < (cobjs_req->oid_list).size(); ++i) {
        const ObjectID& obj_id = (cobjs_req->oid_list)[i];

        LOGDEBUG << "Compaction is working on object " << obj_id
                 << " on tier " << cobjs_req->tier;

        // copy this object if not garbage, otherwise rm object db entry
        if (execNewStubs) {
            err = objectStore->copyObjectToNewLocation(obj_id, cobjs_req->tier);
        } else {
            err = condCopyObjectInternal(obj_id, cobjs_req->tier);
        }
        if (!err.ok()) {
            LOGERROR << "Failed to compact object " << obj_id
                     << ", error " << err;
            break;
        }
    }

    /* Mark the request as complete */
    qosCtrl->markIODone(*cobjs_req, diskio::diskTier);

    cobjs_req->smio_compactobj_resp_cb(err, cobjs_req);

    delete cobjs_req;
}

void
ObjectStorMgr::resolveSyncEntryInternal(SmIoReq* ioReq)
{
    SmIoResolveSyncEntry *resolve_entry =  static_cast<SmIoResolveSyncEntry*>(ioReq);
    Error e = smObjDb->resolveEntry(resolve_entry->object_id);
    if (e != ERR_OK) {
        fds_assert(!"error");
        LOGERROR << "Error in resolving metadata entry.  Object Id: "
                << resolve_entry->object_id;
    }
    /* Mark the request as complete */
    qosCtrl->markIODone(*resolve_entry,
            diskio::diskTier);
    omJrnl->release_transaction(resolve_entry->getTransId());

    resolve_entry->smio_resolve_resp_cb(e, resolve_entry);
}

void
ObjectStorMgr::applyObjectDataInternal(SmIoReq* ioReq)
{
    SmIoApplyObjectdata *applydata_entry =  static_cast<SmIoApplyObjectdata*>(ioReq);
    ObjectID objId = applydata_entry->obj_id;
    ObjMetaData objMetadata;
    Error err = smObjDb->get(objId, objMetadata);
    if (err != ERR_OK && err != ERR_NOT_FOUND) {
        fds_assert(!"ObjMetadata read failed");
        LOGERROR << "Failed to write the object: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                diskio::diskTier);
        omJrnl->release_transaction(applydata_entry->getTransId());
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    err = ERR_OK;

    if (objMetadata.dataPhysicallyExists()) {
        /* No need to write the object data.  It already exits */
        LOGDEBUG << "Object already exists. Id: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                diskio::diskTier);
        omJrnl->release_transaction(applydata_entry->getTransId());
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    /* Moving the data to not incur copy penalty */
    DBG(std::string temp_data = applydata_entry->obj_data);
    ObjectBuf objData;
    objData.data = std::move(applydata_entry->obj_data);
    objData.size = objData.data.size();
    fds_assert(temp_data == objData.data);
    applydata_entry->obj_data.clear();

    /* write data to storage tier */
    obj_phy_loc_t phys_loc;
    err = writeObjectDataToTier(objId, objData, diskio::diskTier, phys_loc);
    if (err != ERR_OK) {
        fds_assert(!"Failed to write");
        LOGERROR << "Failed to write the object: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                diskio::diskTier);
        omJrnl->release_transaction(applydata_entry->getTransId());
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    /* write the metadata */
    objMetadata.updatePhysLocation(&phys_loc);
    smObjDb->put(OpCtx(OpCtx::SYNC), objId, objMetadata);

    qosCtrl->markIODone(*applydata_entry,
            diskio::diskTier);
    omJrnl->release_transaction(applydata_entry->getTransId());

    applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
}

void
ObjectStorMgr::readObjectDataInternal(SmIoReq* ioReq)
{
    SmIoReadObjectdata *read_entry =  static_cast<SmIoReadObjectdata*>(ioReq);
    ObjectBuf        objData;
    ObjMetaData objMetadata;
    diskio::DataTier tierUsed;

    Error err = readObject(SmObjDb::NON_SYNC_MERGED, read_entry->getObjId(),
                           objMetadata, objData, tierUsed, 0);
    if (err != ERR_OK) {
        fds_assert(!"failed to read");
        LOGERROR << "Failed to read object id: " << read_entry->getObjId();
        qosCtrl->markIODone(*read_entry,
                diskio::diskTier);
        omJrnl->release_transaction(read_entry->getTransId());
        read_entry->smio_readdata_resp_cb(err, read_entry);
    }
    // TODO(Rao): use std::move here
    read_entry->obj_data.data = objData.data;
    fds_assert(read_entry->obj_data.data.size() > 0);

    qosCtrl->markIODone(*read_entry,
            diskio::diskTier);
    omJrnl->release_transaction(read_entry->getTransId());

    read_entry->smio_readdata_resp_cb(err, read_entry);
}

void
ObjectStorMgr::readObjectMetadataInternal(SmIoReq* ioReq)
{
    SmIoReadObjectMetadata *read_entry =  static_cast<SmIoReadObjectMetadata*>(ioReq);
    ObjMetaData objMetadata;

    Error err = smObjDb->get(read_entry->getObjId(), objMetadata);
    if (err != ERR_OK) {
        fds_assert(!"failed to read");
        LOGERROR << "Failed to read object metadata id: " << read_entry->getObjId();
        qosCtrl->markIODone(*read_entry,
                diskio::diskTier);
        omJrnl->release_transaction(read_entry->getTransId());
        read_entry->smio_readmd_resp_cb(err, read_entry);
        return;
    }

    objMetadata.extractSyncData(read_entry->meta_data);

    qosCtrl->markIODone(*read_entry,
            diskio::diskTier);
    omJrnl->release_transaction(read_entry->getTransId());

    read_entry->smio_readmd_resp_cb(err, read_entry);
}

inline void ObjectStorMgr::swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg) {
    FDSP_MgrIdType temp_id;
    fds_int32_t tmp_addr;
    fds_uint32_t tmp_port;

    temp_id = fdsp_msg->dst_id;
    fdsp_msg->dst_id = fdsp_msg->src_id;
    fdsp_msg->src_id = temp_id;

    tmp_addr = fdsp_msg->dst_ip_hi_addr;
    fdsp_msg->dst_ip_hi_addr = fdsp_msg->src_ip_hi_addr;
    fdsp_msg->src_ip_hi_addr = tmp_addr;

    tmp_addr = fdsp_msg->dst_ip_lo_addr;
    fdsp_msg->dst_ip_lo_addr = fdsp_msg->src_ip_lo_addr;
    fdsp_msg->src_ip_lo_addr = tmp_addr;

    tmp_port = fdsp_msg->dst_port;
    fdsp_msg->dst_port = fdsp_msg->src_port;
    fdsp_msg->src_port = tmp_port;

    fdsp_msg->src_service_uuid.uuid =
            (fds_int64_t)(PlatformProcess::plf_manager()->
                          plf_get_my_svc_uuid())->uuid_get_val();
}

// TODO(Sean)
// These three ifaces --  {get,put,del}ObjectExt() may be deprecated.
// Should consider removing them.
void getObjectExt(SmIoReq* getReq) {
    objStorMgr->getObjectInternal(getReq);
}

void putObjectExt(SmIoReq* putReq) {
    objStorMgr->putObjectInternal(putReq);
}

void delObjectExt(SmIoReq* putReq) {
    objStorMgr->deleteObjectInternal(putReq);
}

Error ObjectStorMgr::SmQosCtrl::processIO(FDS_IOType* _io) {
    Error err(ERR_OK);
    SmIoReq *io = static_cast<SmIoReq*>(_io);

    // Boolean to determine which code paths (new vs. legacy) to execute
    bool exec_new_stubs = parentSm->execNewStubs;

    PerfTracer::tracePointEnd(io->opQoSWaitCtx);

    switch (io->io_type) {
        case FDS_IO_READ:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a get request";
            threadPool->schedule(getObjectExt, io);
            break;
        case FDS_IO_WRITE:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";
            threadPool->schedule(putObjectExt, io);
            break;
        case FDS_SM_DELETE_OBJECT:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a Delete request";

            // This is a temporary toggle to execute either a legacy code path
            // or a new code path.
            if (exec_new_stubs == true) {
                threadPool->schedule(&ObjectStorMgr::deleteObjectInternalSvcV2,
                                     objStorMgr,
                                     static_cast<SmIoDeleteObjectReq *>(io));
            } else {
                threadPool->schedule(&ObjectStorMgr::deleteObjectInternalSvc,
                                     objStorMgr,
                                     static_cast<SmIoDeleteObjectReq *>(io));
            }
            break;
        case FDS_SM_GET_OBJECT:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a get request";

            // This is a temporary toggle to execute either a legacy code path
            // or a new code path.
            if (exec_new_stubs == true) {
                threadPool->schedule(&ObjectStorMgr::getObjectInternalSvcV2,
                                     objStorMgr,
                                     static_cast<SmIoGetObjectReq *>(io));
            } else {
                threadPool->schedule(&ObjectStorMgr::getObjectInternalSvc,
                                     objStorMgr,
                                     static_cast<SmIoGetObjectReq *>(io));
            }
            break;
        case FDS_SM_PUT_OBJECT:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";

            // This is a temporary toggle to execute either a legacy code path
            // or a new code path.
            if (exec_new_stubs == true) {
                threadPool->schedule(&ObjectStorMgr::putObjectInternalSvcV2,
                                     objStorMgr,
                                     static_cast<SmIoPutObjectReq *>(io));
            } else {
                threadPool->schedule(&ObjectStorMgr::putObjectInternalSvc,
                                     objStorMgr,
                                     static_cast<SmIoPutObjectReq *>(io));
            }
            break;
        case FDS_SM_ADD_OBJECT_REF:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing and add object reference request";
            threadPool->schedule(&ObjectStorMgr::addObjectRefInternalSvc,
                                 objStorMgr,
                                 static_cast<SmIoAddObjRefReq *>(io));
            break;
        }
        case FDS_SM_WRITE_TOKEN_OBJECTS:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a write token ibjects";
            threadPool->schedule(&ObjectStorMgr::putTokenObjectsInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_TOKEN_OBJECTS:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a read token objects";
            threadPool->schedule(&ObjectStorMgr::getTokenObjectsInternal, objStorMgr, io);
            break;
        case FDS_SM_SNAPSHOT_TOKEN:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing snapshot";
            threadPool->schedule(&ObjectStorMgr::snapshotTokenInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_COMPACT_OBJECTS:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing sync apply metadata";
            threadPool->schedule(&ObjectStorMgr::compactObjectsInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_SYNC_APPLY_METADATA:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing sync apply metadata";
            threadPool->schedule(&ObjectStorMgr::applySyncMetadataInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_SYNC_RESOLVE_SYNC_ENTRY:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing sync resolve";
            threadPool->schedule(&ObjectStorMgr::resolveSyncEntryInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_APPLY_OBJECTDATA:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing sync apply object metadata";
            threadPool->schedule(&ObjectStorMgr::applyObjectDataInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_OBJECTDATA:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing read object data";
            threadPool->schedule(&ObjectStorMgr::readObjectDataInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_OBJECTMETADATA:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing read object metadata";
            threadPool->schedule(&ObjectStorMgr::readObjectMetadataInternal, objStorMgr, io);
            break;
        }
        default:
            fds_assert(!"Unknown message");
            break;
    }

    return err;
}


void log_ocache_stats() {
    /*
     *TODO(Vy): this is kind of bloated stat, the file grows quite big with redudant
     *data, disable it.
     */
#if 0
    while (1) {
        usleep(500000);
        objStorMgr->getObjCache()->log_stats_to_file("ocache_stats.dat");
    }
#endif
}


}  // namespace fds
