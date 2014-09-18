/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENSYNC_SENDER_H_
#define TOKENSYNC_SENDER_H_

#include <string>
#include <memory>
#include <boost/msm/back/state_machine.hpp>

#include <leveldb/comparator.h>

#include <fds_assert.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>
#include <fds_base_migrators.h>
#include <util/Log.h>
// #include <NetSession.h>
#include <ClusterCommMgr.h>
#include <StorMgrVolumes.h>
#include <ObjMeta.h>
#include <TokenSyncBaseEvents.h>


namespace fds {

using namespace  ::FDS_ProtocolInterface;

/* Forward declarations */
struct TokenSyncSenderFSM_;
typedef boost::msm::back::state_machine<TokenSyncSenderFSM_> TokenSyncSenderFSM;
class TokenSender;

/**
 * Leveldb based Token sync log
 * Can be made generic persistent log collector by either templating the underneath db
 * or via inheritance
 */
class TokenSyncLog {
 public:
    /* Comparator for ordering synclog based on modification timestamp */
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
          return ObjectID::compare(id1, id2);
        }
        // Ignore the following methods for now:
        const char* Name() { return "TwoPartComparator"; }
        void FindShortestSeparator(std::string*, const leveldb::Slice&) const { }
        void FindShortSuccessor(std::string*) const { }
      };

 public:
    explicit TokenSyncLog(const std::string& name);

    virtual ~TokenSyncLog();

    Error add(const ObjectID& id, const ObjMetaData &entry);
    size_t size();

    leveldb::Iterator* iterator();
    /* Extracts objectid and entry from the iterator */
    static void parse_iterator(leveldb::Iterator* itr,
            ObjectID &id, ObjMetaData &entry);

 private:
    static std::string create_key(const ObjectID& id, const ObjMetaData& entry);
    static void parse_key(const leveldb::Slice& s, uint64_t &ts, ObjectID& id);

    std::string name_;
    leveldb::WriteOptions write_options_;
    leveldb::DB* db_;
    size_t cnt_;
};

typedef boost::shared_ptr<TokenSyncLog> TokenSyncLogPtr;
/* State machine events */
/* Sync request event */
typedef FDSP_SyncTokenReq SyncReqEvt;
typedef FDSP_PushTokenMetadataResp SendDnEvt;

struct BldSyncLogDnEvt {};

/* IO closed notification.  This provides upper bound timestamp
 * for sync */
typedef MigSvcSyncCloseReq TSIoClosedEvt;

struct SyncDnAckEvt {};


struct TokenSyncSender {
public:
    TokenSyncSender();
    virtual ~TokenSyncSender();
    void init(const std::string &mig_stream_id,
            FdsMigrationSvc *migrationSvc,
            TokenSender *parent,
            SmIoReqHandler *data_store,
            const std::string &rcvr_ip,
            const int &rcvr_port,
            fds_token_id token_id,
            netMigrationPathClientSession *rcvr_session,
            boost::shared_ptr<FDSP_MigrationPathRespIf> client_resp_handler);
    void start();
    void process_event(const SyncReqEvt& event);
    void process_event(const SendDnEvt& event);
    void process_event(const TSnapDnEvt& event);
    void process_event(const TSIoClosedEvt& event);
private:
    TokenSyncSenderFSM *fsm_;

};
}  // namespace fds

#endif  /* TOKENSYNC_SENDER_H_*/
