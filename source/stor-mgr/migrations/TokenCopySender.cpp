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

/* Statemachine Events */
/* Triggered after token data has been read from object store */
struct TokReadEvt{};
/* Triggered after token data has been sent to receiver */
struct TokSentEvt{};

/* State machine */
struct TokenCopySenderFSM_
        : public msm::front::state_machine_def<TokenCopySenderFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopySender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        rcvr_port_ = rcvr_port;
        pending_tokens_ = tokens;
        client_resp_handler_ = client_resp_handler;

        objstor_read_req_.io_type = FDS_SM_READ_TOKEN_OBJECTS;
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
        LOGDEBUG << "TokenCopySender SM";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "TokenCopySender SM";
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
    struct Reading : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Reading State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Reading State";
        }
    };
    struct Sending : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Sending State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
//            LOGDEBUG << "Sending State";
        }
    };
    struct UpdatingTok : public msm::front::state<>
    {
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
        void on_entry(Event const& e, FSM& fsm)
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
        /* Connect to the receiver so we can push tokens */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "connect";

            fsm.rcvr_session_ = fsm.migrationSvc_->get_migration_client(fsm.rcvr_ip_,
                    fsm.rcvr_port_);
            if (fsm.rcvr_session_ == nullptr) {
                fds_assert(!"Null session");
                // TODO(rao:) Go into error state
                LOGERROR << "rcvr_session is null";
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
            LOGDEBUG << "update_tok";

            if (fsm.pending_tokens_.size() == 0 &&
                fsm.objstor_read_req_.itr.isEnd()) {
                fds_assert(!"Invalid state action");
                return;
            }

            if (fsm.completed_tokens_.size() == 0 &&
                fsm.objstor_read_req_.itr.isBegin()) {
                /* First request */
                fsm.objstor_read_req_.token_id = *fsm.pending_tokens_.begin();
                // TODO(rao) : Dont hardcode
                fsm.objstor_read_req_.max_size = 2 << 20;

            } else if (fsm.objstor_read_req_.itr.isEnd()) {
                LOGNORMAL << "Token: " << *fsm.pending_tokens_.begin()
                        << " sent completely";
                /* We are done reading objects for current token */
                fsm.completed_tokens_.push_back(*fsm.pending_tokens_.begin());
                fsm.pending_tokens_.erase(fsm.pending_tokens_.begin());

                if (fsm.pending_tokens_.size() > 0) {
                    fsm.objstor_read_req_.token_id = *fsm.pending_tokens_.begin();
                }

                fsm.migrationSvc_->mig_cntrs.tokens_sent.incr();

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

            LOGDEBUG << "read_tok";

            /* Recyle objstor_read_req_ message */
            fds_assert(fsm.objstor_read_req_.response_cb);
            fsm.objstor_read_req_.obj_list.clear();

            err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &fsm.objstor_read_req_);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                // TODO(rao): Put your selft in an error state
                LOGERROR << "Failed to enqueue to StorMgr.  Error: "
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
            LOGDEBUG << "send_tok";

            /* Prepare FDSP_PushTokenObjectsReq */
            fsm.push_tok_req_.header.base_header.err_code = ERR_OK;
            fsm.push_tok_req_.header.base_header.src_node_name =
                    fsm.migrationSvc_->get_ip();
            fsm.push_tok_req_.header.base_header.session_uuid =
                    fsm.rcvr_session_->getSessionId();
            fsm.push_tok_req_.header.mig_id = fsm.parent_->get_mig_id();
            fsm.push_tok_req_.header.mig_stream_id = fsm.mig_stream_id_;
            fsm.push_tok_req_.obj_list.clear();
            fsm.push_tok_req_.token_id = fsm.objstor_read_req_.token_id;
            // TODO(rao): We should std::move here
            fsm.push_tok_req_.obj_list = fsm.objstor_read_req_.obj_list;
            fsm.push_tok_req_.complete = fsm.objstor_read_req_.itr.isEnd();

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
            LOGDEBUG << "teardown";

            MigSvcMigrationCompletePtr mig_complete(new MigSvcMigrationComplete());
            mig_complete->mig_id = fsm.parent_->get_mig_id();
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(MigSvcMigrationComplete), mig_complete));

            Error err = fsm.migrationSvc_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
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
            return (fsm.pending_tokens_.size() > 0);
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
    void no_transition(Event const& e, FSM& fsm, int state)
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
            LOGERROR << "Error: " << e;
            return;
        }
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TcsDataReadDone), nullptr)));
    }
    protected:
    /* Stream id.  Uniquely identifies the copy stream */
    std::string mig_stream_id_;

    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    /* Parent */
    TokenCopySender *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

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
        const std::string &mig_id,
        const std::string &mig_stream_id,
        fds_threadpoolPtr threadpool,
        fds_log *log,
        const std::string &rcvr_ip,
        const int &rcvr_port,
        const std::set<fds_token_id> &tokens,
        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler,
        ClusterCommMgrPtr clust_comm_mgr)
    : MigrationSender(mig_id),
      FdsRequestQueueActor(threadpool),
      log_(log),
      clust_comm_mgr_(clust_comm_mgr)
{
    sm_.reset(new TokenCopySenderFSM());
    sm_->init(mig_stream_id, migrationSvc, this, data_store,
            rcvr_ip, rcvr_port, tokens, client_resp_handler);
}

TokenCopySender::~TokenCopySender()
{
}


void TokenCopySender::start()
{
    sm_->start();
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
