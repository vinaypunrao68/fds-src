/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <AsyncRpcRequestTracker.h>
#include <net/net-service.h>

namespace fds {

/**
 * Constructor
 */
RpcRequestFactory::RpcRequestFactory()
{
    gl_rpcReqTracker = new AsyncRpcRequestTracker();
}

/**
 * Constructs EPRpcRequest object
 * @param uuid
 * @return
 */
EPRpcRequestPtr RpcRequestFactory::EPRpcRequest(const fpi::SvcUuid &uuid)
{
    return new EPRpcRequest(ep);
}

/**
 * Constructs EPAsyncRpcRequest object
 * @param uuid
 * @return
 */
EPAsyncRpcRequestPtr RpcRequestFactory::EPAsyncRpcRequest(const fpi::SvcUuid &uuid)
{
    EPAsyncRpcRequestPtr req = new EPAsyncRpcRequest(ep);
    gl_rpcReqTracker->addForTracking(req);
    return req;
}

}  // namespace fds
