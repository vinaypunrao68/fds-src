/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <utility>
#include <fds_migration.h>
#include <fds_err.h>
#include <fds_uuid.h>

namespace fds {

FDSP_MigrationPathRpc::
FDSP_MigrationPathRpc(FdsMigrationSvc &mig_svc, fds_log *log) // NOLINT
    : mig_svc_(mig_svc) {
    SetLog(log);
}

void FDSP_MigrationPathRpc::
CopyToken(boost::shared_ptr<FDSP_CopyTokenReq>& copy_req) // NOLINT
{
    FdsActorRequestPtr copy_far(new FdsActorRequest(FAR_ID(FDSP_CopyTokenReq), copy_req));
    if (copy_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(copy_far);
}

void FDSP_MigrationPathRpc::
PushTokenObjects(boost::shared_ptr<FDSP_PushTokenObjectsReq>& push_req) // NOLINT
{
    FdsActorRequestPtr push_far(
            new FdsActorRequest(FAR_ID(FDSP_PushTokenObjectsReq), push_req));
    if (push_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(push_far);
}

void FDSP_MigrationPathRpc::
CopyTokenResp(boost::shared_ptr<FDSP_CopyTokenResp>& copytok_resp) // NOLINT
{
    FdsActorRequestPtr copy_far(
            new FdsActorRequest(FAR_ID(FDSP_CopyTokenResp), copytok_resp));
    if (copy_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(copy_far);
}

void FDSP_MigrationPathRpc::
PushTokenObjectsResp(boost::shared_ptr<FDSP_PushTokenObjectsResp>& pushtok_resp)  // NOLINT
{
    FdsActorRequestPtr push_far(
            new FdsActorRequest(FAR_ID(FDSP_PushTokenObjectsResp), pushtok_resp));
    if (push_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(push_far);
}

void FDSP_MigrationPathRpc::
SyncToken(boost::shared_ptr<FDSP_SyncTokenReq>& sync_req) // NOLINT
{
    FdsActorRequestPtr sync_far(
            new FdsActorRequest(FAR_ID(FDSP_SyncTokenReq), sync_req));
    if (sync_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(sync_far);
}

void FDSP_MigrationPathRpc::
SyncTokenResp(boost::shared_ptr<FDSP_SyncTokenResp>& synctok_resp) // NOLINT
{
    FdsActorRequestPtr sync_far(
            new FdsActorRequest(FAR_ID(FDSP_SyncTokenResp), synctok_resp));
    if (sync_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(sync_far);
}

void FDSP_MigrationPathRpc::
PushTokenMetadata(boost::shared_ptr<FDSP_PushTokenMetadataReq>& push_md_req) // NOLINT
{
    FdsActorRequestPtr push_md_far(
            new FdsActorRequest(FAR_ID(FDSP_PushTokenMetadataReq), push_md_req));
    if (push_md_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(push_md_far);
}

void FDSP_MigrationPathRpc::
PushTokenMetadataResp(boost::shared_ptr<FDSP_PushTokenMetadataResp>& push_md_resp) // NOLINT
{
    FdsActorRequestPtr push_md_far(
            new FdsActorRequest(FAR_ID(FDSP_PushTokenMetadataResp), push_md_resp));
    if (push_md_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(push_md_far);
}

void FDSP_MigrationPathRpc::
NotifyTokenSyncComplete(boost::shared_ptr<FDSP_NotifyTokenSyncComplete>& sync_complete) // NOLINT
{
    FdsActorRequestPtr sync_complete_far(
            new FdsActorRequest(FAR_ID(FDSP_NotifyTokenSyncComplete), sync_complete));
    if (sync_complete_far == nullptr) {
        LOGERROR << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(sync_complete_far);
}

TokenCopyTracker::TokenCopyTracker(FdsMigrationSvc *migrationSvc,
        std::set<fds_token_id> tokens, MigSvcCbType cb) {
    migrationSvc_ = migrationSvc;
    tokens_ = tokens;
    cur_itr_ = tokens.begin();
    cb_ = cb;
}

void TokenCopyTracker::start() {
    if (cur_itr_ != tokens_.end()) {
        issue_copy_req();
    }
}

void TokenCopyTracker::token_complet_cb(const Error& e) {
    fds_assert(e == ERR_OK);
    cur_itr_++;
    if (cur_itr_ != tokens_.end()) {
        issue_copy_req();
    } else {
        cb_(ERR_OK);
    }
}

void TokenCopyTracker::issue_copy_req() {
    LOGDEBUG << "token id: " << *cur_itr_;
    MigSvcCopyTokensReqPtr copy_req(new MigSvcCopyTokensReq());
    copy_req->token = *cur_itr_;
    // Send migration request to migration service
    copy_req->migsvc_resp_cb = std::bind(
            &TokenCopyTracker::token_complet_cb,
            this,
            std::placeholders::_1);
    FdsActorRequestPtr copy_far(new FdsActorRequest(
            FAR_ID(MigSvcCopyTokensReq), copy_req));
    migrationSvc_->send_actor_request(copy_far);
}

/**
 * Constructor
 * @param threadpool
 * @param conf_helper
 * @param log
 * @param nst
 */
FdsMigrationSvc::FdsMigrationSvc(SmIoReqHandler *data_store,
        const FdsConfigAccessor &conf_helper,
        fds_log *log, netSessionTblPtr nst,
        ClusterCommMgrPtr clust_comm_mgr)
    : Module("FdsMigrationSvc"),
      FdsRequestQueueActor("FdsMigrationSvc", nullptr),
      mig_cntrs("Migration", g_cntrs_mgr.get()),
      data_store_(data_store),
      conf_helper_(conf_helper),
      nst_(nst),
      clust_comm_mgr_(clust_comm_mgr)
{
    SetLog(log);
}

/**
 * Module start up code
 */
void FdsMigrationSvc::mod_startup()
{
    fds_threadpoolPtr threadpool(
            new fds_threadpool(conf_helper_.get<int>("thread_cnt")));
    FdsRequestQueueActor::init(threadpool);

    setup_migpath_server();

    migpath_session_->listenServerNb();
}

/**
 * Shutdown code
 */
void FdsMigrationSvc::mod_shutdown()
{
    nst_->endSession(migpath_session_->getSessionTblKey());
}

/**
 * Handler for this actor's requests
 * @param req
 * @return
 */
Error FdsMigrationSvc::handle_actor_request(FdsActorRequestPtr req)
{
    Error err = ERR_OK;
    switch (req->type) {
    case FAR_ID(MigSvcCopyTokensReq):
    {
        handle_migsvc_copy_token(req);
        break;
    }
    case FAR_ID(MigSvcBulkCopyTokensReq):
    {
        fds_assert(copy_tracker_ == nullptr);
        auto payload = req->get_payload<MigSvcBulkCopyTokensReq>();
        copy_tracker_.reset(new TokenCopyTracker(this,
                payload->tokens, payload->migsvc_resp_cb));
        copy_tracker_->start();
        break;
    }
    case FAR_ID(FdsActorShutdownComplete):
    {
        handle_migsvc_migration_complete(req);
        break;
    }
    /* Token copy related */
    case FAR_ID(FDSP_CopyTokenReq):
    {
        handle_migsvc_copy_token_rpc(req);
        break;
    }
    case FAR_ID(FDSP_PushTokenObjectsReq):
    {
        auto payload = req->get_payload<FDSP_PushTokenObjectsReq>();
        route_to_mig_actor(payload->header.mig_id, req);
        break;
    }
    case FAR_ID(FDSP_CopyTokenResp):
    {
        auto payload = req->get_payload<FDSP_CopyTokenResp>();
        route_to_mig_actor(payload->mig_id, req);
        break;
    }
    case FAR_ID(FDSP_PushTokenObjectsResp):
    {
        auto payload = req->get_payload<FDSP_PushTokenObjectsResp>();
        route_to_mig_actor(payload->mig_id, req);
        break;
    }
    /* Token sync related */
    case FAR_ID(FDSP_SyncTokenReq):
    {
        auto payload = req->get_payload<FDSP_SyncTokenReq>();
        route_to_mig_actor(payload->header.mig_id, req);
        break;
    }
    case FAR_ID(FDSP_SyncTokenResp):
    {
        auto payload = req->get_payload<FDSP_SyncTokenResp>();
        route_to_mig_actor(payload->header.mig_id, req);
        break;
    }
    case FAR_ID(FDSP_PushTokenMetadataReq):
    {
        auto payload = req->get_payload<FDSP_PushTokenMetadataReq>();
        route_to_mig_actor(payload->header.mig_id, req);
        break;
    }
    case FAR_ID(FDSP_PushTokenMetadataResp):
    {
        auto payload = req->get_payload<FDSP_PushTokenMetadataResp>();
        route_to_mig_actor(payload->header.mig_id, req);
        break;
    }
    case FAR_ID(FDSP_NotifyTokenSyncComplete):
    {
        auto payload = req->get_payload<FDSP_NotifyTokenSyncComplete>();
        route_to_mig_actor(payload->header.mig_id, req);
        break;
    }
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}

/**
 * Routes the request to migration actor.  Only send payloads with
 * FDSP_MigMsgHdrType as the header
 * @param mig_id
 * @param req
 */
void FdsMigrationSvc::route_to_mig_actor(const std::string &mig_id,
        FdsActorRequestPtr req)
{
    auto itr = mig_actors_.find(mig_id);
    if (itr == mig_actors_.end()) {
        /* For testing.  Remove when not needed */
        fds_assert(!"Migration actor not found");
        LOGWARN << "Migration actor id: " << mig_id
                << " disappeared";
        return;
    }
    itr->second.migrator->send_actor_request(req);
}

/**
 * Handles FAR_ID(MigSvcCopyTokens).  Creates TokenCopyReceiver and starts
 * the state machine for it
 * This request is typically result of local entity such as om client
 * prompting migraton service to copy tokens
 */
void FdsMigrationSvc::handle_migsvc_copy_token(FdsActorRequestPtr req)
{
    fds_assert(req->type == FAR_ID(MigSvcCopyTokensReq));

    auto copy_payload = req->get_payload<MigSvcCopyTokensReq>();

    std::string mig_id = fds::get_uuid();

    TokenCopyReceiver* copy_rcvr = new TokenCopyReceiver(this,
            data_store_, mig_id,
            threadpool_, GetLog(), copy_payload->token,
            migpath_handler_, clust_comm_mgr_);
    mig_actors_[mig_id].migrator.reset(copy_rcvr);
    mig_actors_[mig_id].migsvc_resp_cb = copy_payload->migsvc_resp_cb;

    LOGNORMAL << " New TokenCopyReceiver.  Migration id: " << mig_id;

    copy_rcvr->start();
}

/**
 * Handles FAR_ID(FDSP_CopyTokenReq).  Creates TokenCopySender and starts
 * the state machine for it
 * This is initiated by receiver requesting copy of tokens.
 */
void FdsMigrationSvc::handle_migsvc_copy_token_rpc(FdsActorRequestPtr req)
{
    fds_assert(req->type == FAR_ID(FDSP_CopyTokenReq));

    /* First acknowledge/accept the copy request */
    if (ack_copy_token_req(req) != ERR_OK) {
        LOGWARN << "Dropping Copy token request";
        return;
    }

    /* Start off the TokenCopySender state machine */
    auto copy_payload = req->get_payload<FDSP_CopyTokenReq>();
    std::string &mig_id = copy_payload->header.mig_id;
    std::string &mig_stream_id = copy_payload->header.mig_stream_id;
    std::string &rcvr_ip = copy_payload->header.base_header.src_node_name;
    int rcvr_port = copy_payload->header.base_header.src_port;
    std::set<fds_token_id> tokens(
            copy_payload->tokens.begin(), copy_payload->tokens.end());
    fds_assert(mig_id.size() > 0);
    fds_assert(mig_stream_id.size() > 0);
    fds_assert(rcvr_ip.size() > 0);
    fds_assert(tokens.size() > 0);

    TokenCopySender *copy_sender =
            new TokenCopySender(this, data_store_,
                    mig_id, mig_stream_id,
                    threadpool_, GetLog(),
                    rcvr_ip, rcvr_port,
                    tokens, migpath_handler_,
                    clust_comm_mgr_);
    mig_actors_[mig_id].migrator.reset(copy_sender);

    LOGNORMAL << " New sender.  Migration id: " << mig_id
            << " stream id: " << mig_stream_id
            << " receiver ip : " << rcvr_ip;

    copy_sender->start();
}

/**
 * Migration is complete.  Destroy the associated migrator
 * @param req
 */
void FdsMigrationSvc::
handle_migsvc_migration_complete(FdsActorRequestPtr req)
{
    fds_assert(req->type == FAR_ID(FdsActorShutdownComplete));

    auto payload = req->get_payload<FdsActorShutdownComplete>();
    auto itr = mig_actors_.find(payload->far_id);
    if (itr == mig_actors_.end()) {
        /* For testing.  Remove when not needed */
        fds_assert(!"Migration actor not found");
        LOGWARN << "Migration actor id: " << payload->far_id
                << " disappeared";
        return;
    }

    fds_verify(itr->second.migrator->get_queue_size() == 0);

    /* Remove and then issue the callback */
    auto migsvc_resp_cb = itr->second.migsvc_resp_cb;
    // TODO(rao): We need to remove the migrator.  I am experiencing crash
    // when I do this.  I suspect fds actor queue is still scheduled even
    // after removing the migration actor
    mig_actors_.erase(itr);

    if (migsvc_resp_cb) {
        migsvc_resp_cb(ERR_OK);
    }

    LOGNORMAL << " Migration id: " << payload->far_id;
}

/**
 * Sets up migration path server
 */
void FdsMigrationSvc::setup_migpath_server()
{
    migpath_handler_.reset(new FDSP_MigrationPathRpc(*this, GetLog()));

    std::string ip = netSession::getLocalIp();
    int port = conf_helper_.get<int>("port");
    int myIpInt = netSession::ipString2Addr(ip);
    // TODO(rao): Do not hard code.  Get from config
    std::string node_name = "localhost-mig";
    migpath_session_ = nst_->createServerSession<netMigrationPathServerSession>(
        myIpInt,
        port,
        node_name,
        FDSP_MIGRATION_MGR,
        migpath_handler_);

    LOGNORMAL << "Migration path server setup ip: "
              << ip << " port: " << port;
}

/**
 * Acknowledge FAR_ID(FDSP_CopyTokenReq) request.  Ideally this should
 * be part of TokenCopySender.  keeping it here for now.
 * @param req
 */
Error
FdsMigrationSvc::ack_copy_token_req(FdsActorRequestPtr req)
{
    Error err(ERR_OK);

    fds_assert(req->type == FAR_ID(FDSP_CopyTokenReq));
    auto copy_req = req->get_payload<FDSP_CopyTokenReq>();

    /* Send a respones back acking request.  In future we can do more things here */
    boost::shared_ptr<FDSP_CopyTokenResp> response(new FDSP_CopyTokenResp());
    std::string &session_id = copy_req->header.base_header.session_uuid;
    response->base_header.err_code = ERR_OK;
    response->base_header.src_node_name = get_ip();
    response->base_header.session_uuid = session_id;
    response->mig_id = copy_req->header.mig_id;
    response->mig_stream_id = copy_req->header.mig_stream_id;

    migpath_resp_client(session_id)->CopyTokenResp(response);

    LOGNORMAL << "Sent copy ack for mig " << response->mig_id;

    return err;
}

/**
 * Create a rpc client for migration path and returns it
 * @param ip
 * @param port
 */
netMigrationPathClientSession*
FdsMigrationSvc::get_migration_client(const std::string &ip, const int &port)
{
    /* Create a client rpc session from src to dst */
    netMigrationPathClientSession* rpc_session =
            nst_->startSession<netMigrationPathClientSession>(
                    ip,
                    port,
                    FDSP_MIGRATION_MGR,
                    1, /* number of channels */
                    migpath_handler_);
    return rpc_session;
}

/**
 * Return the response client associated with session_uuid
 * @param session_uuid
 * @return
 */
boost::shared_ptr<FDSP_MigrationPathRespClient>
FdsMigrationSvc::get_resp_client(const std::string &session_uuid)
{
    return migpath_session_->getRespClient(session_uuid);
}

/**
 *
 * @return
 */
ClusterCommMgrPtr FdsMigrationSvc::get_cluster_comm_mgr()
{
    return clust_comm_mgr_;
}

/**
 * return ip of migration service server
 * @return
 */
std::string FdsMigrationSvc::get_ip()
{
    return netSession::getLocalIp();
}

/**
 * return port of migration service
 * @return
 */
int FdsMigrationSvc::get_port()
{
    return conf_helper_.get<int>("port");
}
}  // namespace fds
