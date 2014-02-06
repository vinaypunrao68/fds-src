/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <set>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>
#include <vector>
#include <StorMgr.h>
#include <fdsp/FDSP_types.h>
#include <TokenCopyReceiver.h>


namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;         // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* State machine */
struct TokenCopyReceiverFSM_ : public state_machine_def<TokenCopyReceiverFSM_> {
    void init(netSessionTblPtr nst,
            const std::string &sender_ip,
            const std::set<fds_token_id> &tokens,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        nst_ = nst;
        sender_ip_ = sender_ip;
        pending_tokens_ = tokens;
        client_resp_handler_ = client_resp_handler;
        cur_token_complete_ = false;
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

    std::string log_string()
    {
        return "TokenCopyReceiver Statemachine";
    }

    /* Events */
    typedef FDS_ProtocolInterface::FDSP_PushTokenObjectsReq DataReceivedEvt;
    struct DataWrittenEvt {};

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
        typedef mpl::vector<DataReceivedEvt> deferred_events;

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
    struct issue_copy_token
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            FDSP_CopyTokenReq copy_req;
            // TODO(rao): populate heaer with other necessary fields
            copy_req.header.base_header.session_uuid =
                    fsm.sender_session_->getSessionId();
            copy_req.header.base_header.err_code = ERR_OK;
            copy_req.tokens.assign(
                    fsm.pending_tokens_.begin(), fsm.pending_tokens_.end());
            // TODO(rao): Do not hard code
            copy_req.max_size_per_reply = 1 << 20;
            fsm.sender_session_->getClient()->CopyToken(copy_req);
        }
    };
    struct write
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
#if 0
            Error err(ERR_OK);

            SmIoPutTokObjectsReq *ioReq = new SmIoPutTokObjectsReq();
            ioReq->token_id = fsm.tokens_[fsm.cur_token_];
            // TODO(rao): We should std::move the object list here
            ioReq->obj_list = evt.obj_list;

            err = fsm.obj_store_->enqueueMsg(0/* TODO(rao): FdsSysTaskQueueId*/, ioReq);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                // TODO(rao): Put your selft in an error state
            }

            fsm.cur_token_complete_ = evt.complete;
#endif
        }
    };
    struct incr_cur_token
    {
#if 0
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.cur_token_++;
            fds_assert(fsm.cur_token_ <= fsm.tokens_size()-1);
        }
#endif
    };
    struct tear_down
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            // TODO(rao): Notify completion migration svc
        }
    };

    /* Guards */
    struct cur_token_complete
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
#if 0
            return fsm.cur_token_complete_;
#endif
        }
    };
    struct more_tokens_left
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
#if 0
            return fsm.cur_token_ < fsm.tokens_.size()-1;
#endif
        }
    };

    /* transition table */
    struct transition_table : mpl::vector< // NOLINT
    //   Start       Event              Next            Action                      Guard
    Row <Init,       msm_none,          Receiving,      issue_copy_token,           msm_none>, // NOLINT
    Row <Receiving,  DataReceivedEvt,   Writing,        write,                      msm_none >, // NOLINT
    Row <Writing,    DataWrittenEvt,    Receiving,      msm_none,                   Not_<cur_token_complete> >, // NOLINT
    Row <Writing,    DataWrittenEvt,    Receiving,      ActionSequence_<
                                                            mpl::vector<
                                                            incr_cur_token,
                                                            issue_copy_token> >,    And_<cur_token_complete, more_tokens_left> >, // NOLINT
    Row <Writing,    DataWrittenEvt,    Complete,       tear_down,                  Not_<more_tokens_left> > // NOLINT
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

    protected:
    /* Net session table */
    netSessionTblPtr nst_;

    /* Sender ip */
    std::string sender_ip_;

    /* Tokens that are pending copy request */
    std::set<fds_token_id> pending_tokens_;

    /* Current token that we are receiving */
    int cur_token_;

    /* Data for the current token is completely received or not */
    bool cur_token_complete_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Object store reference */
    ObjectStorMgr *obj_store_;

    /* Sender session endpoint */
    netMigrationPathClientSession *sender_session_;
};  /* struct TokenCopyReceiverFSM_ */

TokenCopyReceiver::TokenCopyReceiver(const std::string &migration_id,
        fds_threadpoolPtr threadpool,
        fds_logPtr log,
        netSessionTblPtr nst,
        const std::string &sender_ip,
        const std::set<fds_token_id> &tokens,
        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    : MigrationReceiver(migration_id),
      FdsRequestQueueActor(threadpool),
      log_(log)
{
    sm_.reset(new TokenCopyReceiverFSM());
    sm_->init(nst, sender_ip, tokens, client_resp_handler);
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
    FDSP_PushTokenObjectsReqPtr push_tok_req;

    switch (req->type) {
#if 0
    case FAR_MIG_PUSH_TOKEN_OBJECTS:
        push_tok_req = boost::static_pointer_cast<FDSP_PushTokenObjectsReq>(req->payload);
        sm_->process_event(*push_tok_req);
        break;
#endif
    case FAR_OBJSTOR_TOKEN_OBJECTS_WRITTEN:
        // TODO(rao) : Implement this
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}
} /* namespace fds */
