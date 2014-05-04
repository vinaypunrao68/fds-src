/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ASYNC_RPC_REQUEST_TRACKER_H_
#define SOURCE_INCLUDE_ASYNC_RPC_REQUEST_TRACKER_H_

#include <fdsp/fds_service_types.h>
#include <RpcRequest.h>

namespace fds {

/**
 * For tracking requests
 */
class AsyncRpcRequestTracker
{
public:
    AsyncRpcRequestTracker();
    virtual ~AsyncRpcRequestTracker();

    void addForTracking(AsyncRpcRequestPtr req);
    void removeFromTracking(AsyncRpcRequestPtr req);

protected:
    fds_spinlock lock_;
    std::unorderd_map<AsyncRpcRequestId, AsyncRpcRequestPtr> requestMap_;
};

extern AsyncRpcRequestTracker *gl_rpcReqTracker;

/**
 * Rpc request factory. Use this class for constructing various RPC request objects
 */
class RpcRequestFactory {
public:
    RpcRequestFactory();
    ~RpcRequestFactory();

    EPRpcRequestPtr EPRpcRequest(const fpi::SvcUuid &uuid);
    EPAsyncRpcRequestPtr EPAsyncRpcRequest(const fpi::SvcUuid &uuid);
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_ASYNC_RPC_REQUEST_TRACKER_H_
