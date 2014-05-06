/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_RPC_REQUEST_POOL_H_
#define SOURCE_INCLUDE_RPC_REQUEST_POOL_H_

#include <unordered_map>

#include <concurrency/Mutex.h>
#include <RpcRequest.h>

namespace fds {

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
    fds_spinlock asyncReqMaplock_;
    std::unordered_map<AsyncRpcRequestId, EPAsyncRpcRequestPtr> asyncReqMap_;
    uint64_t nextAsyncReqId_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_RPC_REQUEST_MAP_H_
