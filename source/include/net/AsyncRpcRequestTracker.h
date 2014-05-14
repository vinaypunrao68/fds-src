/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_ASYNCRPCREQUESTTRACKER_H_
#define SOURCE_INCLUDE_NET_ASYNCRPCREQUESTTRACKER_H_

#include <unordered_map>

#include <concurrency/Mutex.h>
#include <net/RpcRequest.h>

namespace fds {

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

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_ASYNCRPCREQUESTTRACKER_H_
