/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <string>
#include <functional>
#include <net/RpcRequestPool.h>
#include <net/net-service.h>
#include <net/AsyncRpcRequestTracker.h>
#include <platform/platform-lib.h>
#include <fdsp_utils.h>
#include <fds_process.h>

namespace fds {

// TODO(Rao): Make RpcRequestPool and AsyncRpcRequestTracker a module
RpcRequestPool *gRpcRequestPool;
AsyncRpcRequestTracker* gAsyncRpcTracker;
AsyncRpcCounters* gAsyncRpcCntrs;

/**
* @brief Constructor
*
* @param id
* @param mgr
*/
AsyncRpcCounters::AsyncRpcCounters(const std::string &id, FdsCountersMgr *mgr)
    : FdsCounters(id, mgr),
    timedout("timedout", this),
    invokeerrors("invokeerrors", this),
    appsuccess("appsuccess", this),
    apperrors("apperrors", this)
{
}

AsyncRpcCounters::~AsyncRpcCounters()
{
}

/**
 * Constructor
 */
RpcRequestPool::RpcRequestPool()
{
    gAsyncRpcTracker = new AsyncRpcRequestTracker();
    gAsyncRpcCntrs = new AsyncRpcCounters("Rpc", g_cntrs_mgr.get());

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

/**
* @brief 
*
* @param reqId
* @param srcUuid
* @param dstUuid
*
* @return 
*/
boost::shared_ptr<fpi::AsyncHdr> RpcRequestPool::newAsyncHeaderPtr(
                                    const AsyncRpcRequestId& reqId,
                                    const fpi::SvcUuid &srcUuid,
                                    const fpi::SvcUuid &dstUuid)
{
    boost::shared_ptr<fpi::AsyncHdr> hdr(new fpi::AsyncHdr());
    *hdr = newAsyncHeader(reqId, srcUuid, dstUuid);
    return hdr;
}

/**
 * Common handling as part of any async request creation
 * @param req
 */
void RpcRequestPool::asyncRpcInitCommon_(AsyncRpcRequestIfPtr req)
{
    gAsyncRpcTracker->addForTracking(req->getRequestId(), req);
    req->setCompletionCb(finishTrackingCb_);
}

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
    asyncRpcInitCommon_(req);

    return req;
}

/**
* @brief Constructs a failover rpc request context
*
* @param myEpId
* @param peerEpIds
*
* @return 
*/
FailoverRpcRequestPtr RpcRequestPool::newFailoverRpcRequest(
    const fpi::SvcUuid& myEpId,
    const std::vector<fpi::SvcUuid>& peerEpIds)
{
    auto reqId = nextAsyncReqId_++;

    FailoverRpcRequestPtr req(new FailoverRpcRequest(reqId, myEpId, peerEpIds));
    asyncRpcInitCommon_(req);

    return req;
}

FailoverRpcRequestPtr RpcRequestPool::newFailoverRpcRequest(
    const EpIdProviderPtr epProvider)
{
    auto reqId = nextAsyncReqId_++;

    fpi::SvcUuid myEpId;
    fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());

    FailoverRpcRequestPtr req(new FailoverRpcRequest(reqId, myEpId, epProvider));
    asyncRpcInitCommon_(req);

    return req;
}
}  // namespace fds
