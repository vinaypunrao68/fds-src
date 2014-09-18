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

#include <NetSession.h>
#include <fdsp_utils.h>
#include <SmObjDb.h>
#include <fds_migration.h>
#include <TokenReceiver.h>
#include <TokenPullReceiver.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* State machine */
struct TokenPullReceiverFSM_
        : public msm::front::state_machine_def<TokenPullReceiverFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenReceiver *parent,
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

        final_pullreq_issued_ = false;
        inflight_writes_ = 0;

        pulldata_written_resp_cb_ = std::bind(
            &TokenPullReceiverFSM_::pulldata_written_resp_cb, this,
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
        LOGDEBUG << "TokenPullReceiverFSM_";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "TokenPullReceiverFSM_";
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
    struct Pulling : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Pullling State";
        }
    };
    struct DataWritten : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "DataWritten State";
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
        /* Add entries for pull */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.pending_.insert(fsm.pending_.end(),
                    evt.pull_ids.begin(), evt.pull_ids.end());

            LOGDEBUG << "add_for_pull token: " << fsm.token_id_
                    << " cnt: " << evt.pull_ids.size()
                    << " pending_pulls: " << fsm.pending_.size()
                    << " inflight_pulls: " << fsm.inflight_.size();
        }
    };
    struct issue_pulls
    {
        /* Out of pending pull set, issue pulls.  Issues pull only if there
         * are entries to be pulled
         */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            /* Issue pull objects request */
            FDSP_PullObjectsReq pull_req;
            pull_req.header.base_header.err_code = ERR_OK;
            pull_req.header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            pull_req.header.base_header.src_port = fsm.migrationSvc_->get_port();
            pull_req.header.base_header.session_uuid =
                    fsm.sender_session_->getSessionId();
            pull_req.header.mig_id = fsm.parent_->get_mig_id();
            pull_req.header.mig_stream_id = fsm.mig_stream_id_;

            int32_t pull_cnt = fsm.MAX_INFLIGHT_PULLS - fsm.inflight_.size();
            fds_assert(pull_cnt >= 0);
            for (int32_t i = 0; i < pull_cnt ; i++) {
                if (fsm.pending_.size() == 0) {
                    break;
                }
                auto e = fsm.pending_.front();
                fsm.pending_.pop_front();
                fsm.inflight_.insert(e);

                FDS_ObjectIdType id;
                assign(id, e);
                pull_req.obj_ids.push_back(id);
            }

            LOGDEBUG << "issue_pulls. token: " << fsm.token_id_
                    << " cnt: " << pull_req.obj_ids.size()
                    << " pending_pulls: " << fsm.pending_.size()
                    << " inflight_pulls: " << fsm.inflight_.size();

            if (pull_req.obj_ids.size() > 0) {
                fsm.sender_session_->getClient()->PullObjects(pull_req);
            }
        }
    };
    struct issue_writes
    {
        /* Issues writes on pulled data */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.migrationSvc_->\
            mig_cntrs.tok_pull_obj_rcvd.incr(evt.obj_data_list.size());

            /* Increment upfront to avoid race between issuing message to ObjStore
             * and response coming back from ObjStore on a different thread.
             */
            fsm.inflight_writes_ += evt.obj_data_list.size();

            for (uint32_t idx = 0; idx < evt.obj_data_list.size(); idx++) {
                ObjectID obj_id(evt.obj_data_list[idx].obj_id.digest);

                fsm.inflight_.erase(obj_id);

                auto apply_data_msg = new SmIoApplyObjectdata();
                apply_data_msg->io_type = FDS_SM_APPLY_OBJECTDATA;
                apply_data_msg->obj_id = obj_id;
                // TODO(Rao): Use std:;move here
                apply_data_msg->obj_data = evt.obj_data_list[idx].data;
                apply_data_msg->smio_apply_data_resp_cb = fsm.pulldata_written_resp_cb_;

                Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId,
                        apply_data_msg);
                if (err != fds::ERR_OK) {
                    fds_assert(!"Hit an error in enqueing");
                    LOGERROR << "Failed to enqueue to snap_msg_ to StorMgr.  Error: "
                            << err;
                    delete apply_data_msg;
                    fsm.inflight_writes_ -= (evt.obj_data_list.size() - idx);
                    fsm.process_event(ErrorEvt());
                    return;
                }
            }

            LOGDEBUG << "issue_writes token: " << fsm.token_id_
                    << ". cnt: " << evt.obj_data_list.size()
                    << " inflight writes: " << fsm.inflight_writes_
                    << " pending_pulls: " << fsm.pending_.size()
                    << " inflight_pulls: " << fsm.inflight_.size();
        }
    };
    struct mark_data_written
    {
        /* Mark that we don't have pending writes into Object store */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fds_assert(fsm.inflight_writes_ > 0);
            fsm.inflight_writes_--;
        }
    };
    struct mark_last_pull
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "mark_last_pull  token: " << fsm.token_id_;
            fsm.final_pullreq_issued_ = true;
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
            LOGDEBUG << fsm.migrationSvc_->mig_cntrs.toString();

            /* Notify sender pull is complete */
            FDSP_NotifyTokenPullComplete complete_req;
            complete_req.header.base_header.err_code = ERR_OK;
            complete_req.header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            complete_req.header.base_header.src_port = fsm.migrationSvc_->get_port();
            complete_req.header.base_header.session_uuid =
                    fsm.sender_session_->getSessionId();
            complete_req.header.mig_id = fsm.parent_->get_mig_id();
            complete_req.header.mig_stream_id = fsm.mig_stream_id_;
            fsm.sender_session_->getClient()->NotifyTokenPullComplete(complete_req);


            /* Notify parent pull is complete */
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(TRPullDnEvt), nullptr));

            Error err = fsm.parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
        }
    };

    /* Guards */
    struct pull_dn
    {
        /* Returns true if pull is complete */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            bool ret = (fsm.pending_.size() == 0 &&
                    fsm.inflight_.size() == 0 &&
                    fsm.inflight_writes_ == 0 &&
                    fsm.final_pullreq_issued_ == true);
            LOGDEBUG << "token: " << fsm.token_id_ << " pull_dn?: " << ret;
            return ret;
        }
    };
    struct pending_pulls
    {
        /* Returns true if there are pending pulls */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            bool ret = (fsm.pending_.size() > 0 ||
                        fsm.inflight_.size() > 0 ||
                        fsm.inflight_writes_ > 0);
            LOGDEBUG << "token: " << fsm.token_id_ << " pending_pulls?: " << ret;
            return ret;
        }
    };

    /* Receiver state machine for pull.  Psuedocode roughly as follows
     * while (pull_not_dn) {
     *  issue_pulls()
     *  apply_pulleddata_to_disk()
     * }
     * resolve_sync_entries_with_client_io()
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , msm_none       , Pulling    , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Pulling    , TRPullReqEvt   , msm_none   , ActionSequence_
                                                    <mpl::vector<
                                                    add_for_pull,
                                                    issue_pulls>>   , msm_none         >,

    Row< Pulling    ,TRLastPullReqEvt, msm_none   , mark_last_pull  , pending_pulls    >,    // NOLINT

    Row< Pulling    ,TRLastPullReqEvt, Complete   , teardown        , Not_<pending_pulls>>,  // NOLINT

    Row< Pulling    , TRDataPulledEvt, msm_none   , issue_writes    , msm_none         >,

    Row< Pulling    , TRPullDataWrittenEvt
                                     , DataWritten, mark_data_written, msm_none        >,

    Row< DataWritten, msm_none       , Pulling    , issue_pulls     , Not_<pull_dn>    >,

    Row< DataWritten, msm_none       , Complete   , teardown        , pull_dn          >,
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

    /* Callback from object store that pulled data is written */
    void pulldata_written_resp_cb(const Error& e,
            SmIoApplyObjectdata *sync_md)
    {
        /* TODO(Rao):  We should send notification that write completed in batches.
         * So that subsequent write reqs are also issued in batches.  This is being
         * to avoid race conditions
         */
        TRPullDataWrittenEvtPtr data_applied_evt(sync_md);
        FdsActorRequestPtr far(new FdsActorRequest(
                FAR_ID(TRPullDataWrittenEvt), data_applied_evt));

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
    TokenReceiver *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* Sender session */
    netMigrationPathClientSession *sender_session_;

    /* We cache session uuid of the incoming request so we can respond
     * back on the same socket
     */
    std::string resp_session_uuid_;

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;

    /* Token id we are syncing */
    fds_token_id token_id_;

    /* Objects that are pending pull.  Right now this is a set.
     * We may need a disk backed datastructure here. If the number
     * of objects that need a pull grows large.
     */
    std::list<ObjectID> pending_;

    /* Object ids inflight of pull */
    std::set<ObjectID> inflight_;

    /* Final pull request is issued or not.  Once final pull request is issued
     * and all the objects are pulled then we can notify the client pull is
     * complete
     */
    bool final_pullreq_issued_;

    /* Current inflight writes */
    uint32_t inflight_writes_;

    /* Apply object data callback */
    SmIoApplyObjectdata::CbType pulldata_written_resp_cb_;

    static const uint32_t MAX_INFLIGHT_PULLS = 100;
};  /* struct TokenPullReceiverFSM_ */

TokenPullReceiver::TokenPullReceiver()
{
    fsm_ = nullptr;
}

TokenPullReceiver::~TokenPullReceiver()
{
    delete fsm_;
}

void TokenPullReceiver::init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenReceiver *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *sender_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler) {
    fsm_ = new TokenPullReceiverFSM();
    fsm_->init(mig_stream_id, migrationSvc,
            parent,
            data_store,
            rcvr_ip,
            rcvr_port,
            token_id,
            sender_session,
            client_resp_handler);
}

void TokenPullReceiver::start()
{
    fsm_->start();
}

void TokenPullReceiver::process_event(const TRPullReqEvt& evt) {
    fsm_->process_event(evt);
}

void TokenPullReceiver::process_event(const TRDataPulledEvt& evt) {
    fsm_->process_event(evt);
}

void TokenPullReceiver::process_event(const TRPullDataWrittenEvt& evt) {
    fsm_->process_event(evt);
}

void TokenPullReceiver::process_event(const TRLastPullReqEvt& evt) {
    fsm_->process_event(evt);
}
} /* namespace fds */
