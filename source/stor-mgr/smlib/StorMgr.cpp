/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream>
#include <thread>
#include <functional>

#include "ObjMeta.h"
#include "SmObjDb.h"
#include <policy_rpc.h>
#include <policy_tier.h>
#include "StorMgr.h"
#include "fds_obj_cache.h"
#include <fds_timestamp.h>

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
    LOGDEBUG << "Received a Putobject() network request";

#ifdef FDS_TEST_SM_NOOP
    msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
    msgHdr->result = FDSP_ERR_OK;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
    LOGDEBUG << "FDS_TEST_SM_NOOP defined. Sent async PutObj response right after receiving req.";
    return;
#endif /* FDS_TEST_SM_NOOP */


    // check the payload checksum  and return Error, if we run in to issues 
    std::string new_checksum;

    //    objStorMgr->chksumPtr->checksum_update(putObj->data_obj_id.hash_high);
    //    objStorMgr->chksumPtr->checksum_update(putObj->data_obj_id.hash_low);
    objStorMgr->chksumPtr->checksum_update(putObj->data_obj_id.digest);
    objStorMgr->chksumPtr->checksum_update(putObj->data_obj_len);
    objStorMgr->chksumPtr->checksum_update(putObj->volume_offset);
    objStorMgr->chksumPtr->checksum_update(putObj->dlt_version);
    objStorMgr->chksumPtr->checksum_update(putObj->data_obj);

    objStorMgr->chksumPtr->get_checksum(new_checksum);
    LOGDEBUG << "RPC Checksum :" << new_checksum << " received checksum: " << msgHdr->payload_chksum; 

    /*
      if (msgHdr->payload_chksum.compare(new_checksum) != 0) {
      msgHdr->result = FDSP_ERR_CKSUM_MISMATCH; 			
      msgHdr->err_code = FDSP_ERR_RPC_CKSUM; 			
      LOGERROR << "RPC Checksum: " << new_checksum << " received checksum: " << msgHdr->payload_chksum; 

      msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
      objStorMgr->swapMgrId(msgHdr);
      objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
      LOGWARN << "Sent async PutObj response after checksum mismatch";
      return;
      }
    */

    /*
     * Track the outstanding get request.
     */

    if ((uint)putObj->dlt_version == objStorMgr->omClient->getDltVersion()) {
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
        LOGWARN << "DLT version Conflict returning the latest";
    }

    /*
     * If we failed to enqueue the I/O return the error response
     * now as there is no more processing to do.
     */
    if (msgHdr->result != FDSP_ERR_OK) {
        msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
        objStorMgr->swapMgrId(msgHdr);
        objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);

        LOGERROR << "Sent async PutObj response after receiving";
    }
}

void
ObjectStorMgrI::GetObject(FDSP_MsgHdrTypePtr& msgHdr,
                          FDSP_GetObjTypePtr& getObj)
{
    LOGDEBUG << "Received a Getobject() network request";

#ifdef FDS_TEST_SM_NOOP
    msgHdr->msg_code = FDSP_MSG_GET_OBJ_RSP;
    msgHdr->result = FDSP_ERR_OK;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->GetObjectResp(msgHdr, getObj);
    LOGDEBUG << "FDS_TEST_SM_NOOP defined. Sent async GetObj response right after receiving req.";
    return;
#endif /* FDS_TEST_SM_NOOP */

    /*
     * Track the outstanding get request.
     */
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
        if((uint)getObj->dlt_version != objStorMgr->omClient->getDltVersion()) {
            msgHdr->result = FDSP_ERR_DLT_MISMATCH;
            msgHdr->err_code = FDSP_ERR_DLT_CONFLICT;
            // send the dlt version of SM to AM
            getObj->dlt_version = objStorMgr->omClient->getDltVersion();
            // update the resp  with new DLT
            objStorMgr->omClient->getLatestDlt(getObj->dlt_data);
        }

        objStorMgr->swapMgrId(msgHdr);
        objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->GetObjectResp(msgHdr, getObj);

        LOGDEBUG << "Sent async GetObj response after receiving";
    }
}

void
ObjectStorMgrI::DeleteObject(FDSP_MsgHdrTypePtr& msgHdr,
                             FDSP_DeleteObjTypePtr& delObj)
{
    LOGDEBUG << "Received a Deleteobject() network request";

#ifdef FDS_TEST_SM_NOOP
    msgHdr->msg_code = FDSP_MSG_PUT_OBJ_RSP;
    msgHdr->result = FDSP_ERR_OK;
    objStorMgr->swapMgrId(msgHdr);
    objStorMgr->fdspDataPathClient(msgHdr->session_uuid)->DeleteObjectResp(msgHdr, delObj);
    LOGDEBUG << "FDS_TEST_SM_NOOP defined. Sent async DeleteObj response right after receiving req.";
    return;
#endif /* FDS_TEST_SM_NOOP */

    /*
     * Track the outstanding get request.
     */
    if ((uint)delObj->dlt_version == objStorMgr->omClient->getDltVersion()) {

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

        LOGDEBUG << "Sent async DeleteObj response after receiving";
    }
}

void
ObjectStorMgrI::OffsetWriteObject(FDSP_MsgHdrTypePtr& msg_hdr, FDSP_OffsetWriteObjTypePtr& offset_write_obj) {
    LOGDEBUG << "In the interface offsetwrite()";
}

void
ObjectStorMgrI::RedirReadObject(FDSP_MsgHdrTypePtr &msg_hdr, FDSP_RedirReadObjTypePtr& redir_read_obj) {
    LOGDEBUG << "In the interface redirread()";
}

void ObjectStorMgrI::GetObjectMetadata(
        boost::shared_ptr<FDSP_GetObjMetadataReq>& metadata_req) {  // NOLINT
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
                             Platform *platform, Module **mod_vec)
    : PlatformProcess(argc, argv, "fds.sm.", "sm.log", platform, mod_vec),
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

    GetLog()->setSeverityFilter((fds_log::severity_level) conf_helper_.get<int>("log_severity"));
    LOGDEBUG << "Constructing the Object Storage Manager";
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
                            GetLog());
    qosCtrl->runScheduler();

    /* Set up the journal */
    omJrnl = new TransJournal<ObjectID, ObjectIdJrnlEntry>(qosCtrl, GetLog());
    /*
     * stats class init
     */
    objStats = &gl_objStats;

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

void ObjectStorMgr::setup()
{
    /*
     * Invoke FdsProcess setup so that it can setup the signal hander and
     * execute the module vector for us
     */
    PlatformProcess::setup();

    /* Rest of the setup */
    // todo: clean up the code below.  It's doing too many things here.
    // Refactor into functions or make it part of module vector

    std::string     myIp;

    proc_root->fds_mkdir(proc_root->dir_user_repo_objs().c_str());
    std::string obj_dir = proc_root->dir_user_repo_objs();

    // Create leveldb
    smObjDb = new  SmObjDb(this, obj_dir, objStorMgr->GetLog());
    // init the checksum verification class
    chksumPtr =  new checksum_calc();

    /* Token state db */
    tokenStateDb_.reset(new kvstore::TokenStateDB());
    // TODO(Rao): token state db population should be done based on
    // om events.  For now hardcoding
    for (fds_token_id tok = 0; tok < 256; tok++) {
        tokenStateDb_->addToken(tok);
    }

    /* Set up FDSP RPC endpoints */
    nst_ = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_STOR_MGR));
    myIp = netSession::getLocalIp();
    setup_datapath_server(myIp);

    /*
     * Register this node with OM.
     */
    omClient = new OMgrClient(FDSP_STOR_MGR,
                              *plf_mgr->plf_get_om_ip(),
                              plf_mgr->plf_get_om_ctrl_port(),
                              "localhost-sm",
                              GetLog(),
                              nst_, plf_mgr);

    /*
     * Create local volume table. Create after omClient
     * is initialized, because it needs to register with the
     * omClient. Create before register with OM because
     * the OM vol event receivers depend on this table.
     */
    volTbl = new StorMgrVolumeTable(this, GetLog());

    /* Create tier related classes -- has to be after volTbl is created */
    FdsRootDir::fds_mkdir(proc_root->dir_fds_var_stats().c_str());
    std::string obj_stats_dir = proc_root->dir_fds_var_stats();
    rankEngine = new ObjectRankEngine(obj_stats_dir, 100000, volTbl,
                                      objStats, objStorMgr->GetLog());
    tierEngine = new TierEngine(TierEngine::FDS_TIER_PUT_ALGO_BASIC_RANK,
                                volTbl, rankEngine, objStorMgr->GetLog());
    objCache = new FdsObjectCache(1024 * 1024 * 256,
                                  slab_allocator_type_default,
                                  eviction_policy_type_default,
                                  objStorMgr->GetLog());

    // TODO: join this thread
    std::thread *stats_thread = new std::thread(log_ocache_stats);

    // Create a special queue for System (background) tasks
    // and registe rwith QosCtrlr
    sysTaskQueue = new SmVolQueue(FdsSysTaskQueueId,
                                  256,
                                  getSysTaskIopsMax(),
                                  getSysTaskIopsMin(),
                                  getSysTaskPri());

    qosCtrl->registerVolume(FdsSysTaskQueueId,
                            sysTaskQueue);
    // TODO(Rao): Size it appropriately
    objCache->vol_cache_create(FdsSysTaskQueueId, 8, 256);

    /*
     * Register/boostrap from OM
     */
    omClient->initialize();
    omClient->registerEventHandlerForNodeEvents((node_event_handler_t)nodeEventOmHandler);
    omClient->registerEventHandlerForVolEvents((volume_event_handler_t)volEventOmHandler);
    omClient->registerEventHandlerForMigrateEvents((migration_event_handler_t)migrationEventOmHandler);
    omClient->registerEventHandlerForDltCloseEvents((dltclose_event_handler_t) dltcloseEventHandler);
    omClient->omc_srv_pol = &sg_SMVolPolicyServ;
    omClient->startAcceptingControlMessages();
    omClient->registerNodeWithOM(plf_mgr);

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
        for (fds_int32_t testVolId = 1; testVolId < numTestVols + 1; testVolId++) {
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
                              false,
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
    std::string node_name = "_SM";
    // TODO: Ideally createServerSession should take a shared pointer
    // for datapath_handler.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    // TODO: Figure out who cleans up datapath_session_
    datapath_session_ = nst_->createServerSession<netDataPathServerSession>(
        myIpInt,
        plf_mgr->plf_get_my_data_port(),
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
                                            clust_comm_mgr_,
                                            tokenStateDb_));
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
    return state == kvstore::TokenStateInfo::SYNCING;
}

void ObjectStorMgr::migrationEventOmHandler(bool dlt_type)
{
    GLOGDEBUG << "ObjectStorMgr - Migration  event Handler " << dlt_type;

    std::set<fds_token_id> tokens =
            DLT::token_diff(objStorMgr->getUuid(),
                    objStorMgr->omClient->getCurrentDLT(),
                    objStorMgr->omClient->getPreviousDLT());

    if (tokens.size() > 0) {
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
        GLOGDEBUG << "No tokens to copy";
        objStorMgr->migrationSvcResponseCb(Error(ERR_OK), TOKEN_COPY_COMPLETE);
        objStorMgr->migrationSvcResponseCb(Error(ERR_OK), MIGRATION_OP_COMPLETE);
    }

}

void ObjectStorMgr::dltcloseEventHandler()
{
    MigSvcSyncCloseReqPtr close_req(new MigSvcSyncCloseReq());
    close_req->sync_close_ts = get_fds_timestamp_ms();

    FdsActorRequestPtr close_far(new FdsActorRequest(
            FAR_ID(MigSvcSyncCloseReq), close_req));

    objStorMgr->migrationSvc_->send_actor_request(close_far);

    GLOGNORMAL << "Received ioclose. Time: " << close_req->sync_close_ts;
}

void ObjectStorMgr::migrationSvcResponseCb(const Error& err,
        const MigrationStatus& status) {
    if (status == TOKEN_COPY_COMPLETE) {
        LOGDEBUG << "Token copy complete";
        omClient->sendMigrationStatusToOM(err);
    } else if (status == MIGRATION_OP_COMPLETE) {
        // TODO(Rao): Send migration comple to om
        LOGDEBUG << "Token migration complete";
    }
}

void ObjectStorMgr::nodeEventOmHandler(int node_id,
                                       unsigned int node_ip_addr,
                                       int node_state,
                                       fds_uint32_t node_port,
                                       FDS_ProtocolInterface::FDSP_MgrIdType node_type)
{
    GLOGDEBUG << "ObjectStorMgr - Node event Handler " << node_id << " Node IP Address " <<  node_ip_addr;
    switch(node_state) {
        case FDS_Node_Up :
            GLOGNOTIFY << "ObjectStorMgr - Node UP event NodeId " << node_id << " Node IP Address " <<  node_ip_addr;
            break;

        case FDS_Node_Down:
        case FDS_Node_Rmvd:
            GLOGNOTIFY << " ObjectStorMgr - Node Down event NodeId :" << node_id << " node IP addr" << node_ip_addr ;
            break;
        case FDS_Start_Migration:
            GLOGNOTIFY << " ObjectStorMgr - Start Migration  event NodeId :" << node_id << " node IP addr" << node_ip_addr ;
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
                                 fds_bool_t check_only,
                                 FDSP_ResultType result) {
    StorMgrVolume* vol = NULL;
    Error err(ERR_OK);
    fds_assert(vdb != NULL);

    switch(action) {
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
                err = objStorMgr->qosCtrl->registerVolume(vol->getVolId(),
                                                          dynamic_cast<FDS_VolumeQueue*>(vol->getQueue()));
		
		if (err.ok()) {
		  objStorMgr->objCache->vol_cache_create(volumeId, 1024 * 1024 * 8, 1024 * 1024 * 256);
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
            vol->voldesc->modifyPolicyInfo(vdb->iops_min, vdb->iops_max, vdb->relativePrio);
            err = objStorMgr->qosCtrl->modifyVolumeQosParams(vol->getVolId(),
                                                             vdb->iops_min, vdb->iops_max, vdb->relativePrio);
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
    OpCtx opCtx(OpCtx::RELOCATE, 0);
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
        LOGDEBUG << "Failed to put object ";
        // delete put_obj_req;
        return;
    }
    // delete put_obj_req;

    FDSP_GetObjTypePtr get_obj_req(new FDSP_GetObjType());
    memset((char *)&(get_obj_req->data_obj_id),
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

   /*
   * Get existing object locations
   */
    err = readObjMetaData(objId, objMap);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
        LOGERROR << "Failed to read existing object locations"
                    << " during location write";
        return err;
     } else if (err == ERR_DISK_READ_FAILED) {
            /*
             * Assume this error means the key just did not exist.
             * TODO: Add an err to differention "no key" from "failed read".
             */
            objMap.initialize(objId, obj_size);
            LOGDEBUG << "Not able to read existing object locations"
                    << ", assuming no prior entry existed";
            err = ERR_OK;
     }

    /*
     * Add new location to existing locations
     */
    objMap.updatePhysLocation(obj_phy_loc);

    if(relocate_flag) { 
      objMap.removePhyLocation(fromTier);
    } else {
      objMap.updateAssocEntry(objId, (fds_volid_t)vol->vol_uuid);
    }

    err = smObjDb->put(opCtx, objId, objMap);
    if (err == ERR_OK) {
        LOGDEBUG << "Updating object location for object "
                << objId << " to " << objMap;
    } else {
        LOGERROR << "Failed to put object " << objId
                << " into odb with error " << err;
    }

    return err;
}

/*
 * Reads all object locations
 */
Error
ObjectStorMgr::readObjMetaData(const ObjectID &objId,
        ObjMetaData& objMap) 
{
    return smObjDb->get(objId, objMap);
}

Error
ObjectStorMgr::deleteObjectMetaData(const OpCtx &opCtx,
        const ObjectID& objId, fds_volid_t vol_id) {

    Error err(ERR_OK);
    // NOTE !!!
    ObjMetaData objMap;

    /*
     * Get existing object locations
     */
    err = readObjMetaData(objId, objMap);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
        LOGERROR << "Failed to read existing object locations"
                << " during location write";
        return err;
    } else if (err == ERR_DISK_READ_FAILED) {
        /*
         * Assume this error means the key just did not exist.
         * TODO: Add an err to differention "no key" from "failed read".
         */
        LOGDEBUG << "Not able to read existing object locations"
                << ", assuming no prior entry existed";
        err = ERR_OK;
        return err;
    }

    /*
     * Set the ref_cnt to 0, which will be the delete marker for the Garbage collector
     */
    objMap.deleteAssocEntry(objId, vol_id, opCtx.ts);
    err = smObjDb->put(opCtx, objId, objMap);
    if (err == ERR_OK) {
        LOGDEBUG << "Setting the delete marker for object "
                << objId << " to " << objMap;
    } else {
        LOGERROR << "Failed to put object " << objId
                << " into odb with error " << err;
    }

    return err;
}

Error
ObjectStorMgr::readObject(const SmObjDb::View& view, const ObjectID& objId,
        ObjMetaData& objMetadata,
        ObjectBuf& objData) {
    diskio::DataTier tier;
    return readObject(view, objId, objMetadata, objData, tier);
}

Error
ObjectStorMgr::readObject(const SmObjDb::View& view, const ObjectID& objId,
        ObjectBuf& objData) {
    ObjMetaData objMetadata;
    diskio::DataTier tier;
    return readObject(view, objId, objMetadata, objData, tier);
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
                          diskio::DataTier &tierUsed) {
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
    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true);

    /*
     * Read all of the object's locations
     */
    err = readObjMetaData(objId, objMetadata);
    if (err == ERR_OK) {
        /*
         * Read obj from flash if we can
         */
        if (objMetadata.onFlashTier() ) {
            tierToUse = flashTier;
        } else {
            /*
             * Read obj from disk
             */
            tierToUse = diskTier;
        }
        // Update the request with the tier info from disk
        disk_req->setTier(tierToUse);
        disk_req->set_phy_loc(objMetadata.getObjPhyLoc(tierToUse));
        tierUsed = tierToUse;

        LOGDEBUG << "Reading object " << objId << " from "
                << ((disk_req->getTier() == diskio::diskTier) ? "disk" : "flash")
                << " tier";
        objData.size = objMetadata.getObjSize();
        objData.data.resize(objData.size, 0);
        err = dio_mgr.disk_read(disk_req);
        if ( err != ERR_OK) {
            LOGDEBUG << " Disk Read Err: " << err; 
            delete disk_req;
            return err;
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
    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true, tier); // blocking call
    err = dio_mgr.disk_write(disk_req);
    if (err != ERR_OK) {
        LOGDEBUG << " 1. Disk Write Err: " << err; 
        delete disk_req;
        return err;
    }
    
    err = writeObjectMetaData(opCtx, objId, objData.data.length(),
                disk_req->req_get_phy_loc(), false, diskTier, &vio);


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
    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objData, true, tier); // blocking call
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

    disk_req = new SmPlReq(vio, oid, (ObjectBuf *)&objGetData, true, to_tier);
    err = dio_mgr.disk_write(disk_req);
    if (err != ERR_OK) {
        LOGDEBUG << " 2. Disk Write Err: " << err; 
        delete disk_req;
        return err;
    }
    err = writeObjectMetaData(opCtx, objId, objGetData.data.length(),
            disk_req->req_get_phy_loc(), true, from_tier, &vio);

    if (to_tier == diskio::diskTier) {
        perfStats->recordIO(flashToDisk, 0, diskio::diskTier, FDS_IO_WRITE);
    } else {
        fds_verify(to_tier == diskio::flashTier);
        perfStats->recordIO(diskToFlash, 0, diskio::flashTier, FDS_IO_WRITE);
    }
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
        if (objMap.isInitialized() == false) { 
            err = readObjMetaData(objId, objMap);
            /* At this point object metadata must exist */
            fds_verify(err == ERR_OK);
        }

        // Update  the AssocEntry for dedupe-ing
        objMap.updateAssocEntry(objId, volId);
        err = smObjDb->put(opCtx, objId, objMap);
        if (err == ERR_OK) {
            LOGDEBUG << "Dedupe object Assoc Entry for object "
                    << objId << " to " << objMap;
        } else {
            LOGERROR << "Failed to put ObjMetaData " << objId
                    << " into odb with error " << err;
        }

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
    objStorMutex->unlock();

    qosCtrl->markIODone(*putReq,
                        tierUsed);

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
        msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_FAILED;
        msgHdr->err_code = err.getFdspErr();
    }

    msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_RSP;
    swapMgrId(msgHdr);
    fdspDataPathClient(msgHdr->session_uuid)->PutObjectResp(msgHdr, putObj);
    omJrnl->release_transaction(putReq->getTransId());
    LOGDEBUG << "Sent async PutObj response after processing";

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
    // TODO(Rao): Refactor create_transaction so that it just takes key and cb as
    // params
    Error err = omJrnl->create_transaction(obj_id,
                                           static_cast<FDS_IOType *>(ioReq), trans_id,
                                           std::bind(&ObjectStorMgr::create_transaction_cb, this,
                                                     msgHdr, ioReq, std::placeholders::_1));
    if (err != ERR_OK &&
        err != ERR_TRANS_JOURNAL_REQUEST_QUEUED) {
        return err;
    }

    if (err == ERR_TRANS_JOURNAL_REQUEST_QUEUED) {
        return ERR_OK;
    }
    err = qosCtrl->enqueueIO(ioReq->getVolId(), static_cast<FDS_IOType*>(ioReq));
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

    for(fds_uint32_t obj_num = 0; obj_num < numObjs; obj_num++) {

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

    objBufPtr = objCache->object_retrieve(volId, objId);
    if (objBufPtr != NULL) {
        objCache->object_release(volId, objId, objBufPtr);
        objCache->object_delete(volId, objId);
    }


    objStorMutex->lock();
    /*
     * Delete the object, decrement refcnt of the assoc entry & overall refcnt
     */
    err = deleteObjectMetaData(opCtx, objId, volId);
    objStorMutex->unlock();
    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to delete object " << err;
    } else {
        LOGDEBUG << "Successfully delete object " << objId;
    }

    qosCtrl->markIODone(*delReq,
                        diskio::diskTier);

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
    fdspDataPathClient(msgHdr->session_uuid)->DeleteObjectResp(msgHdr, delObj);
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
    objBufPtr = NULL; //Disable the object cache lookup 

    if (!objBufPtr) {
        ObjectBuf objData;
        ObjMetaData objMetadata;
        objData.size = 0;
        objData.data = "";
        err = readObject(SmObjDb::SYNC_MERGED, objId, objMetadata, objData, tierUsed);
        if (err == fds::ERR_OK) {
            objBufPtr = objCache->object_alloc(volId, objId, objData.size);
            memcpy((void *)objBufPtr->data.c_str(), (void *)objData.data.c_str(), objData.size);
            objCache->object_add(volId, objId, objBufPtr, false); // read data is always clean
            // ACL: If this Volume never put this object, then it should not access the object
            if (!objMetadata.isVolumeAssociated(volId)) {
               err = ERR_UNAUTH_ACCESS;
               LOGDEBUG << "Volume " << volId << " unauth-access of object " << objId
                // << " and data " << objData.data
                << " for request ID " << getReq->io_req_id;
            }
        }
    } else {
        fds_verify(!(objCache->is_object_io_in_progress(volId, objId, objBufPtr)));
    }

    objStorMutex->unlock();

    if (err != fds::ERR_OK) {
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
    if ((uint)getObjReq->dlt_version != objStorMgr->omClient->getDltVersion()) {
	msgHdr->err_code = FDSP_ERR_DLT_CONFLICT; 			
	// msgHdr->result = FDSP_ERR_DLT_MISMATCH; 			
	// send the dlt version of SM to AM 
        getObj->dlt_version = objStorMgr->omClient->getDltVersion();
    }
    fdspDataPathClient(msgHdr->session_uuid)->GetObjectResp(msgHdr, getObj);
    LOGDEBUG << "Sent async GetObj response after processing";
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
        return err;
    }
    LOGDEBUG << "Successfully enqueued delObject request "
             << am_transId << ":" << trans_id;

    return err;
}


boost::shared_ptr<FDSP_DataPathReqClient> ObjectStorMgr::getProxyClient(ObjectID& oid, const FDSP_MsgHdrTypePtr& msg) {
    const DLT* dlt = getDLT();
    NodeUuid uuid=0;
        
    FDSP_MsgHdrTypePtr& fdsp_msg = const_cast<FDSP_MsgHdrTypePtr&> (msg);
    // get the first Node that is not ME!!
    DltTokenGroupPtr nodes = dlt->getNodes(oid);
    for (uint i = 0 ; i< nodes->getLength() ; i++ ) {
        if (nodes->get(i) != getUuid()) {
            uuid = nodes->get(i);
            break;
        }
    }

    fds_verify(uuid != getUuid());
        
    LOGDEBUG << "obj : " << oid << " not located here : " << getUuid() << " : proxy_count="<<fdsp_msg->proxy_count;
    LOGDEBUG << "proxying request to " << uuid;
    fds_int32_t node_state = -1;

    uint addr;
    fds_uint32_t port;
    int state;
    // Get primary SM's node info
    omClient->getNodeInfo(uuid.uuid_get_val(),&addr,&port,&state);
    fdsp_msg->dst_ip_lo_addr = addr;
    fdsp_msg->dst_port = port;
    
    netDataPathClientSession *sessionCtx = nst_->getClientSession<netDataPathClientSession>(fdsp_msg->dst_ip_lo_addr, fdsp_msg->dst_port);
    fds_verify(sessionCtx != NULL);
        
    boost::shared_ptr<FDSP_DataPathReqClient> client = sessionCtx->getClient();
    LOGDEBUG << "switching session from [" << fdsp_msg->session_uuid << "] to [" <<sessionCtx->getSessionId() <<"]";
    fdsp_msg->session_uuid = sessionCtx->getSessionId();
    fdsp_msg->proxy_count++;
    return client;
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


    // TODO (Rao) - Uncomment this code
    // check if we have the object or else we need to proxy it 
    /**
    if (!smObjDb->dataPhysicallyExists(oid)) {
        LOGWARN << "object id " << oid << " is not here , find it else where";
        
        getProxyClient(oid,fdsp_msg)->GetObject(*fdsp_msg, *get_obj_req);
        return ;
    }
    */

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

    switch (ioReq->io_type) {
        case FDS_SM_WRITE_TOKEN_OBJECTS:
        case FDS_SM_READ_TOKEN_OBJECTS:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        case FDS_SM_SNAPSHOT_TOKEN:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        case FDS_SM_SYNC_APPLY_METADATA:
            objectId.SetId(static_cast<SmIoApplySyncMetadata*>(ioReq)->md.object_id.digest);
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        case FDS_SM_SYNC_RESOLVE_SYNC_ENTRY:
            objectId = static_cast<SmIoResolveSyncEntry*>(ioReq)->object_id;
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        case FDS_SM_APPLY_OBJECTDATA:
            objectId = static_cast<SmIoApplyObjectdata*>(ioReq)->obj_id;
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        case FDS_SM_READ_OBJECTDATA:
            objectId = static_cast<SmIoReadObjectdata*>(ioReq)->getObjId();
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        case FDS_SM_READ_OBJECTMETADATA:
            objectId = static_cast<SmIoReadObjectMetadata*>(ioReq)->getObjId();
            err =  enqTransactionIo(nullptr, objectId, ioReq, trans_id);
            if (err != fds::ERR_OK) {
                LOGERROR << "Failed to enqueue msg: " << ioReq->log_string();
            }
            break;
        default:
            fds_assert(!"Unknown message");
            LOGERROR << "Unknown message: " << ioReq->io_type;
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
        if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
            fds_assert(!"ObjMetadata read failed" );
            LOGERROR << "Failed to write the object: " << objId;
            break;
        }

        err = ERR_OK;

        /* Apply metadata */
        objMetadata.apply(obj.meta_data);

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
        err = writeObjectDataToTier(objId, objData, DataTier::diskTier, phys_loc);
        if (err != ERR_OK) {
            LOGERROR << "Failed to write the object: " << objId;
            break; 
        }

        /* write the metadata */
        objMetadata.updatePhysLocation(&phys_loc);
        smObjDb->put_(objId, objMetadata);
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


    leveldb::DB *db;
    leveldb::ReadOptions options;

    smObjDb->snapshot(snapReq->token_id, db, options);

    /* Mark the request as complete */
    qosCtrl->markIODone(*snapReq,
                        DataTier::diskTier);

    snapReq->smio_snap_resp_cb(err, snapReq, options, db);
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
            DataTier::diskTier);

    applyMdReq->smio_sync_md_resp_cb(e, applyMdReq);
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
            DataTier::diskTier);

    resolve_entry->smio_resolve_resp_cb(e, resolve_entry);
}

void
ObjectStorMgr::applyObjectDataInternal(SmIoReq* ioReq)
{
    SmIoApplyObjectdata *applydata_entry =  static_cast<SmIoApplyObjectdata*>(ioReq);
    ObjectID objId = applydata_entry->obj_id;
    ObjMetaData objMetadata;
    Error err = smObjDb->get(objId, objMetadata);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
        fds_assert(!"ObjMetadata read failed" );
        LOGERROR << "Failed to write the object: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                DataTier::diskTier);
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    err = ERR_OK;

    if (objMetadata.dataPhysicallyExists()) {
        /* No need to write the object data.  It already exits */
        LOGDEBUG << "Object already exists. Id: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                DataTier::diskTier);
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
    err = writeObjectDataToTier(objId, objData, DataTier::diskTier, phys_loc);
    if (err != ERR_OK) {
        fds_assert(!"Failed to write");
        LOGERROR << "Failed to write the object: " << objId;
        qosCtrl->markIODone(*applydata_entry,
                DataTier::diskTier);
        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        return;
    }

    /* write the metadata */
    objMetadata.updatePhysLocation(&phys_loc);
    smObjDb->put(OpCtx(OpCtx::SYNC), objId, objMetadata);

    qosCtrl->markIODone(*applydata_entry,
            DataTier::diskTier);

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
            objMetadata, objData, tierUsed);
    if (err != ERR_OK) {
        fds_assert(!"failed to read");
        LOGERROR << "Failed to read object id: " << read_entry->getObjId();
    }
    // TODO(Rao): use std::move here
    read_entry->obj_data.data = objData.data;
    fds_assert(read_entry->obj_data.data.size() > 0);

    qosCtrl->markIODone(*read_entry,
            DataTier::diskTier);
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
        read_entry->smio_readmd_resp_cb(err, read_entry);
    }

    objMetadata.extractSyncData(read_entry->meta_data);

    qosCtrl->markIODone(*read_entry,
            DataTier::diskTier);
    read_entry->smio_readmd_resp_cb(err, read_entry);
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
    while(1) {
        usleep(500000);
        objStorMgr->getObjCache()->log_stats_to_file("ocache_stats.dat");
    }
#endif
}


}  // namespace fds

