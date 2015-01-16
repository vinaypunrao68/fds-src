/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <net/net-service.h>
#include "platform/platform.h"
#include <fdsp_utils.h>
#include <fds_process.h>
#include <net/BaseAsyncSvcHandler.h>
#include <net/SvcRequestTracker.h>
#include <net/SvcRequestPool.h>
#include <fiu-control.h>

namespace fds {

// TODO(Rao): Make SvcRequestPool and SvcRequestTracker a module
SvcRequestPool *gSvcRequestPool;
SvcRequestCounters* gSvcRequestCntrs;
SvcRequestTracker* gSvcRequestTracker;

template<typename T>
T SvcRequestPool::get_config(std::string const& option)
{ return gModuleProvider->get_fds_config()->get<T>(option); }

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

    svcSendTp_.reset(new LFMQThreadpool(get_config<uint32_t>(
                "fds.plat.svc.lftp.io_thread_cnt")));
    svcWorkerTp_.reset(new LFMQThreadpool(get_config<uint32_t>(
                "fds.plat.svc.lftp.worker_thread_cnt")));
    if (true == get_config<bool>("fds.plat.svc.lftp.enable")) {
        fiu_enable("svc.use.lftp", 1, NULL, 0);
    }
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
                                    const fpi::FDSPMsgTypeId &msgTypeId,
                                    const fpi::SvcUuid &srcUuid,
                                    const fpi::SvcUuid &dstUuid)
{
    fpi::AsyncHdr header;

    header.msg_src_id = reqId;
    header.msg_type_id = msgTypeId;
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
                                    const fpi::FDSPMsgTypeId &msgTypeId,
                                    const fpi::SvcUuid &srcUuid,
                                    const fpi::SvcUuid &dstUuid)
{
    boost::shared_ptr<fpi::AsyncHdr> hdr(new fpi::AsyncHdr());
    *hdr = newSvcRequestHeader(reqId, msgTypeId, srcUuid, dstUuid);
    return hdr;
}

/**
 * Common handling as part of any async request creation
 * @param req
 */
void SvcRequestPool::asyncSvcRequestInitCommon_(SvcRequestIfPtr req)
{
    // Initialize this once the first time it's used from the config file
    static auto const timeout = get_config<uint32_t>("fds.plat.svc.timeout.thrift_message");

    req->setCompletionCb(finishTrackingCb_);
    req->setTimeoutMs(timeout);
    gSvcRequestTracker->addForTracking(req->getRequestId(), req);
}

EPSvcRequestPtr
SvcRequestPool::newEPSvcRequest(const fpi::SvcUuid &peerEpId, int minor_version)
{
    auto reqId = ++nextAsyncReqId_;
    Platform *plat = Platform::platf_singleton();

    fpi::SvcUuid myEpId;
    if (plat->plf_get_node_type() == fpi::FDSP_ORCH_MGR) {
        fds::assign(myEpId, gl_OmUuid);
    } else {
        fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());
    }
    EPSvcRequestPtr req(new EPSvcRequest(reqId, myEpId, peerEpId));
    req->set_minor(minor_version);
    asyncSvcRequestInitCommon_(req);

    return req;
}

FailoverSvcRequestPtr SvcRequestPool::newFailoverSvcRequest(
    const EpIdProviderPtr epProvider)
{
    auto reqId = ++nextAsyncReqId_;

    fpi::SvcUuid myEpId;
    fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());

    FailoverSvcRequestPtr req(new FailoverSvcRequest(reqId, myEpId, epProvider));
    asyncSvcRequestInitCommon_(req);

    return req;
}

QuorumSvcRequestPtr SvcRequestPool::newQuorumSvcRequest(const EpIdProviderPtr epProvider)
{
    auto reqId = ++nextAsyncReqId_;

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
    Platform::platf_singleton()->getBaseAsyncSvcHandler()->asyncResp(header, payload);
}

LFMQThreadpool* SvcRequestPool::getSvcSendThreadpool()
{
    return svcSendTp_.get();
}

LFMQThreadpool* SvcRequestPool::getSvcWorkerThreadpool()
{
    return svcWorkerTp_.get();
}

void SvcRequestPool::dumpLFTPStats()
{
    std::cout << "IO threadpool stats:\n";
    uint32_t i = 0;
    for (auto &w : svcSendTp_->workers) {
        std::cout << "Id: " << i << " completedCnt: " << w->completedCntr << std::endl;
    }

    std::cout << "Worker threadpool stats:\n";
    i = 0;
    for (auto &w : svcWorkerTp_->workers) {
        std::cout << "Id: " << i << " completedCnt: " << w->completedCntr << std::endl;
    }
}

}  // namespace fds
