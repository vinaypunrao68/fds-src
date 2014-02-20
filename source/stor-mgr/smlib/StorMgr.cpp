/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream>
#include <thread>
#include <functional>

#include "SmObjDb.h"
#include <policy_rpc.h>
#include <policy_tier.h>
#include "StorMgr.h"
#include "fds_obj_cache.h"

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
    FDS_PLOG(objStorMgr->GetLog()) << "Received a Putobject() network request";

#ifdef FDS_TEST_SM_NOOP
    msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
    msgHdr->result = FDSP_ERR_OK;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
    FDS_PLOG(objStorMgr->GetLog()) << "FDS_TEST_SM_NOOP defined. Sent async PutObj response right after receiving req.";
    return;
#endif /* FDS_TEST_SM_NOOP */


#if 0
     // check the payload checksum  and return Error, if we run in to issues 
    std:string new_checksum,Received_checksum;

    Received_checksum = msgHdr->payload_chksum;
    objStorMgr->chksumPtr->checksum_update(reinterpret_cast<unsigned char *>(putObj.get()), sizeof(*(putObj.get())));
    objStorMgr->chksumPtr->checksum_update(reinterpret_cast<unsigned char *>(const_cast <char *>(putObj->data_obj.data())), putObj->data_obj_len);
    objStorMgr->chksumPtr->get_checksum(new_checksum);
    FDS_PLOG(objStorMgr->GetLog()) << "RPC Checksum :" << new_checksum << " received checksum: " << Received_checksum; 
    if (msgHdr->payload_chksum.compare(new_checksum) != 0) {
	msgHdr->result = FDSP_ERR_CKSUM_MISMATCH; 			
	msgHdr->err_code = FDSP_ERR_RPC_CKSUM; 			
        FDS_PLOG(objStorMgr->GetLog()) << "RPC Checksum Error "; 

        /*
        msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
        objStorMgr->swapMgrId(msgHdr);
        objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
        */

        FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response after receiving";
    }
#endif

    /*
     * Track the outstanding get request.
     */

    if (putObj->dlt_version == objStorMgr->omClient->getDltVersion()) {
    /*
     * Track the outstanding get request.
     */
      objStorMgr->PutObject(msgHdr, putObj);
    } else {
	msgHdr->err_code = FDSP_ERR_DLT_CONFLICT; 			
	msgHdr->result = FDSP_ERR_DLT_MISMATCH; 			
	// send the dlt version of SM to AM 
        putObj->dlt_version = objStorMgr->omClient->getDltVersion();
        // update the resp  with new DLT
        objStorMgr->omClient->getLatestDlt(putObj->dlt_data);
        FDS_PLOG(objStorMgr->GetLog()) << "DLT  version Conflict returning the latest"; 
    }

    /*
     * If we failed to enqueue the I/O return the error response
     * now as there is no more processing to do.
     */
    if (msgHdr->result != FDSP_ERR_OK) {
        msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
        objStorMgr->swapMgrId(msgHdr);
        objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);

        FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response after receiving";
    }
}

void
ObjectStorMgrI::GetObject(FDSP_MsgHdrTypePtr& msgHdr,
        FDSP_GetObjTypePtr& getObj)
{
    FDS_PLOG(objStorMgr->GetLog()) << "Received a Getobject() network request";

#ifdef FDS_TEST_SM_NOOP
    msgHdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
    msgHdr->result = FDSP_ERR_OK;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->GetObjectResp(msgHdr, getObj);
    FDS_PLOG(objStorMgr->GetLog()) << "FDS_TEST_SM_NOOP defined. Sent async GetObj response right after receiving req.";
    return;
#endif /* FDS_TEST_SM_NOOP */

    /*
     * Track the outstanding get request.

    /*
     * Submit the request to be enqueued
     */
    objStorMgr->GetObject(msgHdr, getObj);
    /*
     * If we failed to enqueue the I/O return the error response
     * now as there is no more processing to do.
     */
    if (msgHdr->result != FDSP_ERR_OK) {

        msgHdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
        if(getObj->dlt_version != objStorMgr->omClient->getDltVersion()) {
            msgHdr->result = FDSP_ERR_DLT_MISMATCH;
	   msgHdr->err_code = FDSP_ERR_DLT_CONFLICT;
	  // send the dlt version of SM to AM
           getObj->dlt_version = objStorMgr->omClient->getDltVersion();
        // update the resp  with new DLT
        objStorMgr->omClient->getLatestDlt(getObj->dlt_data);
	}
	
        objStorMgr->swapMgrId(msgHdr);
        objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->GetObjectResp(msgHdr, getObj);

        FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response after receiving";
    }
}

void
ObjectStorMgrI::DeleteObject(FDSP_MsgHdrTypePtr& msgHdr,
        FDSP_DeleteObjTypePtr& delObj)
{
    FDS_PLOG(objStorMgr->GetLog()) << "Received a Deleteobject() network request";

#ifdef FDS_TEST_SM_NOOP
    msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
    msgHdr->result = FDSP_ERR_OK;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->DeleteObjectResp(msgHdr, delObj);
    FDS_PLOG(objStorMgr->GetLog()) << "FDS_TEST_SM_NOOP defined. Sent async DeleteObj response right after receiving req.";
    return;
#endif /* FDS_TEST_SM_NOOP */

    /*
     * Track the outstanding get request.
     */
    if (delObj->dlt_version == objStorMgr->omClient->getDltVersion()) {

        objStorMgr->DeleteObject(msgHdr, delObj);
    } else {
	msgHdr->err_code = FDSP_ERR_DLT_CONFLICT; 			
	msgHdr->result = FDSP_ERR_DLT_MISMATCH; 			
	// send the dlt version of SM to AM 
        delObj->dlt_version = objStorMgr->omClient->getDltVersion();
        // update the resp  with new DLT
        objStorMgr->omClient->getLatestDlt(delObj->dlt_data);
    }

    /*
     * If we failed to enqueue the I/O return the error response
     * now as there is no more processing to do.
     */
    if (msgHdr->result != FDSP_ERR_OK) {

        msgHdr->msg_code = FDSP_MSG_DELETE_OBJ_RSP;
        objStorMgr->swapMgrId(msgHdr);
        objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->DeleteObjectResp(msgHdr, delObj);

        FDS_PLOG(objStorMgr->GetLog()) << "Sent async DeleteObj response after receiving";
    }
}

void
ObjectStorMgrI::OffsetWriteObject(FDSP_MsgHdrTypePtr& msg_hdr, FDSP_OffsetWriteObjTypePtr& offset_write_obj) {
  FDS_PLOG(objStorMgr->GetLog()) << "In the interface offsetwrite()";
}

void
ObjectStorMgrI::RedirReadObject(FDSP_MsgHdrTypePtr &msg_hdr, FDSP_RedirReadObjTypePtr& redir_read_obj) {
  FDS_PLOG(objStorMgr->GetLog()) << "In the interface redirread()";
}

/**
 * Storage manager member functions
 * 
 * TODO: The number of test vols, the
 * totalRate, and number of qos threads
 * are being hard coded in the initializer
 * list below.
 */
ObjectStorMgr::ObjectStorMgr(int argc, char *argv[], 
                             const std::string &default_config_path,
                             const std::string &base_path) 
    : Module("StorMgr"),
    FdsProcess(argc, argv, default_config_path, base_path),
    totalRate(2000),
    qosThrds(10),
    shuttingDown(false),
    numWBThreads(1),
    maxDirtyObjs(10000),
    counters_("SM", cntrs_mgrPtr_.get())
{
    /*
     * TODO: Fix the totalRate above to not
     * be hard coded.
     */

    // Init  the log infra
    sm_log = g_fdslog;
    sm_log->setSeverityFilter((fds_log::severity_level) conf_helper_.get<int>("log_severity"));
    FDS_PLOG(sm_log) << "Constructing the Object Storage Manager";
    objStorMutex = new fds_mutex("Object Store Mutex");

    /*
     * Will setup OM comm during run()
     */
    omClient = NULL;

    /*
     * Setup the tier related members
     */
    dirtyFlashObjs = new ObjQueue(maxDirtyObjs);
    fds_verify(dirtyFlashObjs->is_lock_free() == true);
    writeBackThreads = new fds_threadpool(numWBThreads);


    /*
     * Setup QoS related members.
     */
    qosCtrl = new SmQosCtrl(this,
            qosThrds,
            FDS_QoSControl::FDS_DISPATCH_WFQ,
            sm_log);
    qosCtrl->runScheduler();

    /* Set up the journal */
    omJrnl = new TransJournal<ObjectID, ObjectIdJrnlEntry>(qosCtrl, sm_log);
    /*
     * stats class init
     */
    objStats =  new ObjStatsTracker(sm_log);

    /*
     * Performance stats recording
     * TODO: This is *not* a unique name, so
     * multiple SMs will clobber each other.
     * The prefix isn't parsed until later.
     * We should fix that.
     */
    Error err;
    perfStats = new PerfStats("migratorSmStats");
    err = perfStats->enable();
    fds_verify(err == ERR_OK);
}

ObjectStorMgr::~ObjectStorMgr() {
    FDS_PLOG(objStorMgr->GetLog()) << " Destructing  the Storage  manager";
    shuttingDown = true;

    delete smObjDb;
    /*
     * Clean up the QoS system. Need to wait for I/Os to
     * complete and deregister each volume. The volume info
     * is freed when the table is deleted.
     * TODO: We should prevent further volume registration and
     * accepting network I/Os while shutting down.
     */
    std::list<fds_volid_t> volIds = volTbl->getVolList();
    for (std::list<fds_volid_t>::iterator vit = volIds.begin();
            vit != volIds.end();
            vit++) {
        qosCtrl->quieseceIOs((*vit));
        qosCtrl->deregisterVolume((*vit));
    }

    delete perfStats;

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

int ObjectStorMgr::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void ObjectStorMgr::setup(int argc, char *argv[], fds::Module **mod_vec)
{
    /*
     * Invoke FdsProcess setup so that it can setup the signal hander and
     * execute the module vector for us
     */
    FdsProcess::setup(argc, argv, mod_vec);

    /* Rest of the setup */
    // todo: clean up the code below.  It's doing too many things here.
    // Refactor into functions or make it part of module vector

    std::string     myIp;
    DmDiskInfo     *info;
    DmDiskQuery     in;
    DmDiskQueryOut  out;

    std::string stor_prefix = conf_helper_.get<std::string>("prefix");

    // Create leveldb
    smObjDb = new  SmObjDb(stor_prefix, objStorMgr->GetLog());
    // init the checksum verification class
    chksumPtr =  new checksum_calc();

    /* Set up FDSP RPC endpoints */
    nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));
    myIp = netSession::getLocalIp();
    setup_datapath_server(myIp);

    /*
     * Query persistent layer for disk parameter details
     */
    fds::DmQuery &query = fds::DmQuery::dm_query();
    in.dmq_mask = fds::dmq_disk_info;
    query.dm_disk_query(in, &out);
    /* we should be bundling multiple disk parameters into one message to OM TBD */
    FDSP_AnnounceDiskCapabilityPtr dInfo(new FDSP_AnnounceDiskCapability());
    while (1) {
        info = out.query_pop();
        if (info != nullptr) {
            FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Max blks capacity: " << info->di_max_blks_cap
                    << ", Disk type........: " << info->di_disk_type
                    << ", Max iops.........: " << info->di_max_iops
                    << ", Min iops.........: " << info->di_min_iops
                    << ", Max latency (us).: " << info->di_max_latency
                    << ", Min latency (us).: " << info->di_min_latency;

            if ( info->di_disk_type == FDS_DISK_SATA) {
                dInfo->disk_iops_max =  info->di_max_iops; /*  max avarage IOPS */
                dInfo->disk_iops_min =  info->di_min_iops; /* min avarage IOPS */
                dInfo->disk_capacity = info->di_max_blks_cap;  /* size in blocks */
                dInfo->disk_latency_max = info->di_max_latency; /* in us second */
                dInfo->disk_latency_min = info->di_min_latency; /* in us second */
            } else if (info->di_disk_type == FDS_DISK_SSD) {
                dInfo->ssd_iops_max =  info->di_max_iops; /*  max avarage IOPS */
                dInfo->ssd_iops_min =  info->di_min_iops; /* min avarage IOPS */
                dInfo->ssd_capacity = info->di_max_blks_cap;  /* size in blocks */
                dInfo->ssd_latency_max = info->di_max_latency; /* in us second */
                dInfo->ssd_latency_min = info->di_min_latency; /* in us second */
            } else
                FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::warning) << "Unknown Disk Type " << info->di_disk_type;

            delete info;
            continue;
        }
        break;
    }

    /*
     * Register this node with OM.
     */
    omClient = new OMgrClient(FDSP_STOR_MGR,
                              conf_helper_.get<std::string>("om_ip"),
                              conf_helper_.get<int>("om_port"),
                              myIp,
                              conf_helper_.get<int>("data_port"),
                              stor_prefix + "localhost-sm",
                              sm_log,
                              nst_,
                              conf_helper_.get<int>("migration.port"));

    /*
     * Create local volume table. Create after omClient
     * is initialized, because it needs to register with the
     * omClient. Create before register with OM because
     * the OM vol event receivers depend on this table.
     */
    volTbl = new StorMgrVolumeTable(this, sm_log);

    /* Create tier related classes -- has to be after volTbl is created */
    rankEngine = new ObjectRankEngine(stor_prefix, 100000, volTbl, objStats, objStorMgr->GetLog());
    tierEngine = new TierEngine(TierEngine::FDS_TIER_PUT_ALGO_BASIC_RANK, volTbl, rankEngine, objStorMgr->GetLog());
    objCache = new FdsObjectCache(1024 * 1024 * 256,
            slab_allocator_type_default,
            eviction_policy_type_default,
            objStorMgr->GetLog());

    // TODO: join this thread
    std::thread *stats_thread = new std::thread(log_ocache_stats);
    

    // Create a special queue for System (background) tasks
    // and registe rwith QosCtrlr
    sysTaskQueue = new SmVolQueue(FdsSysTaskQueueId,
                                  50,
                                  getSysTaskIopsMax(),
                                  getSysTaskIopsMin(),
                                  getSysTaskPri());

    qosCtrl->registerVolume(FdsSysTaskQueueId,
                            sysTaskQueue);

    /*
     * Register/boostrap from OM
     */
    omClient->initialize();
    omClient->registerEventHandlerForNodeEvents((node_event_handler_t)nodeEventOmHandler);
    omClient->registerEventHandlerForVolEvents((volume_event_handler_t)volEventOmHandler);
    omClient->registerEventHandlerForMigrateEvents((migration_event_handler_t)migrationEventOmHandler);
    omClient->omc_srv_pol = &sg_SMVolPolicyServ;
    omClient->startAcceptingControlMessages(conf_helper_.get<int>("control_port"));
    omClient->registerNodeWithOM(dInfo);

    clust_comm_mgr_.reset(new ClusterCommMgr(omClient));

    /*
     * Create local variables for test mode
     */
    if (conf_helper_.get<bool>("test_mode") == true) {
        /*
         * Create test volumes.
         */
        VolumeDesc*  testVdb;
        std::string testVolName;
        int numTestVols = conf_helper_.get<int>("test_volume_cnt");
        for (fds_uint32_t testVolId = 1; testVolId < numTestVols + 1; testVolId++) {
            testVolName = "testVol" + std::to_string(testVolId);
            /*
             * We're using the ID as the min/max/priority
             * for the volume QoS.
             */
            testVdb = new VolumeDesc(testVolName,
                    testVolId,
                    8+ testVolId,
                    10000,       /* high max iops so that unit tests does not take forever to finish */
                    testVolId);
            fds_assert(testVdb != NULL);
            if ( (testVolId % 3) == 0)
                testVdb->volType = FDSP_VOL_BLKDEV_DISK_TYPE;
            else if ( (testVolId % 3) == 1)
                testVdb->volType = FDSP_VOL_BLKDEV_SSD_TYPE;
            else
                testVdb->volType = FDSP_VOL_BLKDEV_HYBRID_TYPE;

            volEventOmHandler(testVolId,
                    testVdb,
                    FDS_VOL_ACTION_CREATE,
                    FDS_ProtocolInterface::FDSP_ERR_OK);

            delete testVdb;
        }
    }

    /*
     * Kick off the writeback thread(s)
     */
    writeBackThreads->schedule(writeBackFunc, this);

    setup_migration_svc();
}

void ObjectStorMgr::setup_datapath_server(const std::string &ip)
{
    ObjectStorMgrI *osmi = new ObjectStorMgrI(); 
    datapath_handler_.reset(osmi);

    int myIpInt = netSession::ipString2Addr(ip);
    std::string node_name = 
        conf_helper_.get<std::string>("prefix") + "_SM";
    // TODO: Ideally createServerSession should take a shared pointer
    // for datapath_handler.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    // TODO: Figure out who cleans up datapath_session_
    datapath_session_ = nst_->createServerSession<netDataPathServerSession>(
        myIpInt,
        conf_helper_.get<int>("data_port"),
        node_name,
        FDSP_STOR_HVISOR,
        datapath_handler_);
}

void ObjectStorMgr::setup_migration_svc()
{
    migrationSvc_.reset(new FdsMigrationSvc(this,
                FdsConfigAccessor(conf_helper_.get_fds_config(),
                        conf_helper_.get_base_path() + "migration."),
                GetLog(),
                nst_,
                clust_comm_mgr_));
    migrationSvc_->mod_startup();
}

void ObjectStorMgr::run()
{
    nst_->listenServer(datapath_session_);
}

void ObjectStorMgr::interrupt_cb(int signum)
{
    migrationSvc_->mod_shutdown();
    nst_->endAllSessions();
    nst_.reset(); 
    exit(0);
}

void ObjectStorMgr::mod_startup() {    
}

void ObjectStorMgr::mod_shutdown() {
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

void ObjectStorMgr::migrationEventOmHandler(bool dlt_type)
{
    FDS_PLOG(objStorMgr->GetLog()) << "ObjectStorMgr - Migration  event Handler " << dlt_type;

#if 0
    // Determine our new tokens that we need to retrieve from
    // by comparing with the previous DLT
    // TODO(Andrew): For now, we're just getting all of the primary
    // tokens in the current DLT
    MigSvcCopyTokensReqPtr copy_req(new MigSvcCopyTokensReq());
    TokenList tokens;
    // const TokenList &tokens = objStorMgr->getTokensForNode(
    // objStorMgr->getUuid());
    objStorMgr->getTokensForNode(&tokens,
                                 objStorMgr->getUuid(),
                                 0);
    for (TokenList::const_iterator it = tokens.cbegin();
         it != tokens.cend();
         it++) {
        copy_req->tokens.insert(*it);
    }

    // TODO(Andrew): For now, we're assuming if the list of
    // new tokens all of the tokens, then we must be the first
    // SM entering the system and don't need to migration anything
    // because nothing exists to migrate and no one to migrate from.
    // Note: Since the above token list is *NOT* do a delta yet,
    // this will skip migration scenarios where I'm the only node
    // let in the cluster.
    if (copy_req->tokens.size() < objStorMgr->getTotalNumTokens()) {
        // Send migration request to migration service
        copy_req->migsvc_resp_cb = std::bind(
            &ObjectStorMgr::migrationSvcResponseCb,
            objStorMgr,
            std::placeholders::_1);
        FdsActorRequestPtr copy_far(new FdsActorRequest(
            FAR_ID(MigSvcCopyTokensReq), copy_req));
        objStorMgr->migrationSvc_->send_actor_request(copy_far);
    } else {
        objStorMgr->migrationSvcResponseCb(Error(ERR_OK));
    }
#endif

    std::set<fds_token_id> tokens = DLT::token_diff(objStorMgr->getUuid(),
            objStorMgr->omClient->getCurrentDLT(), objStorMgr->omClient->getPreviousDLT());

    if (tokens.size() > 0) {
        MigSvcCopyTokensReqPtr copy_req(new MigSvcCopyTokensReq());
        copy_req->tokens = tokens;
        // Send migration request to migration service
        copy_req->migsvc_resp_cb = std::bind(
            &ObjectStorMgr::migrationSvcResponseCb,
            objStorMgr,
            std::placeholders::_1);
        FdsActorRequestPtr copy_far(new FdsActorRequest(
            FAR_ID(MigSvcCopyTokensReq), copy_req));
        objStorMgr->migrationSvc_->send_actor_request(copy_far);
    } else {
        FDS_PLOG(objStorMgr->GetLog()) << "No tokens to copy";
        objStorMgr->migrationSvcResponseCb(Error(ERR_OK));
    }

}

void ObjectStorMgr::migrationSvcResponseCb(const Error& err) {
    omClient->sendMigrationStatusToOM(err);
}

void ObjectStorMgr::nodeEventOmHandler(int node_id,
        unsigned int node_ip_addr,
        int node_state,
        fds_uint32_t node_port,
        FDS_ProtocolInterface::FDSP_MgrIdType node_type)
{
    FDS_PLOG(objStorMgr->GetLog()) << "ObjectStorMgr - Node event Handler " << node_id << " Node IP Address " <<  node_ip_addr;
    switch(node_state) {
    case FDS_Node_Up :
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "ObjectStorMgr - Node UP event NodeId " << node_id << " Node IP Address " <<  node_ip_addr;
        break;

    case FDS_Node_Down:
    case FDS_Node_Rmvd:
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << " ObjectStorMgr - Node Down event NodeId :" << node_id << " node IP addr" << node_ip_addr ;
        break;
    case FDS_Start_Migration:
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << " ObjectStorMgr - Start Migration  event NodeId :" << node_id << " node IP addr" << node_ip_addr ;
        break;
    }
}

/*
 * Note this function is generally run in the context
 * of an Ice thread.
 */
void
ObjectStorMgr::volEventOmHandler(fds_volid_t  volumeId,
        VolumeDesc  *vdb,
        int          action,
        FDSP_ResultType result) {
    StorMgrVolume* vol = NULL;
    Error err = ERR_OK;

    fds_assert(vdb != NULL);

    switch(action) {
    case FDS_VOL_ACTION_CREATE :
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Received create for vol "
        << "[" << volumeId << ", "
        << vdb->getName() << "]";
        /*
         * Needs to reference the global SM object
         * since this is a static function.
         */
        objStorMgr->volTbl->registerVolume(*vdb);
        vol = objStorMgr->volTbl->getVolume(volumeId);
        fds_assert(vol != NULL);
        err = objStorMgr->qosCtrl->registerVolume(vol->getVolId(),
                dynamic_cast<FDS_VolumeQueue*>(vol->getQueue()));
        objStorMgr->objCache->vol_cache_create(volumeId, 1024 * 1024 * 8, 1024 * 1024 * 256);
        fds_assert(err == ERR_OK);
        if (err != ERR_OK) {
            FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "registration failed for vol id " << volumeId << " error: "
                    << err;
        }
        break;

    case FDS_VOL_ACTION_DELETE:
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Received delete for vol "
        << "[" << volumeId << ", "
        << vdb->getName() << "]";
        objStorMgr->qosCtrl->quieseceIOs(volumeId);
        objStorMgr->qosCtrl->deregisterVolume(volumeId);
        objStorMgr->volTbl->deregisterVolume(volumeId);
        break;
    case fds_notify_vol_mod:
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::notification) << "Received modify for vol "
        << "[" << volumeId << ", "
        << vdb->getName() << "]";

        vol = objStorMgr->volTbl->getVolume(volumeId);
        fds_assert(vol != NULL);
        vol->voldesc->modifyPolicyInfo(vdb->iops_min, vdb->iops_max, vdb->relativePrio);
        err = objStorMgr->qosCtrl->modifyVolumeQosParams(vol->getVolId(),
                vdb->iops_min, vdb->iops_max, vdb->relativePrio);
        if ( !err.ok() )  {
            FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Modify volume policy failed for vol " << vdb->getName() << " error: "
                    << err.GetErrstr();
        }
        break;
    default:
        fds_panic("Unknown (corrupt?) volume event recieved!");
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
            FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::debug) << "Nothing dirty in flash, going to sleep...";
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

Error ObjectStorMgr::writeBackObj(const ObjectID &objId) {
    Error err(ERR_OK);

    FDS_PLOG(objStorMgr->GetLog()) << "Writing back object " << objId
            << " from flash to disk";

    /*
     * Read back the object from flash.
     * TODO: We should pin the object in cache when writing
     * to flash so that we can read it back from memory rather
     * than flash.
     */
    ObjectBuf objData;
    err = readObject(objId, objData);
    if (err != ERR_OK) {
        return err;
    }

    /*
     * Write object back to disk tier.
     */
    err = writeObject(objId, objData, diskio::diskTier);
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

    FDS_PLOG(objStorMgr->GetLog()) << "Running unit test";

    /*
     * Create fake objects
     */
    std::string object_data("Hi, I'm object data.");
    FDSP_PutObjTypePtr put_obj_req(new FDSP_PutObjType());

    put_obj_req->volume_offset = 0;
    put_obj_req->data_obj_id.hash_high = 0x00;
    put_obj_req->data_obj_id.hash_low = 0x101;
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
        FDS_PLOG(objStorMgr->GetLog()) << "Failed to put object ";
        // delete put_obj_req;
        return;
    }
    // delete put_obj_req;

    FDSP_GetObjTypePtr get_obj_req(new FDSP_GetObjType());
    memset((char *)&(get_obj_req->data_obj_id),
            0x00,
            sizeof(get_obj_req->data_obj_id));
    get_obj_req->data_obj_id.hash_low = 0x101;
    err = enqGetObjectReq(msgHdr, get_obj_req, vol_id, 1, num_objs);
    // delete get_obj_req;
}

Error
ObjectStorMgr::writeObjectLocation(const ObjectID& objId,
        meta_obj_map_t *obj_map,
        fds_bool_t      append) {

    Error err(ERR_OK);

    diskio::MetaObjMap objMap;
    ObjectBuf          objData;

    if (append == true) {
        FDS_PLOG(objStorMgr->GetLog()) << "Appending new location for object " << objId;

        /*
         * Get existing object locations
         * TODO: We need a better way to update this
         * location DB with a new location. This requires
         * reading the existing locations, updating the entry,
         * and re-writing it. We often just want to append.
         */
        err = readObjectLocations(objId, objMap);
        if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
            FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Failed to read existing object locations"
                    << " during location write";
            return err;
        } else if (err == ERR_DISK_READ_FAILED) {
            /*
             * Assume this error means the key just did not exist.
             * TODO: Add an err to differention "no key" from "failed read".
             */
            FDS_PLOG(objStorMgr->GetLog()) << "Not able to read existing object locations"
                    << ", assuming no prior entry existed";
            err = ERR_OK;
        }
    }

    /*
     * Add new location to existing locations
     */
    objMap.updateMap(*obj_map);

    objData.size = objMap.marshalledSize();
    objData.data = std::string(objMap.marshalling(), objMap.marshalledSize());
    err = smObjDb->Put(objId, objData);
    if (err == ERR_OK) {
        FDS_PLOG(GetLog()) << "Updating object location for object "
                << objId << " to " << objMap;
    } else {
        FDS_PLOG_SEV(GetLog(), fds::fds_log::error) << "Failed to put object " << objId
                << " into odb with error " << err;
    }

    return err;
}

/*
 * Reads all object locations
 */
Error
ObjectStorMgr::readObjectLocations(const ObjectID     &objId,
        diskio::MetaObjMap &objMaps) {
    Error     err(ERR_OK);
    ObjectBuf objData;

    objData.size = 0;
    objData.data = "";
    err = smObjDb->Get(objId, objData);
    if (err == ERR_OK) {
        objData.size = objData.data.size();
        objMaps.unmarshalling(objData.data, objData.size);
    }

    return err;
}

Error
ObjectStorMgr::readObjectLocations(const ObjectID &objId,
        meta_obj_map_t *objMap) {
    Error err(ERR_OK);
    ObjectBuf objData;

    objData.size = 0;
    objData.data = "";
    err = smObjDb->Get(objId, objData);
    if (err == ERR_OK) {
        string_to_obj_map(objData.data, objMap);
        FDS_PLOG(GetLog()) << "Retrieving object location for object "
                << objId << " as " << objData.data;
    } else {
        FDS_PLOG(GetLog()) << "No object location found for object " << objId << " in index DB";
    }
    return err;
}

Error
ObjectStorMgr::deleteObjectLocation(const ObjectID& objId) { 

    Error err(ERR_OK);
    meta_obj_map_t *obj_map;

    diskio::MetaObjMap objMap;
    ObjectBuf          objData;

    /*
     * Get existing object locations
     */
    err = readObjectLocations(objId, objMap);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Failed to read existing object locations"
                << " during location write";
        return err;
    } else if (err == ERR_DISK_READ_FAILED) {
        /*
         * Assume this error means the key just did not exist.
         * TODO: Add an err to differention "no key" from "failed read".
         */
        FDS_PLOG(objStorMgr->GetLog()) << "Not able to read existing object locations"
                << ", assuming no prior entry existed";
        err = ERR_OK;
    }

    /*
     * Set the ref_cnt to 0, which will be the delete marker for this object and Garbage collector feeds on these objects
     */
    obj_map->obj_refcnt = -1;
    objData.size = objMap.marshalledSize();
    objData.data = std::string(objMap.marshalling(), objMap.marshalledSize());
    err = smObjDb->Put(objId, objData);
    if (err == ERR_OK) {
        FDS_PLOG(GetLog()) << "Setting the delete marker for object "
                << objId << " to " << objMap;
    } else {
        FDS_PLOG_SEV(GetLog(), fds::fds_log::error) << "Failed to put object " << objId
                << " into odb with error " << err;
    }

    return err;
}

Error
ObjectStorMgr::readObject(const ObjectID& objId,
        ObjectBuf& objData) {
    diskio::DataTier tier;
    return readObject(objId, objData, tier);
}

/*
 * Note the tierUsed parameter is an output param.
 * It gets set in the function and the prior
 * value is unused.
 * It is only guaranteed to be set if success
 * is returned
 */
Error 
ObjectStorMgr::readObject(const ObjectID   &objId,
        ObjectBuf        &objData,
        diskio::DataTier &tierUsed) {
    Error err(ERR_OK);

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    vadr_set_inval(vio.vol_adr);

    /*
     * TODO: Why to we create another oid structure here?
     * Just pass a ref to objId?
     */
    oid.oid_hash_hi = objId.GetHigh();
    oid.oid_hash_lo = objId.GetLow();

    // create a blocking request object
    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true);

    /*
     * Read all of the object's locations
     */
    diskio::MetaObjMap objMap;
    err = readObjectLocations(objId, objMap);
    if (err == ERR_OK) {
        /*
         * Read obj from flash if we can
         */
        if (objMap.hasFlashMap() == true) {
            err = objMap.getFlashMap(*(disk_req->req_get_vmap()));
            if(err != ERR_OK) {
                delete disk_req;
                return err;
            }
        } else {
            /*
             * Read obj from disk
             */
            fds_verify(objMap.hasDiskMap() == true);
            err = objMap.getDiskMap(*(disk_req->req_get_vmap()));
            if(err != ERR_OK) {
                delete disk_req;
                return err;
            }
        }
        // Update the request with the tier info from disk
        disk_req->setTierFromMap();
        tierUsed = disk_req->getTier();

        FDS_PLOG(objStorMgr->GetLog()) << "Reading object " << objId << " from "
                << ((disk_req->getTier() == diskio::diskTier) ? "disk" : "flash")
                << " tier";
        objData.size = disk_req->req_get_vmap()->obj_size;
        objData.data.resize(objData.size, 0);
        dio_mgr.disk_read(disk_req);
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
        const ObjectBuf& objCompData) {
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

    err = readObject(objId, objGetData);

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
            err = ERR_HASH_COLLISION;
            fds_panic("Encountered a hash collision checking object %s. Bailing out now!",
                    objId.ToHex().c_str());
        }
    } else if (err == ERR_DISK_READ_FAILED) {
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
ObjectStorMgr::writeObject(const ObjectID   &objId, 
        const ObjectBuf  &objData,
        fds_volid_t       volId,
        diskio::DataTier &tier) {
    /*
     * Ask the tiering engine which tier to place this object
     */
    tier = tierEngine->selectTier(objId, volId);
    return writeObject(objId, objData, tier);
}

Error 
ObjectStorMgr::writeObject(const ObjectID  &objId, 
        const ObjectBuf &objData,
        diskio::DataTier tier) {
    Error err(ERR_OK);

    fds_verify((tier == diskio::diskTier) ||
            (tier == diskio::flashTier));

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    fds_bool_t      pushOk;
    vadr_set_inval(vio.vol_adr);

    /*
     * TODO: Why to we create another oid structure here?
     * Just pass a ref to objId?
     */
    oid.oid_hash_hi = objId.GetHigh();
    oid.oid_hash_lo = objId.GetLow();

    FDS_PLOG(objStorMgr->GetLog()) << "Writing object " << objId << " into the "
            << ((tier == diskio::diskTier) ? "disk" : "flash")
            << " tier";
    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true, tier); // blocking call
    dio_mgr.disk_write(disk_req);
    err = writeObjectLocation(objId, disk_req->req_get_vmap(), true);
    if ((err == ERR_OK) &&
            (tier == diskio::flashTier)) {
        /*
         * If written to flash, add to dirty flash list
         */
        pushOk = dirtyFlashObjs->push(new ObjectID(objId));
        fds_verify(pushOk == true);
    }

    delete disk_req;
    return err;
}


Error 
ObjectStorMgr::relocateObject(const ObjectID &objId,
        diskio::DataTier from_tier,
        diskio::DataTier to_tier) {

    Error err(ERR_OK);
    ObjectBuf objGetData;

    objGetData.size = 0;
    objGetData.data = "";

    err = readObject(objId, objGetData);

    diskio::DataIO& dio_mgr = diskio::DataIO::disk_singleton();
    SmPlReq     *disk_req;
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    vadr_set_inval(vio.vol_adr);

    oid.oid_hash_lo = objId.GetLow();
    oid.oid_hash_lo = objId.GetLow();

    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objGetData, true, to_tier);
    dio_mgr.disk_write(disk_req);
    err = writeObjectLocation(objId, disk_req->req_get_vmap(), false);

    if (to_tier == diskio::diskTier) {
        perfStats->recordIO(flashToDisk, 0, diskio::diskTier, FDS_IO_WRITE);
    } else {
        fds_verify(to_tier == diskio::flashTier);
        perfStats->recordIO(diskToFlash, 0, diskio::flashTier, FDS_IO_WRITE);
    }
    FDS_PLOG(objStorMgr->GetLog()) << "relocateObject " << objId << " into the "
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
ObjectStorMgr::putObjectInternal(SmIoReq* putReq) {
    Error err(ERR_OK);
    const ObjectID&  objId    = putReq->getObjId();
    fds_volid_t volId         = putReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;
    ObjBufPtrType objBufPtr = NULL;
    const FDSP_PutObjTypePtr& putObjReq = putReq->getPutObjReq();
    bool new_buff_allocated = false, added_cache = false;

    objStorMutex->lock();

    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));

        // Now check for dedup here.
        if (objBufPtr->data == putObjReq->data_obj) {
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
        memcpy((void *)objBufPtr->data.c_str(), (void *)putObjReq->data_obj.c_str(), putObjReq->data_obj.size());
        new_buff_allocated = true;

        err = checkDuplicate(objId,
                *objBufPtr);

    }

    if (err == ERR_DUPLICATE) {
        if (new_buff_allocated) {
            added_cache = true;
            objCache->object_add(volId, objId, objBufPtr, false);
            objCache->object_release(volId, objId, objBufPtr);
        }
        FDS_PLOG(objStorMgr->GetLog()) << "Put dup:  " << err
                << ", returning success";
        /*
         * Reset the err to OK to ack the metadata update.
         */
        err = ERR_OK;
    } else if (err != ERR_OK) {
        if (new_buff_allocated) {
            objCache->object_release(volId, objId, objBufPtr);
            objCache->object_delete(volId, objId);
        }
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Failed to check object duplicate status on put: "
                << err;
    } else {

        /*
         * Write the object and record which tier it went to
         */
        err = writeObject(objId, *objBufPtr, volId, tierUsed);

        if (err != fds::ERR_OK) {
            objCache->object_release(volId, objId, objBufPtr);
            objCache->object_delete(volId, objId);
            objCache->object_release(volId, objId, objBufPtr);
            FDS_PLOG(objStorMgr->GetLog()) << "Successfully put object " << objId;
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
    objStorMutex->unlock();

    qosCtrl->markIODone(*putReq,
            tierUsed);

    /*
     * Prepare a response to send back.
     */
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(putReq->getTransId());
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = jrnlEntry->getMsgHdr();

    FDSP_PutObjTypePtr putObj(new FDSP_PutObjType());
    putObj->data_obj_id.hash_high = objId.GetHigh();
    putObj->data_obj_id.hash_low  = objId.GetLow();
    putObj->data_obj_len          = putObjReq->data_obj_len;

    if (err == ERR_OK) {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
    } else {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    }

    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_RSP;
    swapMgrId(msgHdr);
    fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
    omJrnl->release_transaction(putReq->getTransId());
    FDS_PLOG(objStorMgr->GetLog()) << "Sent async PutObj response after processing";

    /*
     * Free the IO request structure that
     * was allocated when the request was
     * enqueued.
     */
    delete putReq;

    return err;
}

Error
ObjectStorMgr::enqPutObjectReq(FDSP_MsgHdrTypePtr msgHdr, 
        FDSP_PutObjTypePtr putObjReq, 
        fds_volid_t        volId,
        fds_uint32_t       am_transId,
        fds_uint32_t       numObjs) {
    fds::Error err(fds::ERR_OK);
    TransJournalId trans_id;
    ObjectID obj_id(putObjReq->data_obj_id.hash_high,
            putObjReq->data_obj_id.hash_low);

    for(fds_uint32_t obj_num = 0; obj_num < numObjs; obj_num++) {

        /*
         * Allocate the ioReq structure. The pointer will get copied
         * into the queue and freed later once the IO completes.
         */
        SmIoReq *ioReq = new SmIoReq(putObjReq->data_obj_id.hash_high,
                putObjReq->data_obj_id.hash_low,
                // putObjReq->data_obj,
                putObjReq,
                volId,
                FDS_IO_WRITE,
                am_transId);

        err = omJrnl->create_transaction(obj_id, static_cast<FDS_IOType *>(ioReq), trans_id);
        ioReq->setTransId(trans_id);
        ObjectIdJrnlEntry *jrnlEntry = omJrnl->get_transaction(trans_id);
        jrnlEntry->setMsgHdr(msgHdr);

        err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));
        if (err != ERR_OK) {
            /*
             * Just return if we see an error. The way the function is setup
             * it's a bit unnatrual to return errors for multiple IOs so
             * we'll just stop at the first error we see to make sure it
             * doesn't get lost.
             */
            FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Unable to enqueue putObject request "
                    << am_transId << ":" << trans_id;
            return err;
        }
        FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued putObject request "
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

    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        objCache->object_release(volId, objId, objBufPtr);
        objCache->object_delete(volId, objId);
    }


    objStorMutex->lock();
    /*
     * Delete the object
     */
    err = deleteObjectLocation(objId);
    objStorMutex->unlock();
    if (err != fds::ERR_OK) {
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Failed to delete object " << err;
    } else {
        FDS_PLOG(objStorMgr->GetLog()) << "Successfully delete object " << objId;
    }

    qosCtrl->markIODone(*delReq,
            diskio::diskTier);

    /*
     * Prepare a response to send back.
     */
    ObjectIdJrnlEntry* jrnlEntry =  omJrnl->get_transaction(delReq->getTransId());
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr = jrnlEntry->getMsgHdr();

    FDSP_DeleteObjTypePtr delObj(new FDS_ProtocolInterface::FDSP_DeleteObjType());
    delObj->data_obj_id.hash_high = objId.GetHigh();
    delObj->data_obj_id.hash_low  = objId.GetLow();
    delObj->data_obj_len          = delObjReq->data_obj_len;

    if (err == ERR_OK) {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
    } else {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    }

    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_OBJ_RSP;
    swapMgrId(msgHdr);
    fdspDataPathClient(msgHdr->session_uuid)->DeleteObjectResp(msgHdr, delObj);
    omJrnl->release_transaction(delReq->getTransId());
    FDS_PLOG(objStorMgr->GetLog()) << "Sent async DelObj response after processing";

    /*
     * Free the IO request structure that
     * was allocated when the request was
     * enqueued.
     */
    delete delReq;

    return err;
}

void
ObjectStorMgr::PutObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
        const FDSP_PutObjTypePtr& put_obj_req) {
    Error err(ERR_OK);

    // Verify the integrity of the FDSP msg using chksums
    //
    // stor_mgr_verify_msg(fdsp_msg);
    //
    ObjectID oid(put_obj_req->data_obj_id.hash_high,
            put_obj_req->data_obj_id.hash_low);

    FDS_PLOG(objStorMgr->GetLog()) << "PutObject Obj ID: " << oid
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
    }

    counters_.put_reqs.incr();
}


void
ObjectStorMgr::DeleteObject(const FDSP_MsgHdrTypePtr& fdsp_msg,
        const FDSP_DeleteObjTypePtr& del_obj_req) {
    Error err(ERR_OK);

    // Verify the integrity of the FDSP msg using chksums
    //
    // stor_mgr_verify_msg(fdsp_msg);
    //
    ObjectID oid(del_obj_req->data_obj_id.hash_high,
            del_obj_req->data_obj_id.hash_low);

    FDS_PLOG(objStorMgr->GetLog()) << "DeleteObject Obj ID: " << oid
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
    }
}

Error
ObjectStorMgr::getObjectInternal(SmIoReq *getReq) {
    Error            err(ERR_OK);
    const ObjectID  &objId = getReq->getObjId();
    fds_volid_t volId      = getReq->getVolId();
    diskio::DataTier tierUsed = diskio::maxTier;
    const FDSP_GetObjTypePtr& getObjReq = getReq->getGetObjReq();
    ObjBufPtrType objBufPtr = NULL;

    /*
     * We need to fix this once diskmanager keeps track of object size
     * and allocates buffer automatically.
     * For now, we will pass the fixed block size for size and preallocate
     * memory for that size.
     */

    objStorMutex->lock();
    objBufPtr = objCache->object_retrieve(volId, objId);

    if (!objBufPtr) {
        ObjectBuf objData;
        objData.size = 0;
        objData.data = "";
        err = readObject(objId, objData, tierUsed);
        objBufPtr = objCache->object_alloc(volId, objId, objData.size);
        memcpy((void *)objBufPtr->data.c_str(), (void *)objData.data.c_str(), objData.size);
        objCache->object_add(volId, objId, objBufPtr, false); // read data is always clean
    } else {
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));
    }

    objStorMutex->unlock();

    if (err != fds::ERR_OK) {
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Failed to get object " << objId
                << " with error " << err;
        /*
         * Set the data to empty so we don't return
         * garbage.
         */
    } else {
        FDS_PLOG(objStorMgr->GetLog()) << "Successfully got object " << objId
                // << " and data " << objData.data
                << " for request ID " << getReq->io_req_id;
    }

    qosCtrl->markIODone(*getReq, tierUsed);

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

    fds_uint64_t oidHigh = objId.GetHigh();
    fds_uint64_t oidLow = objId.GetLow();
    getObj->data_obj_id.hash_high    = objId.GetHigh();
    getObj->data_obj_id.hash_low     = objId.GetLow();
    getObj->data_obj                 = (err == ERR_OK)? objBufPtr->data:"";
    getObj->data_obj_len             = (err == ERR_OK)? objBufPtr->size:0;

    if (err == ERR_OK) {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
    } else {
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    }
    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_RSP;
    swapMgrId(msgHdr);
    if (getObjReq->dlt_version != objStorMgr->omClient->getDltVersion()) {
	msgHdr->err_code = FDSP_ERR_DLT_CONFLICT; 			
	// msgHdr->result = FDSP_ERR_DLT_MISMATCH; 			
	// send the dlt version of SM to AM 
        getObj->dlt_version = objStorMgr->omClient->getDltVersion();
    }
    fdspDataPathClient(msgHdr->session_uuid)->GetObjectResp(msgHdr, getObj);
    FDS_PLOG(objStorMgr->GetLog()) << "Sent async GetObj response after processing";
    omJrnl->release_transaction(getReq->getTransId());

    objStats->updateIOpathStats(getReq->getVolId(), getReq->getObjId());
    volTbl->updateVolStats(getReq->getVolId());

    objStats->updateIOpathStats(getReq->getVolId(),getReq->getObjId());
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
    ObjectID obj_id(delObjReq->data_obj_id.hash_high,
            delObjReq->data_obj_id.hash_low);

    /*
     * Allocate and enqueue an IO request. The request is freed
     * when the IO completes.
     */
    SmIoReq *ioReq = new SmIoReq(delObjReq->data_obj_id.hash_high,
            delObjReq->data_obj_id.hash_low,
            // "",
            delObjReq,
            volId,
            FDS_DELETE_BLOB,
            msgHdr->req_cookie);

    err =  omJrnl->create_transaction(obj_id, static_cast<FDS_IOType *>(ioReq), trans_id);
    ObjectIdJrnlEntry *jrnlEntry = omJrnl->get_transaction(trans_id);
    jrnlEntry->setMsgHdr(msgHdr);
    ioReq->setTransId(trans_id);
    
    err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));

    if (err != fds::ERR_OK) {
        FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Unable to enqueue delObject request "
                << am_transId << ":" << trans_id;
        return err;
    }
    FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued delObject request "
            << am_transId << ":" << trans_id;

    return err;
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
    ObjectID oid(get_obj_req->data_obj_id.hash_high,
            get_obj_req->data_obj_id.hash_low);

    FDS_PLOG(objStorMgr->GetLog()) << "GetObject XID: " << fdsp_msg->req_cookie
            << ", Obj ID: " << oid
            << ", glob_vol_id: " << fdsp_msg->glob_volume_id
            << ", Num Objs: " << fdsp_msg->num_objects;

    err = enqGetObjectReq(fdsp_msg, get_obj_req,
            fdsp_msg->glob_volume_id,
            fdsp_msg->req_cookie,
            fdsp_msg->num_objects);
    if (err != ERR_OK) {
        fdsp_msg->result = FDSP_ERR_FAILED;
        fdsp_msg->err_code = err.getFdspErr();
    } else {
        fdsp_msg->result = FDSP_ERR_OK;
    }
    
    counters_.get_reqs.incr();
}

Error
ObjectStorMgr::enqGetObjectReq(FDSP_MsgHdrTypePtr msgHdr, 
                               FDSP_GetObjTypePtr getObjReq, 
                               fds_volid_t        volId, 
                               fds_uint32_t       am_transId, 
                               fds_uint32_t       numObjs) {
  Error err(ERR_OK);
  TransJournalId trans_id;
  ObjectID obj_id(getObjReq->data_obj_id.hash_high,
            getObjReq->data_obj_id.hash_low);

  /*
   * Allocate and enqueue an IO request. The request is freed
   * when the IO completes.
   */
  SmIoReq *ioReq = new SmIoReq(getObjReq->data_obj_id.hash_high,
                               getObjReq->data_obj_id.hash_low,
                               // "",
			       getObjReq,
                               volId,
                               FDS_IO_READ,
                               am_transId);

  err =  omJrnl->create_transaction(obj_id, static_cast<FDS_IOType *>(ioReq), trans_id);
  ioReq->setTransId(trans_id);
  ObjectIdJrnlEntry *jrnlEntry = omJrnl->get_transaction(trans_id);
  jrnlEntry->setMsgHdr(msgHdr);

  err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));

  if (err != fds::ERR_OK) {
    FDS_PLOG_SEV(objStorMgr->GetLog(), fds::fds_log::error) << "Unable to enqueue getObject request "
                                   << am_transId << ":" << trans_id;
    getObjReq->data_obj_len = 0;
    getObjReq->data_obj.assign("");
    return err;
  }
  FDS_PLOG(objStorMgr->GetLog()) << "Successfully enqueued getObject request "
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

    err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
    if (err != fds::ERR_OK) {
        FDS_PLOG_ERR(GetLog()) << "Failed to enqueue msg: " << ioReq->log_string();
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
        ObjectID objId(obj.meta_data.object_id.hash_high,
                       obj.meta_data.object_id.hash_low);
        /* TODO: Update obj metadata */

        /* Doing a look up before a commit a write */
        /* TODO:  Ideally if writeObject returns us an error for duplicate
         * write we don't need this check
         */ 
        diskio::MetaObjMap objMap;
        err = readObjectLocations(objId, objMap);
        if (err == ERR_OK) {
            continue;
        }
        /* NOTE: Not checking for other errors.  If read failed then write should also
         * fail
         */
        err = ERR_OK;

        /* Moving the data to not incur copy penalty */
        DBG(std::string temp_data = obj.data);
        ObjectBuf objData;
        objData.data = std::move(obj.data);
        objData.size = objData.data.size();
        fds_assert(temp_data == objData.data);
        obj.data.clear();

        err = writeObject(objId, objData, DataTier::diskTier);
        if (err != ERR_OK) {
            FDS_PLOG_ERR(GetLog()) << "Failed to write the object: " << objId;
           break; 
        }

        counters_.put_tok_objs.incr();
    }

    /* Mark the request as complete */
    qosCtrl->markIODone(*putTokReq,
            DataTier::diskTier);

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
            DataTier::diskTier);

    getTokReq->response_cb(err, getTokReq);
    /* NOTE: We expect the caller to free up ioReq */
}

inline void ObjectStorMgr::swapMgrId(const FDSP_MsgHdrTypePtr& fdsp_msg) {
    FDSP_MgrIdType temp_id;
    long tmp_addr;
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
}

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

    switch (io->io_type) {
        case FDS_IO_READ:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a get request";
            threadPool->schedule(getObjectExt,io);
            break;
        case FDS_IO_WRITE:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a put request";
            threadPool->schedule(putObjectExt,io);
            break;
        case FDS_DELETE_BLOB:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a Delete request";
            threadPool->schedule(delObjectExt,io);
            break;
        case FDS_SM_WRITE_TOKEN_OBJECTS:
        {
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a write token ibjects";
            // SmIoPutTokObjectsReq *putTokReq = static_cast<SmIoPutTokObjectsReq*>(io);
            // putTokReq->response_cb(err, putTokReq);
            threadPool->schedule(&ObjectStorMgr::putTokenObjectsInternal, objStorMgr, io);
            break;
        }
        case FDS_SM_READ_TOKEN_OBJECTS:
            FDS_PLOG(FDS_QoSControl::qos_log) << "Processing a read token objects";
            threadPool->schedule(&ObjectStorMgr::getTokenObjectsInternal, objStorMgr, io);
            break;
        default:
            fds_assert(!"Unknown message");
            break;
    }

    return err;
}


void log_ocache_stats() {

    while(1) {
        usleep(500000);
        objStorMgr->getObjCache()->log_stats_to_file("ocache_stats.dat");
    }

}

fds_int32_t SmObjDb::CompareKey(char *key, ObjectID obj_id)  { 
      return memcmp(key, (char *)&obj_id, sizeof(ObjectID) );
}


fds_int32_t  SmObjDb::RangeCompareKey(ObjectID obj_id, ObjectID start_obj_id, ObjectID end_obj_id) { 
   if (obj_id.GetHigh() >= start_obj_id.GetHigh()  && obj_id.GetHigh() <= end_obj_id.GetHigh() ) return 0;
   return -1;
}

void  SmObjDb::iterRetrieveObjects(const fds_token_id &token, 
        const size_t &max_size,
        FDSP_MigrateObjectList &obj_list,
        SMTokenItr &itr) {
    fds_uint32_t tot_msg_len = 0;
    fds_int64_t tokId = token  & SM_TOKEN_MASK;
    diskio::DataTier tierUsed;
    fds::Error err = ERR_OK;
    ObjectID objId;
    ObjectDB *odb = getObjectDB(tokId);
    if (odb == NULL ) { 
        itr.objId = SMTokenItr::itr_end;
        return;
    }

    ObjectID start_obj_id, end_obj_id;
    // If the iterator is non-zero then use that as a sarting point for the scan else make up a start from token
    if ( itr.objId.GetHigh() == 0 && itr.objId.GetLow() == 0) {
        start_obj_id.SetId((((fds_int64_t )token) << 32 )  | 0x0000000000000000, 0);
    } else {
        start_obj_id = itr.objId;
    }
    end_obj_id.SetId((((fds_int64_t )token) << 32 )| 0x00ffffffffffffff, 0xffffffffffffffff);

    leveldb::Slice startSlice((const char *)&start_obj_id, sizeof(ObjectID));
    leveldb::Slice endSlice((const char *)&end_obj_id, sizeof(ObjectID));

    boost::shared_ptr<leveldb::Iterator> dbIter(odb->GetDB()->NewIterator(odb->GetReadOptions()));
    leveldb::Options options_ = odb->GetOptions();

    memcpy(&objId , &start_obj_id, sizeof(ObjectID));
    FDS_PLOG(objStorMgr->GetLog()) << "Start of the loop " << objId  << "ending obj" << end_obj_id;
    for(dbIter->Seek(startSlice); dbIter->Valid() && (CompareKey((char *)&objId, end_obj_id) <= 0) ;dbIter->Next())
    {
        ObjectBuf        objData;
        // Read the record
        memcpy(&objId , dbIter->key().data(), sizeof(ObjectID));
        FDS_PLOG(objStorMgr->GetLog()) << "Checking an objectId for token range " << token << " into  objList" << objId ;

        // TODO: process the key/data
        if (RangeCompareKey(objId, start_obj_id, end_obj_id) == 0 ) {
            // Get the object buffer
            err = objStorMgr->readObject(objId, objData, tierUsed);
            if (err == ERR_OK ) {
                if ((max_size - tot_msg_len) >= objData.size) {
                    FDSP_MigrateObjectData mig_obj;
                    mig_obj.meta_data.token_id = token;
                    FDS_PLOG(objStorMgr->GetLog()) << "Adding a new objectId to objList" << objId;
                    mig_obj.meta_data.object_id.hash_high = objId.GetHigh();
                    mig_obj.meta_data.object_id.hash_low = objId.GetLow();
                    mig_obj.meta_data.obj_len = objData.size;
                    mig_obj.data = objData.data;
                    obj_list.push_back(mig_obj);
                    tot_msg_len += objData.size;

                    objStorMgr->counters_.get_tok_objs.incr();
                } else {
                    itr.objId = objId;
                    return;
                }
            }
        }
    } // Enf of for loop
    itr.objId = SMTokenItr::itr_end;

}

}  // namespace fds

