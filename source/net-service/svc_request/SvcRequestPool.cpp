/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <string>
#include <functional>
#include <net/net-service.h>
#include <platform/platform-lib.h>
#include <fdsp_utils.h>
#include <fds_process.h>
#include <net/BaseAsyncSvcHandler.h>
#include <net/SvcRequestTracker.h>
#include <net/SvcRequestPool.h>
#include <platform/node-inventory.h>

namespace fds {

// TODO(Rao): Make SvcRequestPool and SvcRequestTracker a module
SvcRequestPool *gSvcRequestPool;
SvcRequestTracker* gSvcRequestTracker;
SvcRequestCounters* gSvcRequestCntrs;

/**
* @brief Constructor
*
* @param id
* @param mgr
*/
SvcRequestCounters::SvcRequestCounters(const std::string &id, FdsCountersMgr *mgr)
    : FdsCounters(id, mgr),
    timedout("timedout", this),
    invokeerrors("invokeerrors", this),
    appsuccess("appsuccess", this),
    apperrors("apperrors", this)
{
}

SvcRequestCounters::~SvcRequestCounters()
{
}

/**
 * Constructor
 */
SvcRequestPool::SvcRequestPool()
{
    gSvcRequestTracker = new SvcRequestTracker();
    gSvcRequestCntrs = new SvcRequestCounters("SvcReq", g_cntrs_mgr.get());

    nextAsyncReqId_ = 0;
    finishTrackingCb_ = std::bind(&SvcRequestTracker::removeFromTracking,
            gSvcRequestTracker, std::placeholders::_1);
}

/**
 *
 */
SvcRequestPool::~SvcRequestPool()
{
    nextAsyncReqId_ = 0;
}

/**
 *
 * @return
 */
fpi::AsyncHdr
SvcRequestPool::newSvcRequestHeader(const SvcRequestId& reqId,
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
boost::shared_ptr<fpi::AsyncHdr> SvcRequestPool::newSvcRequestHeaderPtr(
                                    const SvcRequestId& reqId,
                                    const fpi::SvcUuid &srcUuid,
                                    const fpi::SvcUuid &dstUuid)
{
    boost::shared_ptr<fpi::AsyncHdr> hdr(new fpi::AsyncHdr());
    *hdr = newSvcRequestHeader(reqId, srcUuid, dstUuid);
    return hdr;
}

/**
 * Common handling as part of any async request creation
 * @param req
 */
void SvcRequestPool::asyncSvcRequestInitCommon_(SvcRequestIfPtr req)
{
    gSvcRequestTracker->addForTracking(req->getRequestId(), req);
    req->setCompletionCb(finishTrackingCb_);
    // TODO(Rao): Get this from config
    req->setTimeoutMs(5000);
}

EPSvcRequestPtr
SvcRequestPool::newEPSvcRequest(const fpi::SvcUuid &peerEpId)
{
    auto reqId = nextAsyncReqId_++;
    Platform *plat = Platform::platf_singleton();

    fpi::SvcUuid myEpId;
    if (plat->plf_get_node_type() == fpi::FDSP_ORCH_MGR) {
        fds::assign(myEpId, gl_OmUuid);
    } else {
        fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());
    }
    EPSvcRequestPtr req(new EPSvcRequest(reqId, myEpId, peerEpId));
    asyncSvcRequestInitCommon_(req);

    return req;
}

FailoverSvcRequestPtr SvcRequestPool::newFailoverSvcRequest(
    const EpIdProviderPtr epProvider)
{
    auto reqId = nextAsyncReqId_++;

    fpi::SvcUuid myEpId;
    fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());

    FailoverSvcRequestPtr req(new FailoverSvcRequest(reqId, myEpId, epProvider));
    asyncSvcRequestInitCommon_(req);

    return req;
}

QuorumSvcRequestPtr SvcRequestPool::newQuorumSvcRequest(const EpIdProviderPtr epProvider)
{
    auto reqId = nextAsyncReqId_++;

    fpi::SvcUuid myEpId;
    fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());

    QuorumSvcRequestPtr req(new QuorumSvcRequest(reqId, myEpId, epProvider));
    asyncSvcRequestInitCommon_(req);

    return req;
}


/**
* @brief Common method for posting errors typically encountered in invocation code paths
* for all service requests.  Here the we simulate as if the error is coming from 
* the endpoint.  Error is posted to a threadpool.
*
* @param header
*/
void SvcRequestPool::postError(boost::shared_ptr<fpi::AsyncHdr> &header)
{
    fds_assert(header->msg_code != ERR_OK);

    /* Counter adjustment */
    switch (header->msg_code) {
    case ERR_SVC_REQUEST_INVOCATION:
        gSvcRequestCntrs->invokeerrors.incr();
        break;
    case ERR_SVC_REQUEST_TIMEOUT:
        gSvcRequestCntrs->timedout.incr();
        break;
    }

    /* Simulate an error for remote endpoint */
    boost::shared_ptr<std::string> payload;
    header->msg_type_id = fpi::NullMsgTypeId;
    if (Platform::platf_singleton()->getBaseAsyncSvcHandler())
    Platform::platf_singleton()->getBaseAsyncSvcHandler()->asyncResp(header, payload);
}

}  // namespace fds
