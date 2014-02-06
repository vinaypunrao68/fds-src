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
FDSP_MigrationPathRpc(FdsMigrationSvc &mig_svc, fds_logPtr log) // NOLINT
    : mig_svc_(mig_svc),
      log_(log)
{
}

void FDSP_MigrationPathRpc::
CopyToken(boost::shared_ptr<FDSP_CopyTokenReq>& copy_req) // NOLINT
{
    FdsActorRequestPtr copy_far(new FdsActorRequest(FAR_MIG_COPY_TOKEN_RPC, copy_req));
    if (copy_far == nullptr) {
        FDS_PLOG_ERR(get_log()) << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(copy_far);
}

void FDSP_MigrationPathRpc::
PushTokenObjects(boost::shared_ptr<FDSP_PushTokenObjectsReq>& push_req) // NOLINT
{
    FdsActorRequestPtr push_far(
            new FdsActorRequest(FAR_MIG_PUSH_TOKEN_OBJECTS_RPC, push_req));
    if (push_far == nullptr) {
        FDS_PLOG_ERR(get_log()) << "Failed to allocate memory";
        return;
    }
    mig_svc_.send_actor_request(push_far);
}

void FDSP_MigrationPathRpc::
CopyTokenResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)  // NOLINT
{
    // Your implementation goes here
    printf("CopyTokenResp\n");
}

void FDSP_MigrationPathRpc::
PushTokenObjectsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)  // NOLINT
{
    // Your implementation goes here
    printf("PushTokenObjectsResp\n");
}

FdsMigrationSvc::FdsMigrationSvc(fds_threadpoolPtr threadpool,
        const FdsConfigAccessor &conf_helper,
        fds_logPtr log, netSessionTblPtr nst)
    : Module("FdsMigrationSvc"),
      FdsRequestQueueActor(threadpool),
      conf_helper_(conf_helper),
      log_(log),
      nst_(nst)
{
}

/**
 * Module start up code
 */
void FdsMigrationSvc::mod_startup()
{
    setup_migpath_server();
}

/**
 * Shutdown code
 */
void FdsMigrationSvc::mod_shutdown()
{
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
    case FAR_MIGSVC_COPY_TOKEN:
    {
        handle_migsvc_copy_token(req);
        break;
    }
    case FAR_MIG_COPY_TOKEN_RPC:
    {
        handle_migsvc_copy_token_rpc(req);
        break;
    }
    case FAR_MIG_PUSH_TOKEN_OBJECTS_RPC:
#if 0
        if (tok_receiver_ == nullptr) {
            fds_assert(!"tok_receiver is null");
            FDS_PLOG_WARN(get_log()) << "Token receiver deleted";
            break;
        }
        tok_receiver_->handle_actor_request(req);
#endif
        break;
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}

/**
 * Handles FAR_MIGSVC_COPY_TOKEN.  Creates TokenCopyReceiver and starts
 * the state machine for it
 */
void FdsMigrationSvc::handle_migsvc_copy_token(FdsActorRequestPtr req)
{
    fds_assert(req->type == FAR_MIGSVC_COPY_TOKEN);
    auto copy_payload = req->get_payload<MigSvcCopyTokensReq>();
    IpTokenTable token_tbl = get_ip_token_tbl(copy_payload->tokens);

    /* Create TokenCopyReceiver endpoint for each ip */
    for (auto  itr = token_tbl.begin(); itr != token_tbl.end();  itr++) {
        std::string migration_id = fds::get_uuid();
        std::string ip = itr->first;
        std::unique_ptr<TokenCopyReceiver> copy_rcvr(
                new TokenCopyReceiver(migration_id, threadpool_, log_,
                        nst_, ip, itr->second, migpath_handler_));
        FDS_PLOG_INFO(get_log()) << " New receiver for ip : " << ip;
        pending_mig_actors_[ip] = std::move(copy_rcvr);
        copy_rcvr->start();
    }
}

/**
 * Handles FAR_MIGSVC_COPY_TOKEN_RPC.  Creates TokenCopySender and starts
 * the state machine for it
 */
void FdsMigrationSvc::handle_migsvc_copy_token_rpc(FdsActorRequestPtr req)
{
    fds_assert(req->type == FAR_MIGSVC_COPY_TOKEN_RPC);

    /* First acknowledge/accept the copy request */
    if (ack_copy_token_req(req) != ERR_OK) {
        FDS_PLOG_WARN(get_log()) << "Dropping Copy token request";
    }

    /* Start off the TokenCopySender state machine */
    auto copy_payload = req->get_payload<FDSP_CopyTokenReq>();
    std::string &migration_id = copy_payload->header.migration_id;
    std::string &ip = copy_payload->header.base_header.src_node_name;
    std::set<fds_token_id> tokens(
            copy_payload->tokens.begin(), copy_payload->tokens.begin());
    fds_assert(migration_id.size() > 0);
    fds_assert(ip.size() > 0);
    fds_assert(tokens.size() > 0);

    std::unique_ptr<TokenCopySender> copy_sender(
            new TokenCopySender(migration_id, threadpool_, log_,
                    ip, tokens, migpath_handler_));
    pending_mig_actors_[ip] = std::move(copy_sender);

    copy_sender->start();
    FDS_PLOG_INFO(get_log()) << " New sender for ip : " << ip;
}

/**
 * Returns mapping between primary source ip->tokens
 * @param tokens
 * @return
 */
FdsMigrationSvc::IpTokenTable
FdsMigrationSvc::get_ip_token_tbl(const std::set<fds_token_id>& tokens)
{
    IpTokenTable tbl;
    // TODO(rao): Implement this
    return tbl;
}

/**
 * Sets up migration path server
 */
void FdsMigrationSvc::setup_migpath_server()
{
    migpath_handler_.reset(new FDSP_MigrationPathRpc(*this, log_));

    int myIpInt = netSession::ipString2Addr(conf_helper_.get<std::string>("ip"));
    // TODO(rao): Do not hard code.  Get from config
    std::string node_name = "localhost-mig";
    migpath_session_ = nst_->createServerSession<netMigrationPathServerSession>(
        myIpInt,
        conf_helper_.get<int>("migration_port"),
        node_name,
        FDSP_MIGRATION_MGR,
        migpath_handler_);

    FDS_PLOG(get_log()) << "Migration path server setup";
}

/**
 * Acknowledge FAR_MIG_COPY_TOKEN_RPC request.
 * @param req
 */
Error
FdsMigrationSvc::ack_copy_token_req(FdsActorRequestPtr req)
{
    Error err(ERR_OK);

    fds_assert(req->type == FAR_MIG_COPY_TOKEN);
    auto copy_token = req->get_payload<FDSP_CopyTokenReq>();

    /* Send a respones back acking request.  In future we can do more things here */
    FDSP_MsgHdrTypePtr response(new FDSP_MsgHdrType());
    std::string &session_id = copy_token->header.base_header.session_uuid;
    migpath_resp_client(session_id)->CopyTokenResp(response);

    return err;
}

/**
 * Acknowledge FAR_MIG_COPY_TOKEN request.  Create rpc client session from
 * sender to receiver
 * @param req
 */
netMigrationPathClientSession*
FdsMigrationSvc::get_migration_client(const std::string &ip)
{
    /* Create a client rpc session from src to dst */
    netMigrationPathClientSession* rpc_session =
            nst_->startSession<netMigrationPathClientSession>(
                    ip,
                    conf_helper_.get<int>("migration_port"),
                    FDSP_MIGRATION_MGR,
                    1, /* number of channels */
                    migpath_handler_);
    return rpc_session;
}

#if 0
void FdsMigrationSvc::sync_all_tokens()
{
    std::list<fds_token_id> tokens = dlt.get_my_tokens(); // NOLINT

    /* For now we expect the token migration table to be empty */
    fds_verify(get_tok_table_.size() == 0);

    for (auto tok : tokens) {
        get_tok_table_.add_token(tok);

        FDSP_MigrationPathReqClientPtr client = get_migration_endpoint(tok, 0);
        FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType());
        FDSP_MigrateTokenReqPtr migrate_req(new FDSP_MigrateTokenReq());

        // TODO(rao): Fill up the message

        Error err = client->MigrateToken(fdsp_msg, migrate_req);
        // TODO(rao): Handle the error by trying against a replica
        if (err != ERR_OK)
            fds_assert(!"Failed to issue migrate");
            FDS_PLOG_WARN(get_log()) << "Issuing migrate token failed.  Error: "
                    << err << " Req: " << migrate_req->log_string();
        }
        get_tok_table_.update_token_state(tok, TokenMigrationState::MIGRATE_ISSUED);
    }
}  // NOLINT

FDSP_MigrationPathReqClientPtr FdsMigrationSvc::
get_migration_endpoint(const fds_toke_id &tok_id, const int &idx)
{
    // TODO(rao): returnt the endpoint associated with idx
    return nullptr;
}

void FdsMigrationSvc::InitiateGetTokenObjects(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_MigrateTokenReq>& get_req)  // NOLINT
{
    /* Check token id doens't exist */
    if (put_tok_handlers_.get_handler(get_req->token_id) != nullptr) {
        fdsp_msg->err_code = ERR_MIGRATION_DUPLICATE_REQUEST;
        FDS_PLOG_WARN(get_log()) << " Token id: " << token_id << " Duplicate request";
    } else {
        PutTokenHandlerPtr handler(new PutTokenHandler(get_req->token_id, threadpool_));
        put_tok_table.add_token(get_req->toke_id, handler);
        fdsp_msg->err_code = handler->start();
        FDS_PLOG(get_log()) << " Token id: " << token_id;
    }

    if (fdsp_msg->err_code != ERR_OK) {
        FDS_PLOG_WARN(get_log()) << " Token id: " << token_id
                << " Erros: " << fdsp_msg->err_code;
    }

    resp_client(fdsp_msg->session_uuid)->InitiateGetTokenObjectsResp(fdsp_msg);
}

void FdsMigrationSvc::PutTokenObjects(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<PutTokenObjectsReq>& mig_put_req)  // NOLINT
{
    auto handler = get_tok_handlers_.get_handler(fdsp_msg->token_id);
    if (handler == nullptr) {
        fdsp_msg->err_code = ERR_MIGRATION_NO_HANDLER;
    } else {
        fdsp_msg->err_code = handler->put_token_objects();
    }

    if (fdsp_msg->err_code != ERR_OK) {
        FDS_PLOG_WARN(get_log()) << " Token id: " << token_id
                << " Erros: " << fdsp_msg->err_code;
    }
    resp_client(fdsp_msg->session_uuid)->PutTokenObjects(fdsp_msg);
}

void FdsMigrationSvc::
InitiateGetTokenObjectsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)  // NOLINT
{
    /* TODO: Handle errors */
}

void FdsMigrationSvc::
PutTokenObjectsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)  // NOLINT
{
    /* TODO: Handle any errors */
    /* Here source may tell us to throttle how much we are asking */
}
#endif
}  // namespace fds
