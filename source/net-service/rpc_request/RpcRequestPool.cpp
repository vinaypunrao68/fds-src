/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <net/RpcRequestPool.h>
#include <net/net-service.h>
#include <net/AsyncRpcRequestTracker.h>

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
#if 0
EPRpcRequestPtr RpcRequestPool::newEPRpcRequest(const fpi::SvcUuid &uuid)
{
    return EPRpcRequestPtr(new EPRpcRequest(uuid));
}
#endif
/**
 * Constructs EPAsyncRpcRequest object
 * @param uuid
 * @return
 */
EPAsyncRpcRequestPtr
RpcRequestPool::newEPAsyncRpcRequest(const fpi::SvcUuid &uuid)
{
    auto reqId = nextAsyncReqId_++;
    EPAsyncRpcRequestPtr req(new EPAsyncRpcRequest(reqId, uuid));
    gAsyncRpcTracker->addForTracking(reqId, req);


    return req;
}
/**
 *
 * @return
 */
fpi::AsyncHdr
RpcRequestPool::newAsyncHeader(const AsyncRpcRequestId& reqId,
                               const fpi::SvcUuid &dstUuid)
{
    fpi::AsyncHdr header;

    header.msg_src_id = reqId;
    // TODO(Rao): Fill the src uuid
    // header.msg_src_uuid = ;
    header.msg_dst_uuid = dstUuid;
    header.msg_code = 0;
    return header;
}

}  // namespace fds
