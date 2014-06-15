/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_ASYNCRPCREQUESTTRACKER_H_
#define SOURCE_INCLUDE_NET_ASYNCRPCREQUESTTRACKER_H_

#include <unordered_map>
#include <string>

#include <concurrency/Mutex.h>
#include <fds_counters.h>
#include <net/RpcRequest.h>

namespace fds {

/**
 * @brief Async Rpc counters
 */
class AsyncRpcCounters : public FdsCounters
{
 public:
    AsyncRpcCounters(const std::string &id, FdsCountersMgr *mgr);
    ~AsyncRpcCounters();

    /* Number of requests that have timedout */
    NumericCounter timedout;
    /* Number of requests that experienced transport error */
    NumericCounter invokeerrors;
    /* Number of responses that resulted in app acceptance */
    NumericCounter appsuccess;
    /* Number of responses that resulted in app rejections */
    NumericCounter apperrors;
};

/**
 * Tracker async rpc requests.  RPC requests are tracked by their id
 */
class AsyncRpcRequestTracker {
 public:
    bool addForTracking(const AsyncRpcRequestId& id, AsyncRpcRequestIfPtr req);
    bool removeFromTracking(const AsyncRpcRequestId& id);
    AsyncRpcRequestIfPtr getAsyncRpcRequest(const AsyncRpcRequestId &id);

 protected:
    fds_spinlock asyncReqMaplock_;
    std::unordered_map<AsyncRpcRequestId, AsyncRpcRequestIfPtr> asyncReqMap_;
};

extern AsyncRpcRequestTracker* gAsyncRpcTracker;
extern AsyncRpcCounters* gAsyncRpcCntrs;

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_ASYNCRPCREQUESTTRACKER_H_
