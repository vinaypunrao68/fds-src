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
#include <TokenCopyReceiver.h>


namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;         // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* For logging purpose */
static inline fds_log* get_log() {
    return g_migrationSvc->get_log();
}
static inline std::string log_string()
{
    return "TokenCopySender";
}

/* Statemachine events */
/* Triggered once response for copy tokens request is received */
struct ConnectRespEvt {};
/* Triggered when token data is received */
typedef FDSP_PushTokenObjectsReq TokRecvdEvt;
/* Triggered when token data has been committed data store */
struct TokWrittenEvt {};

/* State machine */
struct TokenCopyReceiverFSM_ : public state_machine_def<TokenCopyReceiverFSM_> {
    void init(TokenCopyReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &sender_ip,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        parent_ = parent;
        data_store_ = data_store;
        sender_ip_ = sender_ip;
        pending_tokens_ = tokens;
        client_resp_handler_ = client_resp_handler;
        objstor_write_req_.response_cb = std::bind(
            &TokenCopyReceiverFSM_::data_written_cb, this,
            std::placeholders::_1, std::placeholders::_2);
    }

    // Disable exceptions to improve performance
    typedef int no_exception_thrown;

    template <class Event, class FSM>
    void on_entry(Event const& , FSM&)
    {
        // FDS_PLOG(get_log()) << "TokenCopyReceiver SM";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        // FDS_PLOG(get_log()) << "TokenCopyReceiver SM";
    }

    /* The list of state machine states */
    struct Init : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Init State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Init State";
        }
    };
    struct Connecting: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Connecting State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Connecting State";
        }
    };
    struct Receiving : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Receiving State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Receiving State";
        }
    };
    struct Writing : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Writing State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Writing State";
        }
    };
    struct UpdatingTok : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "UpdatingTok";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "UpdatingTok";
        }
    };
    struct Complete : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Complete State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Complete State";
        }
    };


    /* Actions */
    struct connect
    {
        /* Connect to the receiver and issue Copy Tokens request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            /* First connect */
            fsm.sender_session_ = g_migrationSvc->get_migration_client(fsm.sender_ip_);
            if (fsm.sender_session_ == nullptr) {
                fds_assert(!"Null session");
                // TODO(rao:) Go into error state
                // FDS_PLOG_ERR(get_log()) << "sender_session is null";
                return;
            }

            /* Issue copy tokens request */
            FDSP_CopyTokenReq copy_req;
            copy_req.header.base_header.err_code = ERR_OK;
            copy_req.header.base_header.src_node_name = g_migrationSvc->get_ip();
            copy_req.header.base_header.session_uuid =
                    fsm.sender_session_->getSessionId();
            copy_req.header.migration_id = fsm.parent_->get_migration_id();
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
            fds_assert(pending_tokens_.size() > 0);
            if (fsm.write_tok_complete_) {
                fsm.pending_tokens_.erase(fsm.write_token_id_);
                fsm.completed_tokens_.push_back(fsm.write_token_id_);
            }
        }
    };
    struct write_tok
    {
        /* Issues write request to ObjectStore */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);

            /* Book keeping */
            fsm.write_tok_complete_ = evt.complete;
            fsm.write_token_id_ = evt.token_id;

            /* Recycle objstor_write_req_ */
            fds_assert(fsm.objstor_write_req_.response_cb);
            fsm.objstor_write_req_.token_id = evt.token_id;
            // TODO(rao): We should std::move the object list here
            fsm.objstor_write_req_.obj_list = evt.obj_list;

            err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &fsm.objstor_write_req_);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                // TODO(rao): Put your selft in an error state
                FDS_PLOG_ERR(get_log()) << "Failed to enqueue to StorMgr.  Error: "
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
            MigSvcMigrationCompletePtr mig_complete(new MigSvcMigrationComplete());
            mig_complete->migration_id = fsm.parent_->get_migration_id();
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(MigSvcMigrationComplete), mig_complete));

            Error err = g_migrationSvc->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                FDS_PLOG_ERR(get_log()) << "Failed to send actor message.  Error: "
                        << err;
            }
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
    Row< Connecting , ConnectRespEvt , Receiving  , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Receiving  , TokRecvdEvt    , Writing    , write_tok       , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Writing    , TokWrittenEvt  , UpdatingTok, update_tok      , msm_none         >,
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
        if (e != ERR_OK) {
            fds_assert(!"Error in reading");
            FDS_PLOG_ERR(get_log()) << "Error: " << e;
            return;
        }
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TcrDataWriteDone), nullptr)));
    }

    protected:
    /* Parent */
    TokenCopyReceiver *parent_;

    /* Sender ip */
    std::string sender_ip_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Tokens that are pending to be received */
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

    /* Sender session endpoint */
    netMigrationPathClientSession *sender_session_;
};  /* struct TokenCopyReceiverFSM_ */

TokenCopyReceiver::TokenCopyReceiver(SmIoReqHandler *data_store,
        const std::string &migration_id,
        fds_threadpoolPtr threadpool,
        fds_logPtr log,
        const std::string &sender_ip,
        const std::set<fds_token_id> &tokens,
        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    : MigrationReceiver(migration_id),
      FdsRequestQueueActor(threadpool),
      log_(log)
{
    sm_.reset(new TokenCopyReceiverFSM());
    sm_->init(this, data_store, sender_ip, tokens, client_resp_handler);
}

TokenCopyReceiver::~TokenCopyReceiver() {
}

/**
 * Starts TokenCopyReceiver statemachine
 */
void TokenCopyReceiver::start()
{
    sm_->start();
#if 0
    sm_->process_event(ConnectEvt);
#endif
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
    case FAR_ID(FDSP_CopyTokenResp):
        sm_->process_event(ConnectRespEvt());
        break;
    case FAR_ID(FDSP_PushTokenObjectsReq):
        /* Posting TokRecvdEvt */
        sm_->process_event(*req->get_payload<FDSP_PushTokenObjectsReq>());
        break;
    case FAR_ID(TcrDataWriteDone):
        /* Notification event from obeject store that write is complete */
        sm_->process_event(TokWrittenEvt());
        break;
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}
} /* namespace fds */
