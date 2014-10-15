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

#include <leveldb/db.h>
#include <leveldb/comparator.h>
#include <leveldb/env.h>

#include <NetSession.h>
#include <fdsp_utils.h>
#include <SmObjDb.h>
#include <fds_timestamp.h>
#include <fds_migration.h>
#include <TokenSender.h>
#include <TokenSyncSender.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;


TokenSyncLog::TokenSyncLog(const std::string& name) {
    name_ = name;
    cnt_ = 0;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, name, &db_);
    LOGDEBUG << "Opening tokendb: " << name;
    assert(status.ok());
}

TokenSyncLog::~TokenSyncLog() {
    LOGDEBUG << "Destryoing tokendb: " << name_;
    delete db_;

    leveldb::Status status  = leveldb::DestroyDB(name_, leveldb::Options());
    fds_assert(status.ok());
}

Error TokenSyncLog::add(const ObjectID& id, const ObjMetaData &entry) {
    fds::Error err(fds::ERR_OK);

    ObjectBuf buf;
    entry.serializeTo(buf);

    std::string k = create_key(id, entry);
    leveldb::Slice key(k);
    leveldb::Slice value(buf.getData(), buf.getSize());
    leveldb::Status status = db_->Put(write_options_, key, value);

    if (!status.ok()) {
        fds_assert(!"Failed to write");
        LOGERROR << "Failed to write key: " << id;
        err = fds::Error(fds::ERR_DISK_WRITE_FAILED);
    } else {
        cnt_++;
    }

    return err;
}

size_t TokenSyncLog::size() {
    return cnt_;
}

leveldb::Iterator* TokenSyncLog::iterator() {
    return db_->NewIterator(leveldb::ReadOptions());
}

/* Extracts objectid and entry from the iterator */
void TokenSyncLog::parse_iterator(leveldb::Iterator* itr,
        ObjectID &id, ObjMetaData &entry)
{
    uint64_t ts;
    parse_key(itr->key(), ts, id);
    entry.deserializeFrom(itr->value());
}

std::string TokenSyncLog::create_key(const ObjectID& id, const ObjMetaData& entry)
{
    std::ostringstream oss;
    std::string str_id((const char*) id.GetId(), id.GetLen());
    fds_assert(str_id.size() == id.GetLen());
    oss << entry.getModificationTs()<< "\n" << str_id;
    return oss.str();
}

void TokenSyncLog::parse_key(const leveldb::Slice& s, uint64_t &ts, ObjectID& id)
{
    char *pos = (char*) memchr(s.data(), '\n', s.size());  // NOLINT
    uint32_t pos_idx = pos - s.data();
    fds_verify(pos != nullptr);

    ts = atol(s.data());
    fds_verify(ts != 0);

    fds_verify((s.size() - (pos_idx+1)) == id.GetLen());
    id.SetId(pos+1, s.size() - (pos_idx+1));
}

/* State machine */
struct TokenSyncSenderFSM_
        : public msm::front::state_machine_def<TokenSyncSenderFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            fds_token_id token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
    {
        mig_stream_id_ = mig_stream_id;
        migrationSvc_ = migrationSvc;
        parent_ = parent;
        data_store_ = data_store;
        rcvr_ip_ = rcvr_ip;
        rcvr_port_ = rcvr_port;
        rcvr_session_ = rcvr_session;
        client_resp_handler_ = client_resp_handler;

        token_id_ = token_id;
        snap_msg_.token_id = token_id_;
        snap_msg_.io_type = FDS_SM_SNAPSHOT_TOKEN;
        snap_msg_.smio_snap_resp_cb = std::bind(
            &TokenSyncSenderFSM_::snap_done_cb, this,
            std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4);

        push_md_req_.header.base_header.err_code = ERR_OK;
        push_md_req_.header.base_header.src_node_name =
                migrationSvc_->get_ip();
        push_md_req_.header.base_header.session_uuid =
                rcvr_session_->getSessionId();
        push_md_req_.header.mig_id = parent_->get_mig_id();
        push_md_req_.header.mig_stream_id = mig_stream_id_;
        push_md_req_.md_list.clear();

        sync_log_itr_ = nullptr;

        max_entries_per_send_ = 100;
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
        typedef mpl::vector<TSIoClosedEvt> deferred_events;

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
    struct ack_sync_req
    {
        /* Acknowledge sync request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            fds_assert(fsm.token_id_ == static_cast<fds_token_id>(evt.token));
            fsm.max_entries_per_send_ = evt.max_entries_per_reply;
            fsm.cur_sync_range_low_ = evt.start_time;
            /* NOTE: Ignoring evt.end_time for now.  Relying on close event from OM */

            LOGDEBUG << "ack_sync_req token: " << fsm.token_id_
                    << " cur_sync_range_low_: " << fsm.cur_sync_range_low_;

            /* Send a respones back acking request */
            boost::shared_ptr<FDSP_SyncTokenResp> response(new FDSP_SyncTokenResp());

            response->header.base_header.err_code = ERR_OK;
            response->header.base_header.src_node_name = fsm.migrationSvc_->get_ip();
            response->header.base_header.session_uuid =
                    evt.header.base_header.session_uuid;
            response->header.mig_id = evt.header.mig_id;
            response->header.mig_stream_id = evt.header.mig_stream_id;

            auto resp_client = fsm.migrationSvc_->\
                    get_resp_client(evt.header.base_header.session_uuid);
            resp_client->SyncTokenResp(response);
        }
    };
    struct take_snap
    {
        /* Connect to the receiver so we can push tokens */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "take_snap token: " << fsm.token_id_;
            fds_assert(fsm.cur_sync_range_high_ != 0);

            /* Recyle snap_msg_ message */
            fds_assert(fsm.snap_msg_.smio_snap_resp_cb);

            Error err = fsm.data_store_->enqueueMsg(FdsSysTaskQueueId, &fsm.snap_msg_);
            if (err != fds::ERR_OK) {
                fds_assert(!"Hit an error in enqueing");
                // TODO(rao): Put your selft in an error state
                LOGERROR << "Failed to enqueue to snap_msg_ to StorMgr.  Error: "
                        << err;
                fsm.process_event(ErrorEvt());
                return;
            }
        }
    };
    struct build_sync_log
    {
        /* Prepare objstor_read_req_ for next read request */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            /* Create new sync log */
            std::ostringstream oss;
            oss << fsm.migrationSvc_->get_metadir_root() <<
                    "/TokenSyncLog_" << fsm.mig_stream_id_ << "_" << fsm.token_id_
                    << "_" << fsm.cur_sync_range_low_ << "_" << fsm.cur_sync_range_high_;
            fsm.sync_log_.reset(new TokenSyncLog(oss.str()));

            /* Iterate the snapshot and add entries to sync log */
            leveldb::Iterator* it = evt.db->NewIterator(evt.options);
            ObjectID start_obj_id, end_obj_id;
            fsm.migrationSvc_->get_cluster_comm_mgr()->get_dlt()->\
                    getTokenObjectRange(fsm.token_id_, start_obj_id, end_obj_id);

            /* TODO(Rao): We should iterate just the token.  Now we are iterating
             * the whole db, which contains multiple tokens
             */
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                ObjectID id(it->key().ToString());
                LOGDEBUG << "token: " << fsm.token_id_ << "range check id: " << id
                        << " start: " << start_obj_id << " end: " << end_obj_id;
                /* Range check */
                if (id < start_obj_id || id > end_obj_id) {
                    continue;
                }

                ObjMetaData entry;
                entry.deserializeFrom(it->value());
                auto mod_ts = entry.getModificationTs();
                fds_assert(mod_ts != 0);
                if (mod_ts < fsm.cur_sync_range_low_ ||
                    mod_ts > fsm.cur_sync_range_high_) {
                    continue;
                }

                Error e = fsm.sync_log_->add(id, entry);
                if (e != ERR_OK) {
                    fds_assert(!"Error");
                    LOGERROR << "Error adding synclog. Error: " << e;
                    fsm.process_event(ErrorEvt());
                }
            }
            assert(it->status().ok());  // Check for any errors found during the scan

            LOGDEBUG << "build_sync_log token: " << fsm.token_id_
                    << " low sync_ts: " << fsm.cur_sync_range_low_
                    << " high_sync_ts: " <<  fsm.cur_sync_range_high_
                    << " sync log size: " << fsm.sync_log_->size();

            /* Position sync log iterator to beginning */
            fsm.sync_log_itr_ = fsm.sync_log_->iterator();
            fsm.sync_log_itr_->SeekToFirst();

            /* clean up */
            delete it;
            evt.db->ReleaseSnapshot(evt.options.snapshot);
        }
    };
    struct send_sync_log
    {
        /* Iterate sync log and send entries.  There is a limit on number of entries
         * sent per RPC
         */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);

            DBG(uint64_t prev_ts = 0);
            FDSP_MigrateObjectMetadataList &md_list = fsm.push_md_req_.md_list;
            md_list.clear();

            for (; fsm.sync_log_itr_->Valid() &&
                 md_list.size() < fsm.max_entries_per_send_;
                 fsm.sync_log_itr_->Next()) {
                FDSP_MigrateObjectMetadata md;
                ObjMetaData entry;

                ObjectID obj_id;
                TokenSyncLog::parse_iterator(fsm.sync_log_itr_, obj_id, entry);
                entry.extractSyncData(md);
                md.token_id = fsm.token_id_;
                assign(md.object_id, obj_id);
                // TODO(Rao): Set the size
                // md.obj_len = entry.len();
                md.token_id = fsm.token_id_;
                md.modification_ts = entry.getModificationTs();

                fds_assert(static_cast<uint64_t>(md.modification_ts) >= prev_ts);
                DBG(prev_ts = md.modification_ts);

                md_list.push_back(md);
            }

            if (md_list.size() > 0) {
                fsm.rcvr_session_->getClient()->PushTokenMetadata(fsm.push_md_req_);
                fsm.migrationSvc_->mig_cntrs.tok_sync_md_sent.incr(md_list.size());
            } else {
                /* When there is nothing to send we simulate an ack from network
                 * to make the statemachine transitioning easy
                 */
                FDSP_PushTokenMetadataRespPtr send_dn_evt(
                        new FDSP_PushTokenMetadataResp());
                fsm.parent_->send_actor_request(FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(FDSP_PushTokenMetadataResp),
                                send_dn_evt)));
            }

            LOGDEBUG << "send_sync_log token: " << fsm.token_id_
                    << " size: " << md_list.size();
        }
    };
    struct finish_sync
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "finish_sync token: " << fsm.token_id_;

            FDSP_NotifyTokenSyncComplete sync_compl;
            sync_compl.header.base_header.err_code = ERR_OK;
            sync_compl.header.base_header.src_node_name =
                    fsm.migrationSvc_->get_ip();
            sync_compl.header.base_header.session_uuid =
                    fsm.rcvr_session_->getSessionId();
            sync_compl.header.mig_id = fsm.parent_->get_mig_id();
            sync_compl.header.mig_stream_id = fsm.mig_stream_id_;
            sync_compl.token_id = fsm.token_id_;

            fsm.rcvr_session_->getClient()->NotifyTokenSyncComplete(sync_compl);
        }
    };
    struct set_io_closed
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "set_io_closed token: " << fsm.token_id_;
            fds_assert(!fsm.sync_closed_);
            fsm.sync_closed_time_ = evt.sync_close_ts;
            fsm.sync_closed_ = true;
            fsm.cur_sync_range_high_ = fsm.sync_closed_time_;
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
            LOGDEBUG << "teardown token: " << fsm.token_id_;
            LOGDEBUG << fsm.migrationSvc_->mig_cntrs.toString();

            // TODO(Rao): Notify parent sync is done.  For now not notifying
            // is ok, because parent gets to know sync is complete via pull
            // fsm.
        }
    };

    /* Guards */
    struct more_to_send
    {
        /* Returns true if there is more to send in sync log
         */
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            bool ret =  fsm.sync_log_itr_->Valid();
            LOGDEBUG << "token: " << fsm.token_id_ << " more_to_send: " << ret;
            return ret;
        }
    };

    /* Statemachine psuedo-code
     * wait_until_io_closed()
     * take_tokendb_snap()
     * build_sync_log()
     * while (synclog_itr_valid()) {
     *  send_batch_of_synclog()
     * }
     * NOTE: Ideally we shouldn't need to wait for io close event.  As soon as sync
     * request comes in we can start taking snapshot and sending sync log.  However,
     * this would mean we may need to take multiple snapshots and transfer multiple
     * sync logs before io close event arrives.  To keep the statemachine simple
     * for Alpha-release this implementation is chosen.
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , SyncReqEvt     ,WaitForClose, ack_sync_req    , msm_none         >,  // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    Row<WaitForClose, TSIoClosedEvt  , Snapshot   , ActionSequence_
                                                    <mpl::vector<
                                                    set_io_closed,
                                                    take_snap>>     , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Snapshot   , TSnapDnEvt    , BldSyncLog , build_sync_log   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< BldSyncLog , msm_none       , Sending    , send_sync_log   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , SendDnEvt      , msm_none   , send_sync_log   , more_to_send     >,

    Row< Sending    , SendDnEvt      , FiniSync   , finish_sync     , Not_<more_to_send>>,  // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    Row< FiniSync   , msm_none       , Complete   , teardown        , msm_none         >,
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

    // the initial state of the SM. Must be defined
    typedef mpl::vector<Init, AllOk> initial_state;

    /* Callback from object store that metadata snapshot is complete */
    void snap_done_cb(const Error& e,
            SmIoSnapshotObjectDB* snap_msg,
            leveldb::ReadOptions& options, leveldb::DB* db)
    {
        LOGDEBUG << " token: " << token_id_;

        if (!e.ok()) {
            LOGERROR << "Error: " << e;
            msm::back::state_machine<TokenSyncSenderFSM_> &fsm =
                    static_cast<msm::back::state_machine<TokenSyncSenderFSM_> &>(*this);
            fsm.process_event(ErrorEvt());
            return;
        }
        TSnapDnEvtPtr snap_dn_evt(new TSnapDnEvt(options, db));
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TSnapDnEvt), snap_dn_evt)));
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

    /* RPC handler for responses.  NOTE: We don't really use it.  It's just here so
     * it can be passed as a parameter when we create a netClientSession
     */
    boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler_;

    /* Sender session endpoint */
    netMigrationPathClientSession *rcvr_session_;

    /* Current token id we are syncing */
    fds::fds_token_id token_id_;

    /* Token data store reference */
    SmIoReqHandler *data_store_;

    /* Current sync range lower bound */
    uint64_t cur_sync_range_low_;

    /* Current sync range upper bound */
    uint64_t cur_sync_range_high_;

    /* Whether uppper bound on the sync range has been closed or not*/
    bool sync_closed_;

    /* Time at which sync was closed.  Once sync is closed this time becomes
     * cur_sync_range_high_ */
    uint64_t sync_closed_time_;

    /* snap message */
    SmIoSnapshotObjectDB snap_msg_;

    /* RPC request.  It's recycled for every push request */
    FDSP_PushTokenMetadataReq push_md_req_;

    /* Log of object metadata entries to be shipped */
    TokenSyncLogPtr sync_log_;
    /* Current sync log iterator */
    leveldb::Iterator* sync_log_itr_;

    /* Max sync log entries to send per rpc */
    uint32_t max_entries_per_send_;
};  /* struct TokenSyncSenderFSM_ */

TokenSyncSender::TokenSyncSender()
{
    fsm_ = nullptr;
}

TokenSyncSender::~TokenSyncSender()
{
    delete fsm_;
}

void TokenSyncSender::init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            fds_token_id token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler)
{
    fsm_ = new TokenSyncSenderFSM();
    fsm_->init(mig_stream_id, migrationSvc, parent, data_store, rcvr_ip,
            rcvr_port, token_id, rcvr_session, client_resp_handler);
}

void TokenSyncSender::start()
{
    fsm_->start();
}

void TokenSyncSender::process_event(const SyncReqEvt& event)
{
    fsm_->process_event(event);
}

void TokenSyncSender::process_event(const SendDnEvt& event)
{
    fsm_->process_event(event);
}

void TokenSyncSender::process_event(const TSnapDnEvt& event)
{
    fsm_->process_event(event);
}
void TokenSyncSender::process_event(const TSIoClosedEvt& event)
{
    fsm_->process_event(event);
}
} /* namespace fds */
