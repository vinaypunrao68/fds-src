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

#include <leveldb/db.h>
#include <leveldb/env.h>

#include <fds_timestamp.h>
#include <fds_migration.h>
#include <TokenCopySender.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
using namespace msm::front::euml;   // NOLINT
typedef msm::front::none msm_none;

/**
 * Leveldb based Token sync log
 * Can be made generic by either templating the underneath db or
 * via inheritance
 */
class TokenSyncLog {
public:
    /* Modificatin timestamp comparator */
    class ModTSComparator : public leveldb::Comparator {
       public:
        int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
          uint64_t ts1;
          ObjectID id1;
          uint64_t ts2;
          ObjectID id2;
          TokenSyncLog::parse_key(a, ts1, id1);
          TokenSyncLog::parse_key(a, ts2, id2);

          if (ts1 < ts2) {
              return -1;
          } else if (ts1 > ts2) {
              return 1;
          }
          ObjectLess  less;
          if (less(id1, id2)) {
              return -1;
          } else if (less(id2, id1)) {
              return 1;
          }
          return 0;
        }
        // Ignore the following methods for now:
        const char* Name() { return "TwoPartComparator"; }
        void FindShortestSeparator(std::string*, const leveldb::Slice&) const { }
        void FindShortSuccessor(std::string*) const { }
      };
public:
    TokenSyncLog(const std::string& name) {
        name_ = name;
        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options, name, &db_);
        LOGDEBUG << "Opening tokendb: " << name;
        assert(status.ok());
    }

    virtual ~TokenSyncLog() {
        LOGDEBUG << "Closing tokendb: " << name_;
        delete db_;
    }

    Error add(const ObjectID& id, const SmObjMetadata &entry) {
        fds::Error err(fds::ERR_OK);
        std::string k = create_key(id, entry);
        leveldb::Slice key(k);
        leveldb::Slice value(entry.buf(), entry.len());
        leveldb::Status status = db_->Put(write_options_, key, value);

        if (!status.ok()) {
            LOGERROR << "Failed to write key: " << id;
            err = fds::Error(fds::ERR_DISK_WRITE_FAILED);
        }

        return err;
    }

    leveldb::Iterator* iterator() {
        return db_->NewIterator(leveldb::ReadOptions());
    }

    static void parse_iterator(leveldb::Iterator* itr,
            ObjectID &id, SmObjMetadata &entry) {
        uint64_t ts;
        parse_key(itr->key(), ts, id);
        SmObjMetadata temp_entry(itr->value().ToString());
        entry = temp_entry;
    }
private:
    static std::string create_key(const ObjectID& id, const SmObjMetadata &entry)
    {
        std::ostringstream oss;
        oss << entry.get_modification_ts() << "\n" << id;
        return oss.str();
    }

    static void parse_key(const leveldb::Slice& s, uint64_t &ts, ObjectID& id) {
        istringstream iss(s);
        iss >> ts >> id;
    }

    std::string name_;
    leveldb::WriteOptions write_options_;
    leveldb::DB* db_;
};
typedef boost::shared_ptr<TokenSyncLog> TokenSyncLogPtr;

/* Statemachine Events */
struct StartEvt {};

/* Snapshot is complete notification event */
struct TSSnapDnEvt {
    TSSnapDnEvt(leveldb::ReadOptions& options, leveldb::DB* db)
    {
        this->options = options;
        this->db = db;
    }
    leveldb::ReadOptions options;
    leveldb::DB* db;
};
boost::shared_ptr<TSSnapDnEvt> TSSnapDnEvtPtr;

struct BldSyncLogDnEvt {};
struct SendDnEvt {};
struct IoClosedEvt {};
struct SyncDnAckEvt {};
struct ErrorEvt {};

/* State machine */
struct TokenSyncSenderFSM_
        : public msm::front::state_machine_def<TokenSyncSenderFSM_> {
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenCopySender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            fds_token_id token_id,
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

        token_id_ = token_id;
        snap_msg_.token_id = token_id_;
        snap_msg_.io_type = FDS_SM_SNAPSHOT_TOKEN;
        snap_msg_.smio_snap_resp_cb = std::bind(
            &TokenSyncSenderFSM_::snap_done_cb, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

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

            if (sync_closed_) {
                cur_sync_range_high_ = sync_closed_time_;
            } else {
                cur_sync_range_high_ = get_fds_timestamp_ms();
            }

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
            LOGDEBUG << "build_sync_log";
            std::string log_name = "TokenSyncLog_" + fsm.token_id_ +
                    "_" + cur_sync_range_high_ + "_" + cur_sync_range_low_;
            fsm.sync_log_.reset(new TokenSyncLog(log_name));

            /* Iterate the snapshot and add entries to sync log */
            leveldb::Iterator* it = evt.db->NewIterator(evt.options);
            ObjectID start_obj_id, end_obj_id;
            fsm.migrationSvc_->get_cluster_comm_mgr()->get_dlt()->\
                    getTokenObjectRange(fsm.token_id_, start_obj_id, end_obj_id);
            // TODO(Rao): We should iterate just the token.  Now we are iterating
            // the whole db, which contains multiple tokens
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                ObjectID id(it->key().ToString());
                /* Range check */
                if (id < start_obj_id || end_obj_id > id) {
                    continue;
                }
                SmObjMetadata entry(it->value().ToString());

                Error e = fsm.sync_log_.add(id, entry);
                if (e != ERR_OK) {
                    fds_assert(!"Error");
                    LOGERROR << " Error: " << e;
                    fsm.process_event(ErrorEvt());
                }
            }
            assert(it->status().ok());  // Check for any errors found during the scan

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
            LOGDEBUG << "send_sync_log";

            Error err(ERR_OK);

            FDSP_MigrateObjectMetadataList &md_list = push_md_req_.md_list;
            md_list.clear();
            for (; fsm.sync_log_itr_->Valid() &&
                   md_list.size() < fsm.max_entries_per_send_;
                 fsm.sync_log_itr_->Next()) {
                FDSP_MigrateObjectMetadata md;
                SmObjMetadata entry;

                md.token_id = fsm.token_id_;
                TokenSyncLog::parse_iterator(fsm.sync_log_itr_, md.object_id, entry);
                md.obj_len = entry.len();
                md.modification_ts = entry.get_modification_ts();

                md_list.push_back(md);
            }

            if (md_list.size() > 0) {
                fsm.rcvr_session_->getClient()->PushTokenMetadata(fsm.push_md_req_);
            } else {
                /* When there is nothing to send we simulate an ack from network
                 * to make the statemachine transitioning easy
                 */
                FDSP_PushTokenMetadataRespPtr send_dn_evt(new FDSP_PushTokenMetadataResp());
                parent_->send_actor_request(FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(FDSP_PushTokenMetadataResp), send_dn_evt)));
                LOGDEBUG << "send_sync_log: Nothing to send for token: " << token_id_;
            }
        }
    };
    struct finish_sync
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            FDSP_NotifyTokenSyncComplete sync_compl;
            sync_compl.token_id = fsm.token_id_;
            fsm.rcvr_session_->getClient()->NotifyTokenSyncComplete(sync_compl);
        }
    };
    struct report_error
    {
        /* Pushes the token data to receiver */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGERROR << "Error state";
            fds_assert(!"error");
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
            return !(fsm.sync_log_itr_->Valid());
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
    Row< Snapshot   , TSSnapDnEvt    , BldSyncLog , build_sync_log  , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< BldSyncLog , msm_none       , Sending    , send_sync_log   , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , SendDnEvt      , Sending    , send_sync_log   , Not_<sync_log_dn>>,
    Row< Sending    , SendDnEvt      ,WaitForClose, msm_none        , Not_<sync_closed> >,
    Row< Sending    , SendDnEvt      , Snapshot   , take_snap       , Not_<sync_dn>    >,
    Row< Sending    , SendDnEvt      , FiniSync   , finish_sync     , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< WaitForClose, IoClosedEvt   , Snapshot   , take_snap       , msm_none         >,
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

    void snap_done_cb(const Error& e,
            leveldb::ReadOptions& options, leveldb::DB* db)
    {
        if (!e.ok()) {
            LOGERROR << "Error: " << e;
            this->process_event(ErrorEvt());
            return;
        }
        TSSnapDnEvtPtr snap_dn_evt(new TSSnapDnEvt(options, db));
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(TSSnapDnEvt), snap_dn_evt)));

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

    fds::fds_token_id token_id_;

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

/* PullSenderFSM events */
struct PullReqEvt {};
struct DataReadDnEvt {};
struct DataSendDnEvt {};
struct PullFiniEvt {};

/* State machine for Pull sender */
struct PullSenderFSM_
        : public msm::front::state_machine_def<PullSenderFSM_> {
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
            &PullSenderFSM_::data_read_cb, this,
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
        LOGDEBUG << "PullSenderFSM_";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        LOGDEBUG << "PullSenderFSM_";
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
    struct Sending: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            LOGDEBUG << "Sending State";
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

    struct issue_reads
    {
        /* Out of to be read objects, picks a few, and issues read */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            LOGDEBUG << "issue_reads";
            // TODO(Rao): issue_reads. Only if there are objects to be read
        }
    };
    struct send_data
    {
        /* send the read data */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            Error err(ERR_OK);
            // TODO(Rao): Send the read data
            LOGDEBUG << "send_data";
        }
    };
    struct update_sent_data
    {
        /* Mark the read data as sent */
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
            // TODO(Rao): Mark the read data as sent
            LOGDEBUG << "update_sent_data";
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
     *  read();
     *  send();
     *  update_read_objects();
     * }
     */
    struct transition_table : mpl::vector< // NOLINT
    // +------------+----------------+------------+-----------------+------------------+
    // | Start      | Event          | Next       | Action          | Guard            |
    // +------------+----------------+------------+-----------------+------------------+
    Row< Init       , msm_none       , Sending    , msm_none        , msm_none         >,
    // +------------+----------------+------------+-----------------+------------------+
    Row< Sending    , PullReqEvt     , Sending    , ActionSequence_
                                                    <mpl::vector<
                                                    add_for_pull,
                                                    issue_reads> >  , msm_none         >,

    Row< Sending    , DataReadDnEvt  , Sending    , send_data       , msm_none         >,

    Row< Sending    , DataSendDnEvt  , Sending    , ActionSequence_
                                                    <mpl::vector<
                                                    update_sent_data,
                                                    issue_reads> >  , msm_none         >,

    Row< Sending    , PullFiniEvt    , Complete   , teardown        , msm_none         >
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

};  /* struct PullSenderFSM_ */

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
