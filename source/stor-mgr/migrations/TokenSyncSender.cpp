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
struct StartEvt {};
struct SnapDnEvt {};
struct BldSyncLogDnEvt {};
struct SendDnEvt {};
struct IoClosedEvt {};
struct SyncDnAckEvt {};

/* State machine */
struct TokenSyncSenderFSM_
        : public msm::front::state_machine_def<TokenSyncSenderFSM_> {
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
            &TokenSyncSenderFSM_::data_read_cb, this,
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
        LOGDEBUG << "TokenSyncSenderFSM_";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "TokenSyncSenderFSM_";
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
    struct Snapshot: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Snapshot State";
        }
    };
    struct BldSyncLog : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "BldSyncLog State";
        }
    };
    struct Sending : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Sending State";
        }
    };
    struct WaitForClose : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "WaitForClose";
        }
    };
    struct FiniSync : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "FiniSync";
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
    struct take_snap
    {
        /* Connect to the receiver so we can push tokens */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "take_snap";
            // TODO(Rao): Send a message to take a leveldb snapshot
        }
    };

    struct build_sync_log
    {
        /* Prepare objstor_read_req_ for next read request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "build_sync_log";
            // TODO(Rao): Compute sync range.  Build a sync log
        }
    };
    struct send_sync_log
    {
        /* Issues read request to ObjectStore */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);
            LOGDEBUG << "send_sync_log";
        }
    };
    struct finish_sync
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "finish_sync";
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

    /* Guards */
    struct sync_log_dn
    {
        /* Returns true if we are finished sending the current sync log */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            // TODO(Rao): Implement
            return false;
        }
    };
    struct sync_closed
    {
        /* Returns true if client io has been closed  */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            return sync_closed_;
        }
    };
    struct sync_dn
    {
        /* Returns true sync is complete  */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            return sync_closed_ && (cur_sync_range_high_ >= sync_closed_time_);
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
    Row< Init       , StartEvt       , Snapshot   , take_snap       , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Snapshot   , SnapDnEvt      , BldSyncLog , build_sync_log  , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< BldSyncLog , BldSyncLogDnEvt, Sending    , send_sync_log   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , SendDnEvt      , Sending    , send_sync_log   , Not_<sync_log_dn>>,
    Row< Sending    , SendDnEvt      ,WaitForClose, msm_none        , Not_<sync_closed>  >,
    Row< Sending    , SendDnEvt      , Snapshot   , take_snap       , Not_<sync_dn>    >,
    Row< Sending    , SendDnEvt      , FiniSync   , finish_sync     , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< WaitForClose, IoClosedEvt   , Snapshot   , take_snap       , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< FiniSync   , SyncDnAckEvt   , Complete   , teardown        , msm_none         >
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

};  /* struct TokenSyncSenderFSM_ */

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
