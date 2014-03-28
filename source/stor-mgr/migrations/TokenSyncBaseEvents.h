/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENSYNC_BASE_EVENTS_H_
#define TOKENSYNC_BASE_EVENTS_H_

namespace fds {

struct ErrorEvt {};
struct StartEvt {};

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
