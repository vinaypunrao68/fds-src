/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <functional>
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
    finishTrackingCb_ = std::bind(&AsyncRpcRequestTracker::removeFromTracking,
            gAsyncRpcTracker, std::placeholders::_1);
}

/**
 *
 */
RpcRequestPool::~RpcRequestPool()
{
    nextAsyncReqId_ = 0;
}

/**
 * Common handling as part of any async request creation
 * @param req
 */
void RpcRequestPool::asyncRpcCommonHandling(AsyncRpcRequestIfPtr req)
{
    gAsyncRpcTracker->addForTracking(req->getRequestId(), req);
    req->setCompletionCb(finishTrackingCb_);
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
RpcRequestPool::newEPAsyncRpcRequest(const fpi::SvcUuid &myEpId,
                                     const fpi::SvcUuid &peerEpId)
{
    auto reqId = nextAsyncReqId_++;
    EPAsyncRpcRequestPtr req(new EPAsyncRpcRequest(reqId, myEpId, peerEpId));
    asyncRpcCommonHandling(req);

    return req;
}
/**
 *
 * @return
 */
fpi::AsyncHdr
RpcRequestPool::newAsyncHeader(const AsyncRpcRequestId& reqId,
                               const fpi::SvcUuid &srcUuid,
                               const fpi::SvcUuid &dstUuid)
{
    fpi::AsyncHdr header;

    header.msg_src_id = reqId;
    header.msg_src_uuid = srcUuid;
    header.msg_dst_uuid = dstUuid;
    header.msg_code = 0;
    return header;
}

}  // namespace fds
