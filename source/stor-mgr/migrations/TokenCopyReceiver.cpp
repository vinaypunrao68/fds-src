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

/* Statemachine events */
/* Triggered once response for copy tokens request is received */
struct ConnectRespEvt {};
/* Triggered when token data is received */
typedef FDSP_PushTokenObjectsReq TokRecvdEvt;
/* Triggered when token data has been committed data store */
struct TokWrittenEvt {};

/* State machine */
struct TokenCopyReceiverFSM_ : public state_machine_def<TokenCopyReceiverFSM_> {
    void init(FdsMigrationSvc *migrationSvc,
            TokenCopyReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &sender_ip,
            const int &sender_port,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        sender_ip_ = sender_ip;
        sender_port_= sender_port;
        pending_tokens_ = tokens;
        client_resp_handler_ = client_resp_handler;

        objstor_write_req_.io_type = FDS_SM_WRITE_TOKEN_OBJECTS;
        objstor_write_req_.response_cb = std::bind(
            &TokenCopyReceiverFSM_::data_written_cb, this,
            std::placeholders::_1, std::placeholders::_2);
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
            LOGDEBUG << "connect";

            /* First connect */
            fsm.sender_session_ = fsm.migrationSvc_->get_migration_client(fsm.sender_ip_,
                    fsm.sender_port_);
            if (fsm.sender_session_ == nullptr) {
                fds_assert(!"Null session");
                // TODO(rao:) Go into error state
                // LOGERROR << "sender_session is null";
                return;
            }

            /* Issue copy tokens request */
            FDSP_CopyTokenReq copy_req;
            copy_req.header.base_header.err_code = ERR_OK;
            copy_req.header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            copy_req.header.base_header.src_port = fsm.migrationSvc_->get_port();
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
            LOGDEBUG << "update_tok";

            fds_assert(fsm.pending_tokens_.size() > 0);
            if (fsm.write_tok_complete_) {
                fsm.pending_tokens_.erase(fsm.write_token_id_);
                fsm.completed_tokens_.push_back(fsm.write_token_id_);

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
            LOGDEBUG << "ack_tok";

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

            LOGDEBUG << "write_tok";

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
            LOGDEBUG << "teardown";

            MigSvcMigrationCompletePtr mig_complete(new MigSvcMigrationComplete());
            mig_complete->migration_id = fsm.parent_->get_migration_id();
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
    Row< Receiving  , TokRecvdEvt    , Writing    , ActionSequence_
                                                    <mpl::vector<
                                                    ack_tok,
                                                    write_tok> >    , msm_none         >,
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
            LOGERROR << "Error: " << e;
            return;
        }
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TcrDataWriteDone), nullptr)));
    }

    protected:
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

TokenCopyReceiver::TokenCopyReceiver(FdsMigrationSvc *migrationSvc,
        SmIoReqHandler *data_store,
        const std::string &migration_id,
        fds_threadpoolPtr threadpool,
        fds_logPtr log,
        const std::string &sender_ip,
        const int &sender_port,
        const std::set<fds_token_id> &tokens,
        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    : MigrationReceiver(migration_id),
      FdsRequestQueueActor(threadpool),
      log_(log)
{
    sm_.reset(new TokenCopyReceiverFSM());
    sm_->init(migrationSvc, this, data_store,
            sender_ip, sender_port, tokens, client_resp_handler);
}

TokenCopyReceiver::~TokenCopyReceiver() {
}

/**
 * Starts TokenCopyReceiver statemachine
 */
void TokenCopyReceiver::start()
{
    sm_->start();
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
