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
#include <TokenSender.h>
#include <TokenPullSender.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/* State machine */
struct TokenPullSenderFSM_
        : public msm::front::state_machine_def<TokenPullSenderFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        rcvr_port_ = rcvr_port;
        token_id_ = token_id;
        rcvr_session_ = rcvr_session;
        client_resp_handler_ = client_resp_handler;

        push_req_.header.base_header.err_code = ERR_OK;
        push_req_.header.base_header.src_node_name = migrationSvc_->get_ip();
        push_req_.header.base_header.src_port = migrationSvc_->get_port();
        push_req_.header.base_header.session_uuid =
                rcvr_session_->getSessionId();
        push_req_.header.mig_id = parent_->get_mig_id();
        push_req_.header.mig_stream_id = mig_stream_id_;

        accum_data_size_ = 0;

        readdata_resp_cb_ = std::bind(
            &TokenPullSenderFSM_::readdata_resp_cb, this,
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
        LOGDEBUG << "TokenPullSenderFSM_";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "TokenPullSenderFSM_";
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
    struct Complete : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& e, FSM& fsm)
        {
            LOGDEBUG << "Complete State";
        }
    };


    /* Actions */
    struct add_for_read
    {
        /* Add entries for pull */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            for (uint32_t i = 0; i < evt.obj_ids.size(); i++) {
                fsm.pending_.push_back(ObjectID(evt.obj_ids[i].digest));
            }

            LOGDEBUG << "add_for_read token: " << fsm.token_id_
                    << " cnt: " << evt.obj_ids.size()
                    << " pending: " << fsm.pending_.size()
                    << " inflight: " << fsm.inflight_.size();
        }
    };
    struct issue_reads
    {
        /* Issues reads.  Max of  MAX_INFLIGHT_READS are allowed to be
         * inflight
         */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            uint32_t cnt = 0;
            while (fsm.inflight_.size() <= MAX_INFLIGHT_READS &&
                   fsm.pending_.size() > 0) {
                auto read_data_msg = new SmIoReadObjectdata();
                read_data_msg->io_type = FDS_SM_READ_OBJECTDATA;
                assign(read_data_msg->obj_data.obj_id, fsm.pending_.front());
                read_data_msg->setObjId(fsm.pending_.front());
                read_data_msg->smio_readdata_resp_cb = fsm.readdata_resp_cb_;

                fsm.inflight_.insert(fsm.pending_.front());
                fsm.pending_.pop_front();

                Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId,
                        read_data_msg);
                if (err != fds::ERR_OK) {
                    fds_assert(!"Hit an error in enqueing");
                    LOGERROR << "Failed to enqueue to snap_msg_ to StorMgr.  Error: "
                            << err;
                    delete read_data_msg;
                    fsm.process_event(ErrorEvt());
                    return;
                }
                cnt++;
            }

            LOGDEBUG << "issue_reads token: " << fsm.token_id_
                    << "issued: " << cnt
                    << " pending: " << fsm.pending_.size()
                    << " inflight: " << fsm.inflight_.size();
        }
    };
    struct send_data
    {
        /* Response came back from object store.
         * Send read data, if we reached MAX_DATA_PACKET_SZ */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fsm.inflight_.erase(ObjectID(evt.obj_data.obj_id.digest));

            // TODO(Rao): Enable accumulation. For now send one object per message
            fsm.push_req_.obj_data_list.push_back(evt.obj_data);
            fsm.rcvr_session_->getClient()->PushObjects(fsm.push_req_);

            fsm.migrationSvc_->\
            mig_cntrs.tok_pull_obj_sent.incr(fsm.push_req_.obj_data_list.size());

            fsm.push_req_.obj_data_list.clear();

            LOGDEBUG << "send_data token: " << fsm.token_id_
                    << " pending: " << fsm.pending_.size()
                    << " inflight: " << fsm.inflight_.size();
            /*
            if (fsm.accum_data_size_ + evt.obj_data.data.size() >=
                    fsm.MAX_DATA_PACKET_SZ) {
                fsm.rcvr_session_->getClient()->PushObjects(fsm.push_req_);

                LOGDEBUG << "send_data: sent size: " << fsm.accum_data_size_;

                fsm.accum_data_size_ = 0;
            } else {
                fsm.accum_data_size_ += evt.size();
                fsm.push_req_.obj_data_list.push_back(evt.obj_data);

                LOGDEBUG << "send_data: accum_data_size: " << fsm.accum_data_size_;
            }*/
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
            fds_assert(fsm.pending_.size() == 0 && fsm.inflight_.size() == 0);

            LOGDEBUG << "teardown  token: " << fsm.token_id_;
            LOGDEBUG << fsm.migrationSvc_->mig_cntrs.toString();
        }
    };

    /* Receiver state machine for pull.  Psuedocode roughly as follows
     * while (pull_not_dn) {
     *  issue_reads()
     *  send()
     * }
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , msm_none       , Pulling    , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Pulling    , TSPullReqEvt   , msm_none   , ActionSequence_
                                                    <mpl::vector<
                                                    add_for_read,
                                                    issue_reads>>   , msm_none         >,

    Row< Pulling    , TSDataReadEvt  , msm_none   , ActionSequence_
                                                    <mpl::vector<
                                                    send_data,
                                                    issue_reads>>   , msm_none         >,


    Row< Pulling    , TSPullDnEvt    , Complete   , teardown        , msm_none         >,
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

    /* Callback from object store that data has been read */
    void readdata_resp_cb(const Error& e,
            SmIoReadObjectdata *read_data)
    {
        TSDataReadEvtPtr data_read_evt(read_data);
        FdsActorRequestPtr far(new FdsActorRequest(
                FAR_ID(TSDataReadEvt), data_read_evt));

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
    TokenSender *parent_;

    /* Receiver ip */
    std::string rcvr_ip_;

    /* Receiver port */
    int rcvr_port_;

    /* Sender session */
    netMigrationPathClientSession *rcvr_session_;

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

    /* Objects that are pending reads
     */
    std::list<ObjectID> pending_;

    /* Objects that are currently being read.  There is a limit on this */
    std::set<ObjectID> inflight_;

    /* We accumulate read data here.  Once we reach MAX_DATA_PACKET_SZ
     * we flush the data
     */
    FDSP_PushObjectsReq push_req_;

    /* Size of the accumulated data */
    uint32_t accum_data_size_;

    /* Apply object data callback */
    SmIoReadObjectdata::CbType readdata_resp_cb_;

    static const uint32_t MAX_INFLIGHT_READS = 20;
    static const uint32_t MAX_DATA_PACKET_SZ = 8 << 20;
};  /* struct TokenPullSenderFSM_ */

TokenPullSender::TokenPullSender()
{
    fsm_ = nullptr;
}

TokenPullSender::~TokenPullSender()
{
    delete fsm_;
}

void TokenPullSender::init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            const fds_token_id &token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler) {
    fsm_ = new TokenPullSenderFSM();
    fsm_->init(mig_stream_id, migrationSvc,
            parent,
            data_store,
            rcvr_ip,
            rcvr_port,
            token_id,
            rcvr_session,
            client_resp_handler);
}

void TokenPullSender::start()
{
    fsm_->start();
}

void TokenPullSender::process_event(const TSPullReqEvt& event)
{
    fsm_->process_event(event);
}

void TokenPullSender::process_event(const TSDataReadEvt& event)
{
    fsm_->process_event(event);
}

void TokenPullSender::process_event(const TSPullDnEvt& event)
{
    fsm_->process_event(event);
}
} /* namespace fds */
