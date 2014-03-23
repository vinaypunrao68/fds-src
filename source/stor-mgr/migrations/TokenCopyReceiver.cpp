/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <list>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>
#include <vector>
#include <fdsp/FDSP_types.h>
#include <fds_migration.h>
#include <fds_timestamp.h>
#include <TokenCopyReceiver.h>
#include <TokenSyncReceiver.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;         // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* Statemachine events */
/* Triggered once response for copy tokens request is received */
typedef FDSP_CopyTokenResp CopyTokRespEvt;

/* Triggered when token data is received */
typedef FDSP_PushTokenObjectsReq TokRecvdEvt;

/* Triggered when token data has been committed data store */
struct TcrWrittenEvt {
    explicit TcrWrittenEvt(const std::string &mig_stream_id)
    : mig_stream_id_(mig_stream_id)
    {}

    std::string mig_stream_id_;
};
typedef boost::shared_ptr<TcrWrittenEvt> TcrWrittenEvtPtr;

/* Triggered when migration stream needs to be destroyed */
struct TRCopyDnEvt {
    explicit TRCopyDnEvt(const std::string &mig_stream_id,
            const fds_token_id& token_id)
    : mig_stream_id_(mig_stream_id),
      token_id_(token_id)
    {}

    std::string mig_stream_id_;
    fds_token_id token_id_;
};
typedef boost::shared_ptr<TRCopyDnEvt> TRCopyDnEvtPtr;

/* State machine */
struct TokenCopyReceiverFSM_ : public state_machine_def<TokenCopyReceiverFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopyReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &sender_ip,
            const int &sender_port,
            const fds_token_id &token,
            netMigrationPathClientSession *sender_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        sender_ip_ = sender_ip;
        sender_port_= sender_port;
        pending_tokens_.insert(token);
        sender_session_ = sender_session;
        client_resp_handler_ = client_resp_handler;

        /* Setting up the common properties of object store write request */
        objstor_write_req_.io_type = FDS_SM_WRITE_TOKEN_OBJECTS;
        objstor_write_req_.response_cb = std::bind(
            &TokenCopyReceiverFSM_::data_written_cb, this,
            std::placeholders::_1, std::placeholders::_2);
        DBG(write_in_progress_ = false);
    }

    virtual ~TokenCopyReceiverFSM_()
    {
        fds_assert(write_in_progress_ == false);
    }

    // Disable exceptions to improve performance
    typedef int no_exception_thrown;

    template <class Event, class FSM>
    void on_entry(Event const& , FSM&)
    {
        LOGDEBUG << "TokenCopyReceiver SM";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "TokenCopyReceiver SM";
    }

    /* The list of state machine states */
    struct Init : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Init State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Init State";
        }
    };
    struct Connecting: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Connecting State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Connecting State";
        }
    };
    struct Receiving : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Receiving State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Receiving State";
        }
    };
    struct Writing : public msm::front::state<>
    {
        typedef mpl::vector<TokRecvdEvt> deferred_events;

        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Writing State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Writing State";
        }
    };
    struct UpdatingTok : public msm::front::state<>
    {
        typedef mpl::vector<TokRecvdEvt> deferred_events;

        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "UpdatingTok";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "UpdatingTok";
        }
    };
    struct Complete : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Complete State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Complete State";
        }
    };


    /* Actions */
    struct connect
    {
        /* Connect to the receiver and issue Copy Tokens request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "connect " << fsm.mig_stream_id_;

            /* Issue copy tokens request */
            FDSP_CopyTokenReq copy_req;
            copy_req.header.base_header.err_code = ERR_OK;
            copy_req.header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            copy_req.header.base_header.src_port = fsm.migrationSvc_->get_port();
            copy_req.header.base_header.session_uuid =
                    fsm.sender_session_->getSessionId();
            copy_req.header.mig_id = fsm.parent_->get_mig_id();
            copy_req.header.mig_stream_id = fsm.mig_stream_id_;
            copy_req.tokens.assign(
                    fsm.pending_tokens_.begin(), fsm.pending_tokens_.end());
            // TODO(rao): Do not hard code
            copy_req.max_size_per_reply = 1 << 20;
            fsm.sender_session_->getClient()->CopyToken(copy_req);
        }
    };
    struct update_tok
    {
        /* Prepare objstor_write_req_ for next read request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "update_tok " << fsm.mig_stream_id_;

            fds_assert(fsm.pending_tokens_.size() > 0);
            if (fsm.write_tok_complete_) {
                fsm.pending_tokens_.erase(fsm.write_token_id_);
                fsm.completed_tokens_.push_back(fsm.write_token_id_);

                fsm.migrationSvc_->mig_cntrs.tokens_rcvd.incr();

                LOGNORMAL << "Token " << fsm.write_token_id_ << " received completely";
            }
        }
    };
    struct ack_tok
    {
        /* Acknowledge FDSP_PushTokenObjectsReq */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "ack_tok " << fsm.mig_stream_id_;

            auto resp_client = fsm.migrationSvc_->get_resp_client(
                    evt.header.base_header.session_uuid);
            FDSP_PushTokenObjectsResp resp;
            /* Just copying the header from request */
            resp = evt.header;
            resp_client->PushTokenObjectsResp(resp);
        }
    };
    struct write_tok
    {
        /* Issues write request to ObjectStore */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);

            LOGDEBUG << "write_tok " << fsm.mig_stream_id_;

            /* Book keeping */
            fsm.write_tok_complete_ = evt.complete;
            fsm.write_token_id_ = evt.token_id;

            /* Recycle objstor_write_req_ */
            fds_assert(fsm.objstor_write_req_.response_cb);
            fsm.objstor_write_req_.token_id = evt.token_id;
            // TODO(rao): We should std::move the object list here
            fsm.objstor_write_req_.obj_list = evt.obj_list;

            DBG(fsm.write_in_progress_ = true);
            err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &fsm.objstor_write_req_);
            if (err != fds::ERR_OK) {
                DBG(fsm.write_in_progress_ = false);
                fds_assert(!"Hit an error in enqueing");
                // TODO(rao): Put your selft in an error state
                LOGERROR << "Failed to enqueue to StorMgr.  Error: "
                        << err;
            }
        }
    };
    struct teardown
    {
        /* Tears down.  Notify parent we are done */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "teardown " << fsm.mig_stream_id_;

            TRCopyDnEvtPtr destroy_evt(new TRCopyDnEvt(fsm.mig_stream_id_,
                    *fsm.completed_tokens_.begin()));
            fsm.parent_->send_actor_request(
                    FdsActorRequestPtr(
                            new FdsActorRequest(FAR_ID(TRCopyDnEvt), destroy_evt)));
        }
    };

    /* Guards */
    struct more_toks
    {
        /* Returns true if there are token remaining to be received */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            return (fsm.pending_tokens_.size() > 0);
        }
    };
    /* Statemachine transition table.  Roughly the statemachine is
     * 1. request objects for bunch of token
     * 2. Recive data for a token
     * 3. Write the data for the received token
     * 4. Once all the token data is received, move on to receivng another
     *    token until no tokens are left.
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , msm_none       , Connecting , connect         , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Connecting , CopyTokRespEvt , Receiving  , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Receiving  , TokRecvdEvt    , Writing    , ActionSequence_
                                                    <mpl::vector<
                                                    ack_tok,
                                                    write_tok> >    , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Writing    , TcrWrittenEvt  , UpdatingTok, update_tok      , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< UpdatingTok, msm_none       , Receiving  , msm_none        , more_toks        >,
    Row< UpdatingTok, msm_none       , Complete   , teardown        , Not_<more_toks > >
    // +------------+----------------+------------+-----------------+------------------+
    >
    {
    };


    /* Replaces the default no-transition response */
    template <class FSM, class Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        fds_verify(!"Unexpected event");
    }

    /* the initial state of the player SM. Must be defined */
    typedef Init initial_state;

    /* Callback invoked once data's been read from Object store */
    void data_written_cb(const Error &e, SmIoPutTokObjectsReq *resp)
    {
        DBG(write_in_progress_ = false);
        if (e != ERR_OK) {
            fds_assert(!"Error in reading");
            LOGERROR << "Error: " << e;
            return;
        }

        TcrWrittenEvtPtr written_evt(new TcrWrittenEvt(mig_stream_id_));
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TcrWrittenEvt), written_evt)));
    }

    /* Migration stream id */
    std::string mig_stream_id_;

    /* Migration service */
    FdsMigrationSvc *migrationSvc_;

    /* Parent */
    TokenCopyReceiver *parent_;

    /* Sender ip */
    std::string sender_ip_;

    /* Sender port */
    int sender_port_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Tokens that are pending to be received */
    // TODO(Rao):  We don't receive tokens in bulk any more.  Shouldn't be set
    // anymore
    std::set<fds_token_id> pending_tokens_;

    /* Tokens for which receive is complete */
    std::list<fds_token_id> completed_tokens_;

    /* Write token */
    fds_token_id write_token_id_;

    /* Current token written is complete or not */
    bool write_tok_complete_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;

    /* Object store write request */
    SmIoPutTokObjectsReq objstor_write_req_;

    /* Indicates whether write is in progress */
    DBG(bool write_in_progress_);

    /* Sender session endpoint */
    netMigrationPathClientSession *sender_session_;
};  /* struct TokenCopyReceiverFSM_ */

TokenCopyReceiver::TokenCopyReceiver(FdsMigrationSvc *migrationSvc,
        SmIoReqHandler *data_store,
        const std::string &mig_id,
        fds_threadpoolPtr threadpool,
        fds_log *log,
        const fds_token_id &token,
        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler,
        ClusterCommMgrPtr clust_comm_mgr)
    : MigrationReceiver(mig_id),
      FdsRequestQueueActor(mig_id, migrationSvc, threadpool),
      token_id_(token),
      migrationSvc_(migrationSvc),
      log_(log),
      clust_comm_mgr_(clust_comm_mgr)
{
    DBG(destroy_sent_ = false);

    uint32_t int_ip;
    uint32_t sender_port;
    int ret = clust_comm_mgr_->get_node_mig_ip_port(token,
                                                    int_ip,
                                                    sender_port);
    // TODO(Rao): Handle this error
    fds_assert(ret == true);
    std::string sender_ip = netSessionTbl::ipAddr2String(int_ip);

    // TODO(Rao): Don't need mig_stream_id anymore remove it
    std::string mig_stream_id = std::to_string(token);

    auto sender_session = migrationSvc->get_migration_client(
            sender_ip,
            sender_port);
    if (sender_session == nullptr) {
        LOGERROR << "Failed to connect to " << sender_ip << " port: " << sender_port;
        fds_assert(!"Null session");
        // TODO(Rao): return an error back to parent
        return;
    }

    copy_fsm_.reset(new TokenCopyReceiverFSM());
    copy_fsm_->init(mig_stream_id,
                migrationSvc, this, data_store,
                sender_ip, sender_port,
                token,
                sender_session,
                client_resp_handler);

    sync_fsm_ = new TokenSyncReceiver();
    sync_fsm_->init(mig_stream_id,
                migrationSvc, this, data_store,
                sender_ip, sender_port,
                token,
                sender_session,
                client_resp_handler);

    LOGNORMAL << " New receiver stream.  Migration id: " << mig_id
            << "Stream id: " << mig_stream_id << " sender ip : " << sender_ip;
}

TokenCopyReceiver::~TokenCopyReceiver() {
    if (sync_fsm_) {
        delete sync_fsm_;
    }
}

/**
 * Start the statemachine for each stream in the statemachine
 */
void TokenCopyReceiver::start()
{
    copy_fsm_->start();
}

/**
 * Handler for actor requests
 * @param req
 * @return
 */
Error TokenCopyReceiver::handle_actor_request(FdsActorRequestPtr req)
{
    Error err = ERR_OK;
    FDSP_CopyTokenReqPtr copy_tok_req;
    switch (req->type) {
    /* Copy related */
    case FAR_ID(FDSP_CopyTokenResp):
    {
        auto payload = req->get_payload<FDSP_CopyTokenResp>();
        copy_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_PushTokenObjectsReq):
    {
        /* Posting TokRecvdEvt */
        auto payload = req->get_payload<FDSP_PushTokenObjectsReq>();
        copy_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(TcrWrittenEvt):
    {
        /* Notification event from obeject store that write is complete */
        auto payload = req->get_payload<TcrWrittenEvt>();
        copy_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(TRCopyDnEvt):
    {
        /* notify parent that copy is done*/
        migrationSvc_->send_actor_request(req);

        /* Starting token sync statemachine */
        // TODO(Rao): Get the sync start time from token db
        migrationSvc_->getTokenStateDb()->setTokenState(token_id_,
                kvstore::TokenStateInfo::SYNCING);
        sync_fsm_->process_event(TRStartEvt(get_fds_timestamp_ms(), 0));
        break;
    }
    /* Sync related */
    case FAR_ID(FDSP_SyncTokenResp):
    {
        auto payload = req->get_payload<FDSP_SyncTokenResp>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_PushTokenMetadataReq):
    {
        auto payload = req->get_payload<FDSP_PushTokenMetadataReq>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_NotifyTokenSyncComplete):
    {
        auto payload = req->get_payload<FDSP_NotifyTokenSyncComplete>();
        sync_fsm_->process_event(*payload);
        // TODO(rao): issue mark pull stop to pull fsm
        break;
    }
    case FAR_ID(TRPullDnEvt):
    {
        fds_assert(!"not impl");
        break;
    }
    case FAR_ID(TRMdAppldEvt):
    {
        auto payload = req->get_payload<TRMdAppldEvt>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(TSnapDnEvt):
    {
        auto payload = req->get_payload<TSnapDnEvt>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(TRResolveDnEvt):
    {
        sync_fsm_->process_event(TRResolveDnEvt());
        break;
    }
    case FAR_ID(TRSyncDnEvt):
    {
        // TODO(Rao): Move this to pull
        migrationSvc_->getTokenStateDb()->setTokenState(token_id_,
                kvstore::TokenStateInfo::IN_SYNC);
        /* Self message to destruct */
        req->recycle(FAR_ID(FdsActorShutdown), nullptr);
        this->send_actor_request(req);
        break;
    }
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}


/**
 * Send notification to parent to destroy this
 * @param mig_stream_id
 */
void TokenCopyReceiver::destroy_migration_stream(const std::string &mig_stream_id)
{
    LOGNORMAL << " mig_id: " << mig_id_ << " mig_stream_id: " << mig_stream_id;

    FdsActorRequestPtr far(new FdsActorRequest(
            FAR_ID(FdsActorShutdown), nullptr));

    Error err = this->send_actor_request(far);
    if (err != ERR_OK) {
        fds_assert(!"Failed to send message");
        LOGERROR << "Failed to send actor message.  Error: "
                << err;
    }
    DBG(destroy_sent_ = true);
}
} /* namespace fds */
