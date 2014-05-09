/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_RPC_REQUEST_POOL_H_
#define SOURCE_INCLUDE_RPC_REQUEST_POOL_H_

#include <concurrency/Mutex.h>
#include <net/RpcRequest.h>

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
    // TODO(Rao): This lock may not be necessary
    fds_spinlock lock_;
    // TODO(Rao): Use atomic here
    uint64_t nextAsyncReqId_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_RPC_REQUEST_MAP_H_
