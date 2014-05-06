/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <RpcRequestPool.h>
#include <net/net-service.h>

namespace fds {

/**
 * Constructor
 */
RpcRequestPool::RpcRequestPool()
{
    nextAsyncReqId_ = 0;
}

/**
 *
 */
RpcRequestPool::~RpcRequestPool()
{
    nextAsyncReqId_ = 0;
}
/**
 * Constructs EPRpcRequest object
 * @param uuid
 * @return
 */
EPRpcRequestPtr RpcRequestPool::newEPRpcRequest(const fpi::SvcUuid &uuid)
{
    return EPRpcRequestPtr(new EPRpcRequest(uuid));
}

/**
 * Constructs EPAsyncRpcRequest object
 * @param uuid
 * @return
 */
EPAsyncRpcRequestPtr RpcRequestPool::newEPAsyncRpcRequest(const fpi::SvcUuid &uuid)
{
    fds_scoped_lock l(asyncReqMaplock_);
    EPAsyncRpcRequestPtr req(new EPAsyncRpcRequest(nextAsyncReqId_, uuid));
    asyncReqMap_[nextAsyncReqId_] = req;
    nextAsyncReqId_++;

    return req;
}

}  // namespace fds
