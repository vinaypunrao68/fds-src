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

        smio_sync_md_resp_cb_ = std::bind(
            &TokenSyncReceiverFSM_::sync_apply_md_resp_cb, this,
            std::placeholders::_1, std::placeholders::_2);

        sync_stream_done_ = false;

        sent_apply_md_msgs_ = 0;
        ackd_apply_md_msgs_ = 0;

        inflight_resolve_msgs_ = 0;

        db_ = nullptr;
        db_itr_ = nullptr;
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
    struct RecvDone: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "RecvDone State";
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
    struct apply_tok_md
    {
        /* applies token metadata to ObjectStorMgr */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "apply_tok_md token: " << fsm.token_id_
                    << ". cnt: " << evt.md_list.size();

            fsm.migrationSvc_->mig_cntrs.tok_sync_md_rcvd.incr(evt.md_list.size());

            /* Cache the session uuid */
            fsm.resp_session_uuid_ = evt.header.base_header.session_uuid;

            /* Increment upfront.  We compare this value against # acked back
             * from object store.  When they match we know this metadata batch
             * has been applied.  We increment upfront to avoid race conditions.
             */
            fsm.sent_apply_md_msgs_+= evt.md_list.size();

            uint32_t to_send = evt.md_list.size();
            for (auto itr : evt.md_list) {
                auto apply_md_msg = new SmIoApplySyncMetadata();
                apply_md_msg->io_type = FDS_SM_SYNC_APPLY_METADATA;
                apply_md_msg->md = itr;
                apply_md_msg->smio_sync_md_resp_cb = fsm.smio_sync_md_resp_cb_;

                Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, apply_md_msg);
                if (err != fds::ERR_OK) {
                    fds_assert(!"Hit an error in enqueing");
                    LOGERROR << "Failed to enqueue to snap_msg_ to StorMgr.  Error: "
                            << err;
                    delete apply_md_msg;
                    fsm.sent_apply_md_msgs_ -= to_send;
                    fsm.process_event(ErrorEvt());
                    return;
                }
                to_send--;
            }
        }
    };
    struct ack_tok_md
    {
        /* Acknowledge to sender that token metadata is received */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "ack_tok_md. token: " << fsm.token_id_;


            FDSP_PushTokenMetadataResp resp;
            resp.header.base_header.err_code = ERR_OK;
            resp.header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            resp.header.base_header.src_port = fsm.migrationSvc_->get_port();
            resp.header.base_header.session_uuid = fsm.resp_session_uuid_;
            resp.header.mig_id = fsm.parent_->get_mig_id();
            resp.header.mig_stream_id = fsm.mig_stream_id_;

            auto resp_client = fsm.migrationSvc_->get_resp_client(
                    fsm.resp_session_uuid_);
            resp_client->PushTokenMetadataResp(resp);
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
        }
    };
    struct take_snap
    {
        /* Takes snapshot of the token metada, so we can iterate and issue resolves */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "take_snap token: " << fsm.token_id_;

            SmIoSnapshotObjectDB* snap_msg = new SmIoSnapshotObjectDB();
            snap_msg->token_id = fsm.token_id_;
            snap_msg->io_type = FDS_SM_SNAPSHOT_TOKEN;
            snap_msg->smio_snap_resp_cb = std::bind(
                    &TokenSyncReceiverFSM_::snap_done_cb, &fsm,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4);

            Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, snap_msg);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                LOGERROR << "Failed to enqueue to snap_msg to StorMgr.  Error: "
                        << err;
                fsm.process_event(ErrorEvt());
                return;
            }
        }
    };

    struct issue_resolve
    {
        /* Issue resolve for bunch metadata entries from snapshot.  When
         * there is nothing to resolve throw TRResolveDnEvt
         */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            static uint32_t max_to_resolve = 10;
            std::list<ObjectID> resolve_list;

            fds_assert(fsm.inflight_resolve_msgs_ == 0);
            fsm.inflight_resolve_msgs_ = 0;

            /* Iterate the snapshot and resolve few entries at a time */
            leveldb::Iterator* it = fsm.db_itr_;
            ObjectID start_obj_id, end_obj_id;
            fsm.migrationSvc_->get_cluster_comm_mgr()->get_dlt()->\
                    getTokenObjectRange(fsm.token_id_, start_obj_id, end_obj_id);

            /* TODO(Rao): We should iterate just the token.  Now we are iterating
             * the whole db, which contains multiple tokens
             */
            for (; it->Valid() && resolve_list.size() < max_to_resolve; it->Next()) {
                ObjectID id(it->key().ToString());
                LOGDEBUG << "token: " << fsm.token_id_ << " range check id: " << id
                        << " start: " << start_obj_id << " end: " << end_obj_id;
                /* Range check */
                if (id < start_obj_id || id > end_obj_id) {
                    continue;
                }

                ObjMetaData md;
                SmObjDb::get_from_snapshot(it, md);
                if (md.syncDataExists()) {
                    resolve_list.push_back(id);
                }
            }
            fds_assert(it->status().ok());  // Check for any errors found during the scan

            /* Setting inflight_resolve_msgs_ prior to issuing object store requests
             * to avoid race between actor threads and sm response callback thread
             */
            fsm.inflight_resolve_msgs_ = resolve_list.size();

            if (resolve_list.size() == 0) {
                /* Nothing to resolve.  Simulate  TRResolveDnEvt to get state
                 * machine going
                 */
                FdsActorRequestPtr far(new FdsActorRequest(
                        FAR_ID(TRResolveDnEvt), nullptr));

                Error err = fsm.parent_->send_actor_request(far);
                if (err != ERR_OK) {
                    fds_assert(!"Failed to send message");
                    LOGERROR << "Failed to send actor message.  Error: "
                            << err;
                }
                LOGDEBUG << "issue_resolve token: " << fsm.token_id_
                        << " batch size: " << resolve_list.size();
                return;
            }

            /* Issue resolves one entry at a time.  If we want to issue bulk, we'll need
             * to lock the token db. Object store currently only supports serializing
             * requests at an object id level.
             */
            for (auto entry : resolve_list) {
                SmIoResolveSyncEntry *resolve_sync_msg = new SmIoResolveSyncEntry();
                resolve_sync_msg->io_type = FDS_SM_SYNC_RESOLVE_SYNC_ENTRY;
                resolve_sync_msg->object_id = entry;
                resolve_sync_msg->smio_resolve_resp_cb = std::bind(
                        &TokenSyncReceiverFSM_::resolve_sync_entries_cb, &fsm,
                        std::placeholders::_1, std::placeholders::_2);

                Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId,
                        resolve_sync_msg);
                if (err != fds::ERR_OK) {
                    fds_assert(!"Hit an error in enqueing SmIoResolveSyncEntry");
                    LOGERROR << "Failed to enqueue to resolve_sync_msg to "
                            << "StorMgr.  Error: " << err;
                    fsm.process_event(ErrorEvt());
                    return;
                }
            }

            LOGDEBUG << "issue_resolve token: " << fsm.token_id_
                    << " batch size: " << resolve_list.size();
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

            /* Cleanup snapshot taken during resolve */
            if (fsm.db_) {
                delete fsm.db_itr_;
                fsm.db_->ReleaseSnapshot(fsm.db_options_.snapshot);
            }

            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(TRSyncDnEvt), nullptr));

            Error err = fsm.parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
        }
    };

    /* Guards */
    struct resolve_dn
    {
        /* Returns true if we resolved all the sync entries */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            return !(fsm.db_itr_->Valid());
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
    Row< Init       , TRSyncStartEvt , Starting   , send_sync_req   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Starting   , SyncAckdEvt    , Receiving  , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Receiving  , TokMdEvt       , msm_none   , apply_tok_md    , msm_none         >,

    Row< Receiving  , TRMdAppldEvt   , msm_none   , ack_tok_md      , msm_none         >,

    Row< Receiving  , TRMdXferDnEvt  , RecvDone   , mark_sync_dn    , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< RecvDone   , msm_none       , Snapshot   , take_snap       , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Snapshot   , TSnapDnEvt     , Resolving  , issue_resolve   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Resolving  , TRResolveDnEvt , msm_none   , issue_resolve   , Not_<resolve_dn >>,

    Row< Resolving  , TRResolveDnEvt , Complete   , teardown        , resolve_dn       >,
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
            SmIoApplySyncMetadata *sync_md)
    {
        bool dataExists = sync_md->dataExists;
        ObjectID obj_id(sync_md->md.object_id.digest);

        delete sync_md;

        fds_assert(sent_apply_md_msgs_ > 0 &&
                   ackd_apply_md_msgs_ < sent_apply_md_msgs_);

        ackd_apply_md_msgs_++;

        if (ackd_apply_md_msgs_ == sent_apply_md_msgs_) {
            LOGDEBUG << "Applied a batch of token sync metadata. Cnt: "
                    << ackd_apply_md_msgs_;

            TRMdAppldEvtPtr md_applied_evt(new TRMdAppldEvt());
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(TRMdAppldEvt), md_applied_evt));

            Error err = parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
                return;
            }
        }

        /* For metadata without object data send a pull reques */
        if (!dataExists) {
            TRPullReqEvtPtr pull_evt(new TRPullReqEvt());
            pull_evt->pull_ids.push_back(obj_id);
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(TRPullReqEvt), pull_evt));

            Error err = parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
                return;
            }
        }
    }

    /* Callback from object store that metadata snapshot is complete */
    void snap_done_cb(const Error& e,
            SmIoSnapshotObjectDB* snap_msg,
            leveldb::ReadOptions& options, leveldb::DB* db)
    {
        LOGDEBUG << " token: " << token_id_;

        delete snap_msg;

        if (!e.ok()) {
            LOGERROR << "Error: " << e;
            msm::back::state_machine<TokenSyncReceiverFSM_> &fsm =
                    static_cast<msm::back::state_machine<TokenSyncReceiverFSM_> &>(*this);
            fsm.process_event(ErrorEvt());
            return;
        }

        /* Cache db snapshot info.  Thread safe here as only one thread
         * accesses db_itr_
         */
        db_options_ = options;
        db_ = db;
        db_itr_ = db->NewIterator(options);
        db_itr_->SeekToFirst();

        TSnapDnEvtPtr snap_dn_evt(new TSnapDnEvt(options, db));
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TSnapDnEvt), snap_dn_evt)));
    }

    /* Callback from object store that resolving is complete */
    void resolve_sync_entries_cb(const Error& e,
            SmIoResolveSyncEntry* resolve_entry)
    {
        LOGDEBUG << " token: " << token_id_;

        delete resolve_entry;

        fds_assert(inflight_resolve_msgs_ > 0);
        inflight_resolve_msgs_--;

        if (inflight_resolve_msgs_ == 0) {
            LOGDEBUG << "Applied a batch of resolve entries token: " << token_id_;
            FdsActorRequestPtr far(new FdsActorRequest(
                    FAR_ID(TRResolveDnEvt), nullptr));

            Error err = parent_->send_actor_request(far);
            if (err != ERR_OK) {
                fds_assert(!"Failed to send message");
                LOGERROR << "Failed to send actor message.  Error: "
                        << err;
            }
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

    /* Total count of SmIoApplySyncMetadata messages issued against data_store_ */
    uint32_t sent_apply_md_msgs_;

    /* Total count of SmIoApplySyncMetadata */
    uint32_t ackd_apply_md_msgs_;

    /* Apply sync metadata callback */
    SmIoApplySyncMetadata::CbType smio_sync_md_resp_cb_;

    /* Whether is sync stream is complete */
    bool sync_stream_done_;

    /* Current outstanding SmIoResolveSyncEntry messages issued against data_store_ */
    uint32_t inflight_resolve_msgs_;

    /* Token db snapshot info.  Used during resolve process */
    leveldb::DB *db_;
    leveldb::Iterator *db_itr_;
    leveldb::ReadOptions db_options_;
};  /* struct TokenSyncReceiverFSM_ */

#if 0
/* State machine for Pull sender */
struct PullReceiverFSM_
        : public msm::front::state_machine_def<PullReceiverFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenReceiver *parent,
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
            // TODO(Rao): Notify pull done
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
    TokenReceiver *parent_;

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
#endif

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
            TokenReceiver *parent,
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

void TokenSyncReceiver::start()
{
    fsm_->start();
}

void TokenSyncReceiver::process_event(const TRSyncStartEvt& evt) {
    fsm_->process_event(evt);
}

void TokenSyncReceiver::process_event(const SyncAckdEvt& evt) {
    fsm_->process_event(evt);
}

void TokenSyncReceiver::process_event(const TokMdEvt& evt) {
    fsm_->process_event(evt);
}

void TokenSyncReceiver::process_event(const TRMdAppldEvt& evt)
{
    fsm_->process_event(evt);
}

void TokenSyncReceiver::process_event(const TRMdXferDnEvt& evt) {
    fsm_->process_event(evt);
}

void TokenSyncReceiver::process_event(const TSnapDnEvt& evt) {
    fsm_->process_event(evt);
}

void TokenSyncReceiver::process_event(const TRResolveDnEvt& evt) {
    fsm_->process_event(evt);
}

} /* namespace fds */
