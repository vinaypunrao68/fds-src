
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
#include <policy_rpc.h>
#include <policy_tier.h>
#include <StorMgr.h>
#include <NetSession.h>
#include <fds_timestamp.h>
#include <fdsp_utils.h>
#include <net/net_utils.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>

#include "platform/platform.h"

using diskio::DataTier;

namespace fds {

fds_bool_t  stor_mgr_stopping = false;

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
    // Setting shuttingDown will cause any IO going to the QOS queue
    // to return ERR_SM_SHUTTING_DOWN
    LOGDEBUG << " Destructing  the Storage  manager";
    shuttingDown = true;

    // Shutdown scavenger
    SmScavengerCmd *shutdownCmd = new SmScavengerCmd();
    shutdownCmd->command = SmScavengerCmd::SCAV_STOP;
    objectStore->scavengerControlCmd(shutdownCmd);
    LOGDEBUG << "Scavenger is now shut down.";

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
    LOGDEBUG << "Volumes deregistered...";
    /*
     * TODO: Assert that the waiting req map is empty.
     */

    delete qosCtrl;
    LOGDEBUG << "qosCtrl destructed...";
    delete volTbl;
    LOGDEBUG << "volTbl destructed...";

    // Now clean up the persistent layer
    objectStore.reset();
    LOGDEBUG << "Persistent layer has been destructed...";

    // TODO(brian): Make this a cleaner exit
    // Right now it will cause mod_shutdown to be called for each
    // module. When we reach the SM module we bomb out
    delete modProvider_;
}

int
ObjectStorMgr::mod_init(SysParams const *const param) {
    shuttingDown = false;

    // Init  the log infra
    GetLog()->setSeverityFilter(fds_log::getLevelFromName(
        modProvider_->get_fds_config()->get<std::string>("fds.sm.log_severity")));

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

    // init the checksum verification class
    chksumPtr =  new checksum_calc();

    /* Token state db */
    tokenStateDb_.reset(new kvstore::TokenStateDB());
    // TODO(Rao): token state db population should be done based on
    // om events.  For now hardcoding
    for (fds_token_id tok = 0; tok < 256; tok++) {
        tokenStateDb_->addToken(tok);
    }

    testStandalone = modProvider_->get_fds_config()->get<bool>("fds.sm.testing.standalone");
    if (testStandalone == false) {
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

    // create stats dir before creating ObjectStore (that sets up tier engine, etc)
    FdsRootDir::fds_mkdir(modProvider_->proc_fdsroot()->dir_fds_var_stats().c_str());

    // Init the object store
    // TODO(Andrew): The object store should be executed as part
    // of the module initialization since it needs to discover
    // any prior data and possibly perform recovery before allowing
    // another module layer to come up above it.
    objectStore = ObjectStore::unique_ptr(new ObjectStore("SM Object Store Module",
                                                          this,
                                                          volTbl));
    objectStore->mod_init(mod_params);

    // Init token migration manager
    migrationMgr = SmTokenMigrationMgr::unique_ptr(new SmTokenMigrationMgr(this));

    // qos defaults
    qosThrds = modProvider_->get_fds_config()->get<int>(
        "fds.sm.qos.default_qos_threads");

    // the default value is for minimum number; if we have more disks
    // then number of outstanding should be greater than number of disks
    qosOutNum = modProvider_->get_fds_config()->get<int>(
        "fds.sm.qos.default_outstanding_io");
    fds_uint32_t minOutstanding = objectStore->getDiskCount() + 2;
    if (minOutstanding > qosOutNum) {
        qosOutNum = minOutstanding;
    }
    // we should also have enough QoS threads to serve outstanding IO
    if (qosThrds <= qosOutNum) {
        qosThrds = qosOutNum + 1;   // one is used for dispatcher
    }

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

    // create queue for system tasks
    sysTaskQueue = new SmVolQueue(FdsSysTaskQueueId,
                                  256,
                                  getSysTaskIopsMax(),
                                  getSysTaskIopsMin(),
                                  getSysTaskPri());

    qosCtrl->registerVolume(FdsSysTaskQueueId,
                            sysTaskQueue);

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
}

void ObjectStorMgr::mod_shutdown()
{
    LOGDEBUG << "Mod shutdown called on ObjectStorMgr";
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
    if (testStandalone == true) {
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

    if (curDlt->getTokens(objStorMgr->getUuid()).empty()) {
        LOGDEBUG << "Received DLT update that has removed this node.";
        // TODO(brian): Not sure if this is where we should kill scavenger or not
    }
}

void ObjectStorMgr::dltcloseEventHandler(FDSP_DltCloseTypePtr& dlt_close,
        const std::string& session_uuid)
{
    #if 0
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

    // Check to see if we should be shutting down
    const DLT * currentDlt = objStorMgr->getDLT();
    if (currentDlt->getTokens(objStorMgr->getUuid()).empty()) {
        delete objStorMgr;
    }
    #endif
}

void ObjectStorMgr::migrationSvcResponseCb(const Error& err,
        const MigrationStatus& status) {
    if (status == TOKEN_COPY_COMPLETE) {
        LOGNORMAL << "Token copy complete";
        omClient->sendMigrationStatusToOM(err);
    } else if (status == MIGRATION_OP_COMPLETE) {
        LOGNORMAL << "Token migration complete";
        LOGNORMAL << migrationSvc_->mig_cntrs.toString();

        // omClient->sendDLTCloseAckToOM();

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
                if (!err.ok()) {
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

/*------------------------------------------------------------------------- ------------
 * FDSP Protocol internal processing 
 -------------------------------------------------------------------------------------*/

Error
ObjectStorMgr::putObjectInternal(SmIoPutObjectReq *putReq)
{
    Error err(ERR_OK);
    const ObjectID&  objId    = putReq->getObjId();
    fds_volid_t volId         = putReq->getVolId();

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    // latency of ObjectStore layer
    PerfTracer::tracePointBegin(putReq->opLatencyCtx);

    // TODO(Andrew): Remove this copy. The network should allocated
    // a shared ptr structure so that we can directly store that, even
    // after the network message is freed.
    err = objectStore->putObject(volId,
                                 objId,
                                 boost::make_shared<std::string>(
                                     putReq->putObjectNetReq->data_obj));
    qosCtrl->markIODone(*putReq);

    // end of ObjectStore layer latency
    PerfTracer::tracePointEnd(putReq->opLatencyCtx);

    putReq->response_cb(err, putReq);
    return err;
}

Error
ObjectStorMgr::deleteObjectInternal(SmIoDeleteObjectReq* delReq)
{
    Error err(ERR_OK);
    const ObjectID&  objId    = delReq->getObjId();
    fds_volid_t volId         = delReq->getVolId();

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    LOGDEBUG << "Volume " << std::hex << volId << std::dec
             << " " << objId;

    // start of ObjectStore layer latency
    PerfTracer::tracePointBegin(delReq->opLatencyCtx);

    err = objectStore->deleteObject(volId, objId);

    qosCtrl->markIODone(*delReq);

    // end of ObjectStore layer latency
    PerfTracer::tracePointEnd(delReq->opLatencyCtx);

    delReq->response_cb(err, delReq);
    return err;
}

Error
ObjectStorMgr::addObjectRefInternal(SmIoAddObjRefReq* addObjRefReq) {
    fds_assert(0 != addObjRefReq);
    fds_assert(0 != addObjRefReq->getSrcVolId());
    fds_assert(0 != addObjRefReq->getDestVolId());

    Error rc = ERR_OK;

    if (addObjRefReq->objIds().empty()) {
        return rc;
    }

    // start of ObjectStore layer latency
    PerfTracer::tracePointBegin(addObjRefReq->opLatencyCtx);

    for (auto objId : addObjRefReq->objIds()) {
        ObjectID oid = ObjectID(objId.digest);
        rc = objectStore->copyAssociation(addObjRefReq->getSrcVolId(),
                addObjRefReq->getDestVolId(), oid);
        if (!rc.ok()) {
            LOGERROR << "Failed to add association entry for object " << oid
                     << "in to odb with err " << rc;
            break;
        }
    }

    qosCtrl->markIODone(*addObjRefReq);
    // end of ObjectStore layer latency
    PerfTracer::tracePointEnd(addObjRefReq->opLatencyCtx);

    addObjRefReq->response_cb(rc, addObjRefReq);

    return rc;
}

Error
ObjectStorMgr::getObjectInternal(SmIoGetObjectReq *getReq)
{
    Error err(ERR_OK);
    const ObjectID&  objId    = getReq->getObjId();
    fds_volid_t volId         = getReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;

    fds_assert(volId != 0);
    fds_assert(objId != NullObjectID);

    LOGDEBUG << "volume " << std::hex << volId << std::dec
             << " " << objId;

    // start of ObjectStore layer latency
    PerfTracer::tracePointBegin(getReq->opLatencyCtx);

    boost::shared_ptr<const std::string> objData =
            objectStore->getObject(volId,
                                   objId,
                                   tierUsed,
                                   err);
    if (err.ok()) {
        // TODO(Andrew): Remove this copy. The network should allocated
        // a shared ptr structure so that we can directly store that, even
        // after the network message is freed.
        getReq->getObjectNetResp->data_obj = *objData;
    }

    qosCtrl->markIODone(*getReq, tierUsed, amIPrimary(objId));

    // end of ObjectStore layer latency
    PerfTracer::tracePointEnd(getReq->opLatencyCtx);

    getReq->response_cb(err, getReq);
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
    ioReq->setVolId(volId);

    // TODO(brian): Move this elsewhere ?
    // Return ERR_SM_SHUTTING_DOWN here if SM is in shutdown state
    if (isShuttingDown()) {
        LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
        return ERR_SM_SHUTTING_DOWN;
    }

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
        case FDS_SM_TIER_WRITEBACK_OBJECTS:
        case FDS_SM_TIER_PROMOTE_OBJECTS:
        case FDS_SM_APPLY_DELTA_SET:
        case FDS_SM_SNAPSHOT_TOKEN:
        {
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        }
        case FDS_SM_GET_OBJECT:
            {
            StorMgrVolume* smVol = volTbl->getVolume(ioReq->getVolId());
            fds_assert(smVol);
            err = qosCtrl->enqueueIO(smVol->getQueue()->getVolUuid(),
                                     static_cast<FDS_IOType*>(ioReq));
            break;
            }
        case FDS_SM_PUT_OBJECT:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_ADD_OBJECT_REF:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_DELETE_OBJECT:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_SYNC_APPLY_METADATA:
            objectId.SetId(static_cast<SmIoApplySyncMetadata*>(ioReq)->md.object_id.digest);
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_SYNC_RESOLVE_SYNC_ENTRY:
            objectId = static_cast<SmIoResolveSyncEntry*>(ioReq)->object_id;
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_APPLY_OBJECTDATA:
            objectId = static_cast<SmIoApplyObjectdata*>(ioReq)->obj_id;
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_READ_OBJECTDATA:
            objectId = static_cast<SmIoReadObjectdata*>(ioReq)->getObjId();
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        case FDS_SM_READ_OBJECTMETADATA:
            objectId = static_cast<SmIoReadObjectMetadata*>(ioReq)->getObjId();
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
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
    fds_panic("need to be ported to new SM org");

    for (auto obj : objList) {
        ObjectID objId(obj.meta_data.object_id.digest);

        ObjMetaData objMetadata;
        // err = smObjDb->get(objId, objMetadata);
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
            // smObjDb->put(OpCtx(OpCtx::COPY), objId, objMetadata);
            /* No need to write the object data.  It already exits */
            continue;
        }

        /* Moving the data to not incur copy penalty */
        DBG(std::string temp_data = obj.data);
        ObjectBuf objData;
        *(objData.data) = std::move(obj.data);
        fds_assert(temp_data == *(objData.data));
        obj.data.clear();

        /* write data to storage tier */
        obj_phy_loc_t phys_loc;
        fds_panic("need to port back to new SM org");
        // err = writeObjectDataToTier(objId, objData, diskio::diskTier, phys_loc);
        if (err != ERR_OK) {
            LOGERROR << "Failed to write the object: " << objId;
            break;
        }

        /* write the metadata */
        objMetadata.updatePhysLocation(&phys_loc);
        // smObjDb->put(OpCtx(OpCtx::COPY), objId, objMetadata);
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
    fds_panic("need to be ported to new SM org");
    SmIoGetTokObjectsReq *getTokReq = static_cast<SmIoGetTokObjectsReq*>(ioReq);
    // smObjDb->iterRetrieveObjects(getTokReq->token_id,
    //                             getTokReq->max_size, getTokReq->obj_list, getTokReq->itr);

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

    objectStore->snapshotMetadata(snapReq->token_id,
                                  snapReq->smio_snap_resp_cb);

    /* Mark the request as complete */
    qosCtrl->markIODone(*snapReq,
                        diskio::diskTier);
}

void
ObjectStorMgr::applySyncMetadataInternal(SmIoReq* ioReq)
{
    fds_panic("need to be ported to new SM org");
    SmIoApplySyncMetadata *applyMdReq =  static_cast<SmIoApplySyncMetadata*>(ioReq);
    // Error e = smObjDb->putSyncEntry(ObjectID(applyMdReq->md.object_id.digest),
    //                                applyMdReq->md, applyMdReq->dataExists);
    Error e(ERR_OK);
    if (e != ERR_OK) {
        fds_assert(!"error");
        LOGERROR << "Error in applying sync metadata.  Object Id: "
                 << applyMdReq->md.object_id.digest;
    }
    /* Mark the request as complete */
    qosCtrl->markIODone(*applyMdReq,
            diskio::diskTier);

    applyMdReq->smio_sync_md_resp_cb(e, applyMdReq);
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
                 << " on tier " << cobjs_req->tier << " verify data?"
                 << cobjs_req->verifyData;

        // copy this object if not garbage, otherwise rm object db entry
        err = objectStore->copyObjectToNewLocation(obj_id, cobjs_req->tier,
                                                   cobjs_req->verifyData);
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
ObjectStorMgr::applyRebalanceDeltaSet(SmIoReq* ioReq)
{
    Error err(ERR_OK);
    SmIoApplyObjRebalDeltaSet* rebalReq = static_cast<SmIoApplyObjRebalDeltaSet*>(ioReq);
    fds_assert(rebalReq != NULL);

    for (fds_uint32_t i = 0; i < (rebalReq->deltaSet).size(); ++i) {
        const fpi::CtrlObjectMetaDataPropagate& objDataMeta = (rebalReq->deltaSet)[i];
        ObjectID objId(ObjectID(objDataMeta.objectID.digest));

        err = objectStore->applyObjectMetadataData(objId, objDataMeta);
        if (!err.ok()) {
            // we will stop applying object metadata/data and report error to migr mgr
            LOGERROR << "Failed to apply object metadata/data " << objId
                     << ", " << err;
            break;
        }
    }

    // mark request as complete
    qosCtrl->markIODone(*rebalReq, diskio::diskTier);

    // notify migration executor we are done with this request
    rebalReq->smioObjdeltaRespCb(err, rebalReq);

    delete rebalReq;
}

void
ObjectStorMgr::moveTierObjectsInternal(SmIoReq* ioReq) {
    Error err(ERR_OK);
    SmIoMoveObjsToTier *moveReq = static_cast<SmIoMoveObjsToTier*>(ioReq);
    fds_verify(moveReq != NULL);

    LOGDEBUG << "Will move " << (moveReq->oidList).size() << " objs from tier "
             << moveReq->fromTier << " to tier " << moveReq->fromTier
             << " relocate? " << moveReq->relocate;

    for (fds_uint32_t i = 0; i < (moveReq->oidList).size(); ++i) {
        const ObjectID& objId = (moveReq->oidList)[i];
        err = objectStore->moveObjectToTier(objId, moveReq->fromTier,
                                            moveReq->toTier, moveReq->relocate);
        if (!err.ok()) {
            LOGERROR << "Failed to move " << objId << " from tier "
                     << moveReq->fromTier << " to tier " << moveReq->fromTier
                     << " relocate? " << moveReq->relocate << " " << err;
            // we will just continue to move other objects; ok for promotions
            // demotion should not assume that object was written back to HDD
            // anyway, because writeback is eventual
        }
    }

    /* Mark the request as complete */
    qosCtrl->markIODone(*moveReq);

    if (moveReq->moveObjsRespCb) {
        moveReq->moveObjsRespCb(err, moveReq);
    }
    delete moveReq;
}

void
ObjectStorMgr::resolveSyncEntryInternal(SmIoReq* ioReq)
{
    fds_panic("need to be ported to new SM org");
    SmIoResolveSyncEntry *resolve_entry =  static_cast<SmIoResolveSyncEntry*>(ioReq);
    Error e(ERR_OK);  //  = smObjDb->resolveEntry(resolve_entry->object_id);
    if (e != ERR_OK) {
        fds_assert(!"error");
        LOGERROR << "Error in resolving metadata entry.  Object Id: "
                << resolve_entry->object_id;
    }
    /* Mark the request as complete */
    qosCtrl->markIODone(*resolve_entry,
            diskio::diskTier);

    resolve_entry->smio_resolve_resp_cb(e, resolve_entry);
}

void
ObjectStorMgr::applyObjectDataInternal(SmIoReq* ioReq)
{
    SmIoApplyObjectdata *applydata_entry =  static_cast<SmIoApplyObjectdata*>(ioReq);
    ObjectID objId = applydata_entry->obj_id;
    ObjMetaData objMetadata;
    fds_panic("need to be ported to new SM org");
    Error err(ERR_OK);  //  = smObjDb->get(objId, objMetadata);
    if (err != ERR_OK && err != ERR_NOT_FOUND) {
        fds_assert(!"ObjMetadata read failed");
        LOGERROR << "Failed to write the object: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                diskio::diskTier);
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    err = ERR_OK;

    if (objMetadata.dataPhysicallyExists()) {
        /* No need to write the object data.  It already exits */
        LOGDEBUG << "Object already exists. Id: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                diskio::diskTier);
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    /* Moving the data to not incur copy penalty */
    DBG(std::string temp_data = applydata_entry->obj_data);
    ObjectBuf objData;
    *(objData.data) = std::move(applydata_entry->obj_data);
    fds_assert(temp_data == *(objData.data));
    applydata_entry->obj_data.clear();

    /* write data to storage tier */
    obj_phy_loc_t phys_loc;
    fds_panic("need to port back to new SM org");
    // err = writeObjectDataToTier(objId, objData, diskio::diskTier, phys_loc);
    if (err != ERR_OK) {
        fds_assert(!"Failed to write");
        LOGERROR << "Failed to write the object: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                diskio::diskTier);
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    /* write the metadata */
    objMetadata.updatePhysLocation(&phys_loc);
    // smObjDb->put(OpCtx(OpCtx::SYNC), objId, objMetadata);

    qosCtrl->markIODone(*applydata_entry,
            diskio::diskTier);

    applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
}

void
ObjectStorMgr::readObjectDataInternal(SmIoReq* ioReq)
{
    SmIoReadObjectdata *read_entry =  static_cast<SmIoReadObjectdata*>(ioReq);
    ObjectBuf        objData;
    ObjMetaData objMetadata;
    diskio::DataTier tierUsed;

    /*
    Error err = readObject(SmObjDb::NON_SYNC_MERGED, read_entry->getObjId(),
                           objMetadata, objData, tierUsed, 0);
    */
    Error err(ERR_OK);
    fds_panic("This method should not be called");
    if (err != ERR_OK) {
        fds_assert(!"failed to read");
        LOGERROR << "Failed to read object id: " << read_entry->getObjId();
        qosCtrl->markIODone(*read_entry,
                diskio::diskTier);
        read_entry->smio_readdata_resp_cb(err, read_entry);
    }
    // TODO(Rao): use std::move here
    read_entry->obj_data.data = *(objData.data);
    fds_assert(read_entry->obj_data.data.size() > 0);

    qosCtrl->markIODone(*read_entry,
            diskio::diskTier);

    read_entry->smio_readdata_resp_cb(err, read_entry);
}

void
ObjectStorMgr::readObjectMetadataInternal(SmIoReq* ioReq)
{
    SmIoReadObjectMetadata *read_entry =  static_cast<SmIoReadObjectMetadata*>(ioReq);
    ObjMetaData objMetadata;

    fds_panic("need to be ported to new SM org");
    Error err(ERR_OK);  //  = smObjDb->get(read_entry->getObjId(), objMetadata);
    if (err != ERR_OK) {
        fds_assert(!"failed to read");
        LOGERROR << "Failed to read object metadata id: " << read_entry->getObjId();
        qosCtrl->markIODone(*read_entry,
                diskio::diskTier);
        read_entry->smio_readmd_resp_cb(err, read_entry);
        return;
    }

    objMetadata.extractSyncData(read_entry->meta_data);

    qosCtrl->markIODone(*read_entry,
            diskio::diskTier);

    read_entry->smio_readmd_resp_cb(err, read_entry);
}

Error ObjectStorMgr::SmQosCtrl::processIO(FDS_IOType* _io) {
    Error err(ERR_OK);
    SmIoReq *io = static_cast<SmIoReq*>(_io);

    PerfTracer::tracePointEnd(io->opQoSWaitCtx);

    switch (io->io_type) {
        case FDS_IO_READ:
        case FDS_IO_WRITE:
            fds_panic("must not get here!");
            break;
        case FDS_SM_DELETE_OBJECT:
            LOGDEBUG << "Processing a Delete request";
                threadPool->schedule(&ObjectStorMgr::deleteObjectInternal,
                                     objStorMgr,
                                     static_cast<SmIoDeleteObjectReq *>(io));
            break;
        case FDS_SM_GET_OBJECT:
            LOGDEBUG << "Processing a get request";
            threadPool->schedule(&ObjectStorMgr::getObjectInternal,
                                 objStorMgr,
                                 static_cast<SmIoGetObjectReq *>(io));
            break;
        case FDS_SM_PUT_OBJECT:
            LOGDEBUG << "Processing a put request";
            threadPool->schedule(&ObjectStorMgr::putObjectInternal,
                                 objStorMgr,
                                 static_cast<SmIoPutObjectReq *>(io));
            break;
        case FDS_SM_ADD_OBJECT_REF:
        {
            LOGDEBUG << "Processing and add object reference request";
            threadPool->schedule(&ObjectStorMgr::addObjectRefInternal,
                                 objStorMgr,
                                 static_cast<SmIoAddObjRefReq *>(io));
            break;
        }
        case FDS_SM_WRITE_TOKEN_OBJECTS:
        {
            LOGDEBUG << "Processing a write token ibjects";
            threadPool->schedule(&ObjectStorMgr::putTokenObjectsInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_TOKEN_OBJECTS:
            LOGDEBUG << "Processing a read token objects";
            threadPool->schedule(&ObjectStorMgr::getTokenObjectsInternal, objStorMgr, io);
            break;
        case FDS_SM_SNAPSHOT_TOKEN:
        {
            LOGDEBUG << "Processing snapshot";
            threadPool->schedule(&ObjectStorMgr::snapshotTokenInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_COMPACT_OBJECTS:
        {
            LOGDEBUG << "Processing sync apply metadata";
            threadPool->schedule(&ObjectStorMgr::compactObjectsInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_TIER_WRITEBACK_OBJECTS:
        case FDS_SM_TIER_PROMOTE_OBJECTS:
            threadPool->schedule(&ObjectStorMgr::moveTierObjectsInternal, objStorMgr, io);
            break;
        case FDS_SM_APPLY_DELTA_SET:
            threadPool->schedule(&ObjectStorMgr::applyRebalanceDeltaSet, objStorMgr, io);
            break;
        case FDS_SM_SYNC_APPLY_METADATA:
        {
            LOGDEBUG << "Processing sync apply metadata";
            threadPool->schedule(&ObjectStorMgr::applySyncMetadataInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_SYNC_RESOLVE_SYNC_ENTRY:
        {
            LOGDEBUG << "Processing sync resolve";
            threadPool->schedule(&ObjectStorMgr::resolveSyncEntryInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_APPLY_OBJECTDATA:
        {
            LOGDEBUG << "Processing sync apply object metadata";
            threadPool->schedule(&ObjectStorMgr::applyObjectDataInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_OBJECTDATA:
        {
            LOGDEBUG << "Processing read object data";
            threadPool->schedule(&ObjectStorMgr::readObjectDataInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_OBJECTMETADATA:
        {
            LOGDEBUG << "Processing read object metadata";
            threadPool->schedule(&ObjectStorMgr::readObjectMetadataInternal, objStorMgr, io);
            break;
        }
        default:
            fds_assert(!"Unknown message");
            break;
    }

    return err;
}


}  // namespace fds
