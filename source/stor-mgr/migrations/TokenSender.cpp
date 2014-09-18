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

#include <NetSession.h>
#include <fds_migration.h>
#include <TokenSender.h>
#include <TokenSyncSender.h>
#include <TokenPullSender.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* Statemachine Events */
/* Event for starting the sender side copy/sync/pull statemachine */
struct TSStart {};
/* Triggered after token data has been read from object store */
struct TokReadEvt{};
/* Triggered after token data has been sent to receiver */
struct TokSentEvt{};

/* State machine */
struct TokenCopySenderFSM_
        : public msm::front::state_machine_def<TokenCopySenderFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const std::set<fds_token_id> &tokens,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        rcvr_port_ = rcvr_port;
        token_id_ = *(tokens.begin());
        rcvr_session_ = rcvr_session;
        client_resp_handler_ = client_resp_handler;

        objstor_read_req_.token_id = token_id_;
        // TODO(Rao): Do not hard code
        objstor_read_req_.max_size = 16 * (2 << 20);
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


    struct read_tok
    {
        /* Issues read request to ObjectStore */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);

            LOGDEBUG << "read_tok.  token: " << fsm.token_id_;

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

            LOGDEBUG << "send_tok.  token: " << fsm.token_id_
                    << " size: " << fsm.push_tok_req_.obj_list.size();

            fsm.migrationSvc_->\
            mig_cntrs.tok_copy_sent.incr(fsm.push_tok_req_.obj_list.size());
        }
    };
    struct teardown
    {
        /* Tears down.  Notify parent we are done */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "teardown.  token: " << fsm.token_id_;
            LOGDEBUG << fsm.migrationSvc_->mig_cntrs.toString();
            // TODO(Rao): For now we will not send a shutdown to parent as
            // sync will start
            /*
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(FdsActorShutdown), nullptr));

            Error err = fsm.parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
            */
        }
    };

    /* Guards */
    struct more_to_read
    {
        /* Returns true if there are token remaining to be sent */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            bool ret = !(fsm.objstor_read_req_.itr.isEnd());
            LOGDEBUG << "token: " << fsm.token_id_ << " more_to_read?: " << ret;
            return ret;
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
    Row< Init       , msm_none       , Reading    , read_tok        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Reading    , TokReadEvt     , Sending    , send_tok        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , TokSentEvt     , Reading    , read_tok        , more_to_read     >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , TokSentEvt     , Complete   , teardown        , Not_<more_to_read>>
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
        LOGDEBUG << "token: " << token_id_;

        if (e != ERR_OK) {
            fds_assert(!"Error in reading");
            LOGERROR << "Error: " << e;
            return;
        }
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TcsDataReadDone), nullptr)));
    }

    /* Stream id.  Uniquely identifies the copy stream */
    std::string mig_stream_id_;

    /* Migration service reference */
    FdsMigrationSvc *migrationSvc_;

    /* Parent */
    TokenSender *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Token we are copying */
    fds_token_id token_id_;

    /* RPC request.  It's recycled for every push request */
    FDSP_PushTokenObjectsReq push_tok_req_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;

    /* Tracks the current read request with obj_store_ */
    SmIoGetTokObjectsReq objstor_read_req_;

    /* Sender session endpoint */
    netMigrationPathClientSession *rcvr_session_;
};  /* struct TokenCopySenderFSM_ */

TokenSender::TokenSender(FdsMigrationSvc *migrationSvc,
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
      FdsRequestQueueActor(mig_id, migrationSvc, threadpool),
      log_(log),
      clust_comm_mgr_(clust_comm_mgr)
{
    // TODO(Rao): Make copy and sync be based on one token and not
    // mutiple tokens
    fds_verify(tokens.size() == 1);

    auto rcvr_session = migrationSvc->get_migration_client(rcvr_ip, rcvr_port);
    if (rcvr_session == nullptr) {
        fds_assert(!"Null session");
        LOGERROR << "rcvr_session is null";
        // TODO(Rao): return an error back to parent
        return;
    }

    copy_fsm_.reset(new TokenCopySenderFSM());
    copy_fsm_->init(mig_stream_id, migrationSvc, this, data_store,
            rcvr_ip, rcvr_port, tokens, rcvr_session, client_resp_handler);

    sync_fsm_ = new TokenSyncSender();
    sync_fsm_->init(mig_stream_id, migrationSvc, this, data_store,
            rcvr_ip, rcvr_port, *tokens.begin(), rcvr_session, client_resp_handler);

    pull_fsm_ = new TokenPullSender();
    pull_fsm_->init(mig_stream_id, migrationSvc, this, data_store,
            rcvr_ip, rcvr_port, *tokens.begin(), rcvr_session, client_resp_handler);

    LOGNORMAL << " New sender stream.  Migration id: " << mig_id
            << "Stream id: " << mig_stream_id << " receiver ip : " << rcvr_ip;
}

TokenSender::~TokenSender()
{
    delete sync_fsm_;
    delete pull_fsm_;
}


void TokenSender::start()
{
    FdsActorRequestPtr far(new FdsActorRequest(
                        FAR_ID(TSStart), nullptr));
    send_actor_request(far);
}

Error TokenSender::handle_actor_request(FdsActorRequestPtr req)
{
    Error err = ERR_OK;

    switch (req->type) {
    case FAR_ID(TSStart):
    {
        copy_fsm_->start();
        sync_fsm_->start();
        pull_fsm_->start();
        break;
    }
    case FAR_ID(MigSvcSyncCloseReq):
    {
        /* Notification from migration service to close sync */
        auto payload = req->get_payload<MigSvcSyncCloseReq>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_PushTokenObjectsResp):
    {
        /* Posting TokSentEvt */
        copy_fsm_->process_event(TokSentEvt());
        break;
    }
    case FAR_ID(TcsDataReadDone):
    {
        /* Notification from datastore that token data has been read */
        copy_fsm_->process_event(TokReadEvt());
        break;
    }
    /* Sync related */
    case FAR_ID(FDSP_SyncTokenReq):
    {
        /* Sync token request from receiver */
        auto payload = req->get_payload<FDSP_SyncTokenReq>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_PushTokenMetadataResp):
    {
        /* Ack from receiver for the sync metadata that was pushed */
        auto payload = req->get_payload<FDSP_PushTokenMetadataResp>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(TSnapDnEvt):
    {
        /* Notification from object store that taking snapshot for a token is done */
        auto payload = req->get_payload<TSnapDnEvt>();
        sync_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_PullObjectsReq):
    {
        /* Pull objects request from receiver */
        auto payload = req->get_payload<FDSP_PullObjectsReq>();
        pull_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(TSDataReadEvt):
    {
        /* Notification from object store that pull data has been read */
        auto payload = req->get_payload<TSDataReadEvt>();
        pull_fsm_->process_event(*payload);
        break;
    }
    case FAR_ID(FDSP_NotifyTokenPullComplete):
    {
        /* Notification from receiver that pulling object is done.  Pull fsm
         * can teardown
         */
        auto payload = req->get_payload<FDSP_NotifyTokenPullComplete>();
        pull_fsm_->process_event(*payload);

        copy_fsm_->migrationSvc_->mig_cntrs.tok_sent.incr();

        /* Sync and pull is complete.
         * We can safely shut ourselves down now
         */
        req->recycle(FAR_ID(FdsActorShutdown), nullptr);
        this->send_actor_request(req);
        break;
    }
    default:
    {
        fds_assert(!"Invalid request");
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    }
    return err;
}
} /* namespace fds */
