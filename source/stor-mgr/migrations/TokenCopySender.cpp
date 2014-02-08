/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <list>
#include <set>
#include <functional>

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/euml/operator.hpp>

#include <fds_migration.h>
#include <TokenCopySender.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* For logging purpose */
extern fds_log* g_fdslog;
static inline fds_log* get_log() {
    return g_fdslog;
}
static inline std::string log_string()
{
    return "TokenCopySender";
}

/* Statemachine Events */
/* Triggered after token data has been read from object store */
struct TokReadEvt{};
/* Triggered after token data has been sent to receiver */
struct TokSentEvt{};

/* State machine */
struct TokenCopySenderFSM_
        : public msm::front::state_machine_def<TokenCopySenderFSM_> {
    void init(FdsMigrationSvc *migrationSvc,
            TokenCopySender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        pending_tokens_ = tokens;
        client_resp_handler_ = client_resp_handler;
        objstor_read_req_.response_cb = std::bind(
            &TokenCopySenderFSM_::data_read_cb, this,
            std::placeholders::_1, std::placeholders::_2);
    }
    // To improve performance --- if no message queue needed
    // message queue not needed if no action will itself generate
    // a new event
    typedef int no_message_queue;

    // To improve performance -- if behaviors will never throw
    // an exception, below shows how to disable exception handling
    typedef int no_exception_thrown;

    template <class Event, class FSM>
    void on_entry(Event const& , FSM&)
    {
        // FDS_PLOG(get_log()) << "TokenCopySender SM";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        // FDS_PLOG(get_log()) << "TokenCopySender SM";
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
    struct Reading : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Reading State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Reading State";
        }
    };
    struct Sending : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Sending State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Sending State";
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
        /* Connect to the receiver so we can push tokens */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.rcvr_session_ = fsm.migrationSvc_->get_migration_client(fsm.rcvr_ip_);
            if (fsm.rcvr_session_ == nullptr) {
                fds_assert(!"Null session");
                // TODO(rao:) Go into error state
                FDS_PLOG_ERR(get_log()) << "rcvr_session is null";
                return;
            }
        }
    };
    struct update_tok
    {
        /* Prepare objstor_read_req_ for next read request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            if (fsm.pending_tokens_.size() == 0 &&
                fsm.objstor_read_req_.itr.isDone()) {
                fds_assert(!"Invalid state action");
                return;
            }

            if (fsm.completed_tokens_.size() == 0 &&
                fsm.objstor_read_req_.itr.isDone()) {
                /* First request */
                auto first = fsm.pending_tokens_.begin();
                fsm.objstor_read_req_.token_id = *first;
                fsm.pending_tokens_.erase(first);
                // TODO(rao) : Dont hardcode
                fsm.objstor_read_req_.max_size = 2 << 20;

            } else if (fsm.objstor_read_req_.itr.isDone()) {
                /* We are done reading objects for current token */
                auto first = fsm.pending_tokens_.begin();
                fsm.objstor_read_req_.token_id = *first;
                fsm.completed_tokens_.push_back(*first);
                fsm.pending_tokens_.erase(first);

            } else {
                /* Still processing the current token.  Nothing to do*/
            }
        }
    };
    struct read_tok
    {
        /* Issues read request to ObjectStore */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);

            /* Recyle objstor_read_req_ message */
            fds_assert(fsm.objstor_read_req_.response_cb);
            fsm.objstor_read_req_.obj_list.clear();

            err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &fsm.objstor_read_req_);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                // TODO(rao): Put your selft in an error state
                FDS_PLOG_ERR(get_log()) << "Failed to enqueue to StorMgr.  Error: "
                        << err;
            }
        }
    };
    struct send_tok
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            /* Prepare FDSP_PushTokenObjectsReq */
            fsm.push_tok_req_.header.base_header.err_code = ERR_OK;
            fsm.push_tok_req_.header.base_header.src_node_name =
                    fsm.migrationSvc_->get_ip();
            fsm.push_tok_req_.header.base_header.session_uuid =
                    fsm.rcvr_session_->getSessionId();
            fsm.push_tok_req_.header.migration_id = fsm.parent_->get_migration_id();
            fsm.push_tok_req_.obj_list.clear();
            fsm.push_tok_req_.token_id = fsm.objstor_read_req_.token_id;
            // TODO(rao): We should std::move here
            fsm.push_tok_req_.obj_list = fsm.objstor_read_req_.obj_list;
            fsm.push_tok_req_.complete = fsm.objstor_read_req_.itr.isDone();

            /* Push to receiver */
            fsm.rcvr_session_->getClient()->PushTokenObjects(fsm.push_tok_req_);
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

            Error err = fsm.migrationSvc_->send_actor_request(far);
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
        /* Returns true if there are token remaining to be sent */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            return (fsm.pending_tokens_.size() > 0 ||
                    !fsm.objstor_read_req_.itr.isDone());
        }
    };

    /* Statemachine transition table.  Roughly the statemachine is read objects for
     * a token and send those objects.  Once a full token is pushed move to the next
     * token.  Repeat this process until there aren't any tokens left.
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , msm_none       , Connecting , connect         , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Connecting , msm_none       , UpdatingTok, update_tok      , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< UpdatingTok, msm_none       , Reading    , read_tok        , more_toks        >,
    Row< UpdatingTok, msm_none       , Complete   , teardown        , Not_<more_toks > >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Reading    , TokReadEvt     , Sending    , send_tok        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , TokSentEvt     , UpdatingTok, update_tok      , msm_none         >
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
    void data_read_cb(const Error &e, SmIoGetTokObjectsReq *resp)
    {
        if (e != ERR_OK) {
            fds_assert(!"Error in reading");
            FDS_PLOG_ERR(get_log()) << "Error: " << e;
            return;
        }
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TcsDataReadDone), nullptr)));
    }
    protected:
    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    /* Parent */
    TokenCopySender *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Tokens that are pending send */
    std::set<fds_token_id> pending_tokens_;

    /* Tokens for which send is complete */
    std::list<fds_token_id> completed_tokens_;

    /* RPC request.  It's recycled for every push request */
    FDSP_PushTokenObjectsReq push_tok_req_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;

    /* Tracks the current read request with obj_store_ */
    SmIoGetTokObjectsReq objstor_read_req_;

    /* Sender session endpoint */
    netMigrationPathClientSession *rcvr_session_;
};  /* struct TokenCopySenderFSM_ */

TokenCopySender::TokenCopySender(FdsMigrationSvc *migrationSvc,
        SmIoReqHandler *data_store,
        const std::string &migration_id,
        fds_threadpoolPtr threadpool,
        fds_logPtr log,
        const std::string &rcvr_ip,
        const std::set<fds_token_id> &tokens,
        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    : MigrationSender(migration_id),
      FdsRequestQueueActor(threadpool),
      log_(log)
{
    sm_.reset(new TokenCopySenderFSM());
    sm_->init(migrationSvc, this, data_store,
            rcvr_ip, tokens, client_resp_handler);
}

TokenCopySender::~TokenCopySender()
{
}


void TokenCopySender::start()
{
}

Error TokenCopySender::handle_actor_request(FdsActorRequestPtr req)
{
    Error err = ERR_OK;

    switch (req->type) {
    case FAR_ID(FDSP_PushTokenObjectsResp):
        /* Posting TokSentEvt */
        sm_->process_event(TokSentEvt());
        break;
    case FAR_ID(TcsDataReadDone):
        /* Notification from datastore that token data has been read */
        sm_->process_event(TokReadEvt());
        break;
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}
} /* namespace fds */
