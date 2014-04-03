/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENSYNC_BASE_EVENTS_H_
#define TOKENSYNC_BASE_EVENTS_H_

namespace fds {

struct ErrorEvt {};
struct StartEvt {};

/**
 * Token sync start event
 */
struct TRSyncStartEvt {
    TRSyncStartEvt()
    : TRSyncStartEvt(0, 0)
    {}
    TRSyncStartEvt(const uint64_t &start, const uint64_t &end) {
        start_time = start;
        end_time = end;
    }
    /* start time */
    uint64_t start_time;
    /* End time.  zero means end time isn't specified */
    uint64_t end_time;
};
typedef boost::shared_ptr<TRSyncStartEvt> TRSyncStartEvtPtr;

/* Snapshot is complete notification event */
struct TSnapDnEvt {
    TSnapDnEvt(const leveldb::ReadOptions& options, leveldb::DB* db)
    {
        this->options = options;
        this->db = db;
    }
    leveldb::ReadOptions options;
    leveldb::DB* db;
};
typedef boost::shared_ptr<TSnapDnEvt> TSnapDnEvtPtr;

/* Receiver side pull request event */
struct TRPullReqEvt {
    std::list<ObjectID> pull_ids;
};
typedef boost::shared_ptr<TRPullReqEvt> TRPullReqEvtPtr;

}  // namespace fds

#endif  /* TOKENSYNC_BASE_EVENTS_H_*/
