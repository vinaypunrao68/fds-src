/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <list>
#include <vector>
#include <set>
#include <functional>

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/euml/operator.hpp>

#include <fds_migration.h>
#include <TokenCopyReceiver.h>
#include <TokenSyncReceiver.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* Statemachine internal events */
/* PullReceiverFSM events */
struct PullReqEvt {};
struct StopPullReqsEvt {};
struct DataPullDnEvt {};
struct PullFiniEvt {};

/* State machine */
struct TokenSyncReceiverFSM_
        : public msm::front::state_machine_def<TokenSyncReceiverFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopyReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *sender_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        rcvr_port_ = rcvr_port;
        token_id_ = token_id;
        sender_session_ = sender_session;
        client_resp_handler_ = client_resp_handler;

        smio_sync_md_resp_cb_ = std::bind(
            &TokenSyncReceiverFSM_::sync_apply_md_resp_cb, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

        sync_stream_done_ = false;
        pull_done_ = false;
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
        LOGDEBUG << "TokenSyncReceiverFSM_";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "TokenSyncReceiverFSM_";
    }

    /* The list of state machine states */
    struct AllOk : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM& ) {LOGDEBUG << "starting: AllOk";}
        template <class Event, class FSM>
        void on_exit(Event const&, FSM& ) {LOGDEBUG << "finishing: AllOk";}
    };
    /* this state is made terminal so that all the events are blocked */
    struct ErrorMode :  public msm::front::terminate_state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM& ) {LOGDEBUG << "starting: ErrorMode";}
        template <class Event, class FSM>
        void on_exit(Event const&, FSM& ) {LOGDEBUG << "finishing: ErrorMode";}
    };
    struct Init : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Init State";
        }
    };
    struct Starting : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Starting State";
        }
    };
    struct Receiving: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Receiving State";
        }
    };
    struct Resolving : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Resolving State";
        }
    };
    struct Complete : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& e, FSM& fsm)
        {
            LOGDEBUG << "Complete State";
        }
    };


    /* Actions */
    struct send_sync_req
    {
        /* Acknowledge to sender that token metadata is received */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "send_sync_req. token: " << fsm.token_id_;

            /* Issue copy tokens request */
            FDSP_SyncTokenReq sync_req;
            sync_req.header.base_header.err_code = ERR_OK;
            sync_req.header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            sync_req.header.base_header.src_port = fsm.migrationSvc_->get_port();
            sync_req.header.base_header.session_uuid =
                    fsm.sender_session_->getSessionId();
            sync_req.header.mig_id = fsm.parent_->get_mig_id();
            sync_req.header.mig_stream_id = fsm.mig_stream_id_;

            sync_req.token = fsm.token_id_;
            sync_req.start_time = evt.start_time;
            sync_req.end_time = evt.end_time;
            // TODO(rao): Do not hard code
            sync_req.max_entries_per_reply = 1024;
            fsm.sender_session_->getClient()->SyncToken(sync_req);
        }
    };

    struct ack_tok_md
    {
        /* Acknowledge to sender that token metadata is received */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "ack_tok_md. token: " << fsm.token_id_;

            auto resp_client = fsm.migrationSvc_->get_resp_client(
                    evt.header.base_header.session_uuid);
            FDSP_PushTokenMetadataResp resp;
            /* Just copying the header from FDSP_PushTokenMetadataReq */
            resp.header = evt.header;
            resp_client->PushTokenMetadataResp(resp);
        }
    };
    struct apply_tok_md
    {
        /* applies token metadata to ObjectStorMgr */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "apply_tok_md token: " << fsm.token_id_;

            for (auto itr : evt.md_list) {
                auto apply_md_msg = new SmIoApplySyncMetadata();
                apply_md_msg->io_type = FDS_SM_SYNC_APPLY_METADATA;
                apply_md_msg->md = *itr;
                apply_md_msg->smio_sync_md_resp_cb = fsm.smio_sync_md_resp_cb_;

                Error err = data_store_->enqueueMsg(FdsSysTaskQueueId, apply_md_msg);
                if (err != fds::ERR_OK) {
                    fds_assert(!"Hit an error in enqueing");
                    LOGERROR << "Failed to enqueue to snap_msg_ to StorMgr.  Error: "
                            << err;
                    fsm.process_event(ErrorEvt());
                    return;
                }
            }
        }
    };
    struct req_for_pull
    {
        /* Send a request to pull state machine for pull */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "req_for_pull token: " << fsm.token_id_;
            Error err(ERR_OK);
            // TODO(Rao):
        }
    };
    struct mark_sync_dn
    {
        /* Mark sync is complete */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "mark_sync_dn token: " << fsm.token_id_;

            fds_assert(!fsm.sync_stream_done_);
            fsm.sync_stream_done_ = true;

            /* If both sync and pull are complete, we'll start resolve process */
            if (fsm.sync_stream_done_ && fsm.pull_done_) {
                LOGDEBUG << "Token sync transfer complete.  token: " << fsm.token_id_;
                FdsActorRequestPtr far(new FdsActorRequest(
                        FAR_ID(TSXferDnEvt), nullptr));

                Error err = fsm.parent_->send_actor_request(far);
                if (err != ERR_OK) {
                    fds_assert(!"Failed to send message");
                    LOGERROR << "Failed to send actor message.  Error: "
                            << err;
                }
            }
        }
    };
    struct mark_pull_dn
    {
        /* Mark pull is complete */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "mark_pull_dn token: " << fsm.token_id_;

            fds_assert(!fsm.pull_done_);
            fsm.pull_done_= true;

            /* If both sync and pull are complete, we'll start resolve process */
            if (fsm.sync_stream_done_ && fsm.pull_done_) {
                LOGDEBUG << "Token sync transfer complete.  token: " << fsm.token_id_;
                FdsActorRequestPtr far(new FdsActorRequest(
                        FAR_ID(TSXferDnEvt), nullptr));

                Error err = fsm.parent_->send_actor_request(far);
                if (err != ERR_OK) {
                    fds_assert(!"Failed to send message");
                    LOGERROR << "Failed to send actor message.  Error: "
                            << err;
                }
            }
        }
    };
    struct start_resolve
    {
        /* starts resolve process for resolving client io entries with sync entries */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "start_resolve token: " << fsm.token_id_;

            /* Prepare object store message */
            SmIoResolveSyncEntries resolve_sync_msg;
            resolve_sync_msg.io_type = FDS_SM_SYNC_RESOLVE_SYNC_ENTRIES;
            resolve_sync_msg.token_id = fsm.token_id_;
            resolve_sync_msg.smio_resolve_resp_cb = std::bind(
                    &TokenSyncReceiverFSM_::resolve_sync_entries_cb, this,
                    std::placeholders::_1);

            Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &resolve_sync_msg);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                LOGERROR << "Failed to enqueue to resolve_sync_msg to StorMgr.  Error: "
                        << err;
                fsm.process_event(ErrorEvt());
                return;
            }
        }
    };

    struct report_error
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGERROR << "Error state token: " << fsm.token_id_;
            fds_assert(!"error");
        }
    };
    struct teardown
    {
        /* Tears down.  Notify parent we are done */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "teardown  token: " << fsm.token_id_;

            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(FdsActorShutdown), nullptr));

            Error err = fsm.parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
        }
    };

    /* Receiver state machine for sync.  It's roughly as follows
     * while (syn_not_complete) {
     *  recv_tok_metadata()
     *  apply_tok_metadata()
     *  pull_missing_objects()
     * }
     * resolve_sync_entries_with_client_io()
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , TSStartEvt     , Starting   , send_sync_req   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Starting   , SyncAckdEvt    , Receiving  , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Receiving  , TokMdEvt       , msm_none   , apply_tok_md    , msm_none         >,

    Row< Receiving  , TSMdAppldEvt   , msm_none   , ack_tok_md      , msm_none         >,

    Row< Receiving  , NeedPullEvt    , msm_none   , req_for_pull    , msm_none         >,

    Row< Receiving  , TSXferDnEvt    , msm_none   , mark_sync_dn    , msm_none         >,

    Row< Receiving  , PullDnEvt      , msm_none   , mark_pull_dn    , msm_none         >,

    Row< Receiving  , ResolveEvt     , Resolving  , start_resolve   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Resolving  , TSResolveDnEvt , Complete   , teardown        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row < AllOk     , ErrorEvt       , ErrorMode  , ActionSequence_
                                                    <mpl::vector<
                                                    report_error,
                                                    teardown> >     , msm_none         >
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
    typedef mpl::vector<Init, AllOk> initial_state;

    /* Callback from object store that apply sync metadata is complete */
    void sync_apply_md_resp_cb(const Error& e,
            SmIoApplySyncMetadata *sync_md,
            const std::set<ObjectID>& missing_objs)
    {
        fds_assert(pending_sync_md_msgs > 0);

        delete sync_md;

        pending_sync_md_msgs--;
        if (pending_sync_md_msgs == 0) {
            LOGDEBUG << "Applied a batch of token sync metadata";
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(TSMdAppldEvt), nullptr));

            Error err = parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
        }
        // TODO(Rao): Throw pull event if missing_objs exist
    }

    /* Callback from object store that resolving is complete */
    void resolve_sync_entries_cb(const Error& e)
    {
        LOGDEBUG << " token: " << token_id_;

        FdsActorRequestPtr far(new FdsActorRequest(
                FAR_ID(TSResolveDnEvt), nullptr));

        Error err = parent_->send_actor_request(far);
        if (err != ERR_OK) {
            fds_assert(!"Failed to send message");
            LOGERROR << "Failed to send actor message.  Error: "
                    << err;
        }
    }

    protected:
    /* Stream id.  Uniquely identifies the copy stream */
    std::string mig_stream_id_;

    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    /* Parent */
    TokenCopyReceiver *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* Sender session */
    netMigrationPathClientSession *sender_session_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;

    /* Token id we are syncing */
    fds_token_id token_id_;

    /* Current outstanding SmIoApplySyncMetadata messages issued against data_store_ */
    uint32_t pending_sync_md_msgs;

    /* Apply sync metadata callback */
    SmIoApplySyncMetadata::CbType smio_sync_md_resp_cb_;

    /* Whether is sync stream is complete */
    bool sync_stream_done_;

    /* Whether pull is complete */
    bool pull_done_;
};  /* struct TokenSyncReceiverFSM_ */

/* State machine for Pull sender */
struct PullReceiverFSM_
        : public msm::front::state_machine_def<PullReceiverFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopyReceiver *parent,
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
        client_resp_handler_ = client_resp_handler;
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
        LOGDEBUG << "PullReceiverFSM_";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "PullReceiverFSM_";
    }

    /* The list of state machine states */
    struct Init : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Init State";
        }
    };
    struct Pulling: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Pulling State";
        }
    };
    struct Complete : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& e, FSM& fsm)
        {
            LOGDEBUG << "Complete State";
        }
    };

    /* Actions */
    struct add_for_pull
    {
        /* Adds objects for pull */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "add_for_pull";
            // TODO(Rao): Add objects for pull
        }
    };

    struct issue_pull
    {
        /* Out of to be pulled objects, picks a few, and issues pulls */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "issue_pull";
            // TODO(Rao):  Issue only if there are pulls to be done
        }
    };
    struct mark_pull_stop
    {
        /* Marker to indicate that client will not issue pull requests */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);
            // TODO(Rao):
            LOGDEBUG << "mark_pull_stop";
        }
    };
    struct chk_cmpletion
    {
        /* Checks if pull is complete.  If complete throws PullFiniEvt */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            // TODO(Rao):
            LOGDEBUG << "chk_cmpletion";
        }
    };
    struct notify_cl_pulled
    {
        /* Notifies client of the data that has been pulled */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            // TODO(Rao):
            LOGDEBUG << "notify_cl_pulldn";
        }
    };

    struct notify_cl_pulldn
    {
        /* Notifies client that pull is complete */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            // TODO(Rao):
            LOGDEBUG << "notify_cl_pulldn";
        }
    };

    struct teardown
    {
        /* Tears down.  Notify parent we are done */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "teardown ";

            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(FdsActorShutdown), nullptr));

            Error err = fsm.parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
        }
    };

    /* Statemachine transition table.  Roughly the statemachine is
     * while (object_left_to_pull) {
     *  pull_from_remote();
     *  notify_client_of_pulledata();
     *  update_for_next_pull();
     * }
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , msm_none       , Pulling    , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Pulling    , PullReqEvt     , msm_none   , ActionSequence_
                                                    <mpl::vector<
                                                    add_for_pull,
                                                    issue_pull> >   , msm_none         >,

    Row< Pulling    , StopPullReqsEvt, msm_none   , ActionSequence_
                                                    <mpl::vector<
                                                    mark_pull_stop,
                                                    chk_cmpletion> >, msm_none         >,

    Row< Pulling    , DataPullDnEvt  , msm_none   , ActionSequence_
                                                    <mpl::vector<
                                                    notify_cl_pulled,
                                                    issue_pull,
                                                    chk_cmpletion> >, msm_none         >,

    Row< Pulling    , PullFiniEvt    , Complete   , ActionSequence_
                                                    <mpl::vector<
                                                    notify_cl_pulldn,
                                                    teardown> >     , msm_none         >
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

    protected:
    /* Stream id.  Uniquely identifies the copy stream */
    std::string mig_stream_id_;

    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    /* Parent */
    TokenCopyReceiver *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;
};  /* struct PullReceiverFSM_ */


TokenSyncReceiver::TokenSyncReceiver()
{
    fsm_ = nullptr;
}

TokenSyncReceiver::~TokenSyncReceiver()
{
    delete fsm_;
}

void TokenSyncReceiver::init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopyReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *sender_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler) {
    fsm_ = new TokenSyncReceiverFSM();
    fsm_->init(mig_stream_id, migrationSvc,
            parent,
            data_store,
            rcvr_ip,
            rcvr_port,
            token_id,
            sender_session,
            client_resp_handler);
}

void TokenSyncReceiver::process_event(const TSStartEvt& evt) {
    fsm_->process_event(evt);
}
} /* namespace fds */
