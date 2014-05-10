/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <net/RpcRequestPool.h>
#include <net/net-service.h>
#include <AsyncRpcRequestTracker.h>

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
EPAsyncRpcRequestPtr
RpcRequestPool::newEPAsyncRpcRequest(const fpi::SvcUuid &uuid)
{
    fds_scoped_lock l(lock_);
    EPAsyncRpcRequestPtr req(new EPAsyncRpcRequest(nextAsyncReqId_, uuid));
    gAsyncRpcTracker->addForTracking(nextAsyncReqId_, req);

    nextAsyncReqId_++;

    return req;
}

/**
 *
 * @param uuid
 * @return
 */
FailoverRpcRequestPtr
RpcRequestPool::newFailoverRpcRequest(const std::vector<fpi::SvcUuid>& uuid_list)
{
    fds_scoped_lock l(lock_);
    FailoverRpcRequestPtr req(new FailoverRpcRequest(nextAsyncReqId_, uuid_list));
    gAsyncRpcTracker->addForTracking(nextAsyncReqId_, req);

    nextAsyncReqId_++;

    return req;
}
}  // namespace fds
