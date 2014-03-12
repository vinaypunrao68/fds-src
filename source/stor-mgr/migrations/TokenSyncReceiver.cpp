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

class TokenSyncLog {
public:

private:
};

/* Statemachine Events */
struct ErrorEvt {};
struct StartEvt {};
struct TokMetaDataEvt {};
struct NeedPullEvt {};
struct TSXferDnEvt {};
struct PullDnEvt {};
struct ResolveEvt {};
struct TSResolveDnEvt {};

/* State machine */
struct TokenSyncReceiverFSM_
        : public msm::front::state_machine_def<TokenSyncReceiverFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopySender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        rcvr_port_ = rcvr_port;
        token_id_ = token_id;
        client_resp_handler_ = client_resp_handler;

        apply_md_msg_.io_type = FDS_SM_SYNC_APPLY_METADATA;
        apply_md_msg_.smio_sync_md_resp_cb = std::bind(
            &TokenSyncReceiverFSM_::sync_apply_md_resp_cb, this,
            std::placeholders::_1, std::placeholders::_2);

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
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {LOGDEBUG << "starting: AllOk";}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {LOGDEBUG << "finishing: AllOk";}
    };
    /* this state is made terminal so that all the events are blocked */
    struct ErrorMode :  public msm::front::terminate_state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {LOGDEBUG << "starting: ErrorMode";}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {LOGDEBUG << "finishing: ErrorMode";}
    };
    struct Init : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Init State";
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
            /* Recyle apply_md_msg_ message */
            fds_assert(fsm.apply_md_msg_.smio_sync_md_resp_cb);

            fsm.apply_md_msg_.md_list.clear();
            fsm.apply_md_msg_.md_list = std::move(evt.md_list);
            evt.md_list.clear();
            Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &fsm.apply_md_msg_);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                LOGERROR << "Failed to enqueue to snap_msg_ to StorMgr.  Error: "
                        << err;
                fsm.process_event(ErrorEvt());
                return;
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
    Row< Init       , StartEvt       , Receiving  , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Receiving  , TokMetaDataEvt , Receiving  , ActionSequence_
                                                    <mpl::vector<
                                                    ack_tok_md,
                                                    apply_tok_md> > , msm_none         >,

    Row< Receiving  , NeedPullEvt    , Receiving  , req_for_pull    , msm_none         >,

    Row< Receiving  , TSXferDnEvt    , Receiving  , mark_sync_dn    , msm_none         >,

    Row< Receiving  , PullDnEvt      , Receiving  , mark_pull_dn    , msm_none         >,

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
            const std::set<ObjectID>& missing_objs)
    {
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
    TokenCopySender *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Token id we are syncing */
    fds_token_id token_id_;

    /* Apply sync metadata message to object store.  This message is recycled */
    SmIoApplySyncMetadata apply_md_msg_;

    /* Whether is sync stream is complete */
    bool sync_stream_done_;

    /* Whether pull is complete */
    bool pull_done_;

};  /* struct TokenSyncReceiverFSM_ */

/* PullReceiverFSM events */
struct PullReqEvt {};
struct StopPullReqsEvt {};
struct DataPullDnEvt {};
struct PullFiniEvt {};

/* State machine for Pull sender */
struct PullReceiverFSM_
        : public msm::front::state_machine_def<PullReceiverFSM_> {
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
            &PullReceiverFSM_::data_read_cb, this,
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
    Row< Pulling    , PullReqEvt     , Pulling    , ActionSequence_
                                                    <mpl::vector<
                                                    add_for_pull,
                                                    issue_pull> >   , msm_none         >,

    Row< Pulling    , StopPullReqsEvt, Pulling    , ActionSequence_
                                                    <mpl::vector<
                                                    mark_pull_stop,
                                                    chk_cmpletion> >, msm_none         >,

    Row< Pulling    , DataPullDnEvt  , Pulling    , ActionSequence_
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
    TokenCopySender *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Current sync range lower bound */
    uint64_t cur_sync_range_low_;

    /* Current sync range upper bound */
    uint64_t cur_sync_range_high_;

    /* Whether uppper bound on the sync range has been closed or not*/
    bool sync_closed_;

    /* Time at which sync was closed.  Once sync is closed this time becomes
     * cur_sync_range_high_ */
    uint64_t sync_closed_time_;

};  /* struct PullReceiverFSM_ */

//TokenCopySender::TokenCopySender(FdsMigrationSvc *migrationSvc,
//        SmIoReqHandler *data_store,
//        const std::string &mig_id,
//        const std::string &mig_stream_id,
//        fds_threadpoolPtr threadpool,
//        fds_log *log,
//        const std::string &rcvr_ip,
//        const int &rcvr_port,
//        const std::set<fds_token_id> &tokens,
//        boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler,
//        ClusterCommMgrPtr clust_comm_mgr)
//    : MigrationSender(mig_id),
//      FdsRequestQueueActor(mig_id, migrationSvc, threadpool),
//      log_(log),
//      clust_comm_mgr_(clust_comm_mgr)
//{
//    sm_.reset(new TokenCopySenderFSM());
//    sm_->init(mig_stream_id, migrationSvc, this, data_store,
//            rcvr_ip, rcvr_port, tokens, client_resp_handler);
//}
//
//TokenCopySender::~TokenCopySender()
//{
//}
//
//
//void TokenCopySender::start()
//{
//    sm_->start();
//}
//
//Error TokenCopySender::handle_actor_request(FdsActorRequestPtr req)
//{
//    Error err = ERR_OK;
//
//    switch (req->type) {
//    case FAR_ID(FDSP_PushTokenObjectsResp):
//        /* Posting TokSentEvt */
//        sm_->process_event(TokSentEvt());
//        break;
//    case FAR_ID(TcsDataReadDone):
//        /* Notification from datastore that token data has been read */
//        sm_->process_event(TokReadEvt());
//        break;
//    default:
//        err = ERR_FAR_INVALID_REQUEST;
//        break;
//    }
//    return err;
//}
} /* namespace fds */
