/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_RPC_REQUEST_POOL_H_
#define SOURCE_INCLUDE_RPC_REQUEST_POOL_H_

#include <unordered_map>

#include <concurrency/Mutex.h>
#include <RpcRequest.h>

namespace fds {

class AsyncRpcRequestTracker {
public:
    void addForTracking(const AsyncRpcRequestId& id, AsyncRpcRequestIfPtr req);
    void removeFromTracking(const AsyncRpcRequestId& id);
    AsyncRpcRequestIfPtr getAsyncRpcRequest(const AsyncRpcRequestId &id);

protected:
    fds_spinlock asyncReqMaplock_;
    std::unordered_map<AsyncRpcRequestId, AsyncRpcRequestIfPtr> asyncReqMap_;
};
typedef boost::shared_ptr<AsyncRpcRequestTracker> AsyncRpcRequestTrackerPtr;

/**
 * Rpc request factory. Use this class for constructing various RPC request objects
 */
class RpcRequestPool {
public:
    RpcRequestPool();
    ~RpcRequestPool();

    EPRpcRequestPtr newEPRpcRequest(const fpi::SvcUuid &uuid);
    EPAsyncRpcRequestPtr newEPAsyncRpcRequest(const fpi::SvcUuid &uuid);

protected:
    // TODO(Rao): This lock may not be necessary
    fds_spinlock lock_;
    AsyncRpcRequestTracker tracker_;
    // TODO(Rao): Use atomic here
    uint64_t nextAsyncReqId_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_RPC_REQUEST_MAP_H_
