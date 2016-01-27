/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <fds_module_provider.h>
#include <fds_counters.h>
#include <fdsp_utils.h>
#include <FdsRandom.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequest.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestTracker.h>
#include <fiu-control.h>
#include <net/SvcRequestPool.h>

namespace fds {

// TODO(Rao): This shouldn't be global.  We should access it via SvcMgr.
// Lot of code refers to gSvcRequestPool. This will be slowly migrated over
SvcRequestPool *gSvcRequestPool;

SvcRequestId SvcRequestPool::SVC_UNTRACKED_REQ_ID = 0;

template<typename T>
T SvcRequestPool::get_config(std::string const& option)
{ return MODULEPROVIDER()->get_fds_config()->get<T>(option); }

/**
 * Constructor
 */
SvcRequestPool::SvcRequestPool(CommonModuleProviderIf *moduleProvider,
                               const fpi::SvcUuid &selfUuid,
                               PlatNetSvcHandlerPtr handler)
: HasModuleProvider(moduleProvider)
{
    svcRequestTracker_ = new SvcRequestTracker(MODULEPROVIDER());
    svcRequestCntrs_ = new SvcRequestCounters("SvcReq",
                                              MODULEPROVIDER()->get_cntrs_mgr().get());

    selfUuid_ = selfUuid;
    svcReqHandler_ = handler;

    /* Starting the service request id at a random offset with respect to
     * During restart this will in most case help us drop responses to requests
     * issued from previous incarnation of this serivce
     */
    RandNumGenerator rgen(RandNumGenerator::getRandSeed());
    nextAsyncReqId_ = rgen.genNum();
    nextAsyncReqId_ = getNextAsyncReqId_();
    LOGNOTIFY << "Starting service request id at: " << nextAsyncReqId_;

    finishTrackingCb_ = std::bind(&SvcRequestTracker::popFromTracking,
            svcRequestTracker_, std::placeholders::_1);

    if (true == get_config<bool>("fds.pm.svc.lftp.enable")) {
        svcSendTp_.reset(new LFMQThreadpool(get_config<uint32_t>(
                    "fds.pm.svc.lftp.io_thread_cnt")));
        svcWorkerTp_.reset(new LFMQThreadpool(get_config<uint32_t>(
                    "fds.pm.svc.lftp.worker_thread_cnt")));
        fiu_enable("svc.use.lftp", 1, NULL, 0);
    }
    reqTimeout_ = get_config<uint32_t>("fds.pm.svc.timeout.thrift_message");
}

/**
 *
 */
SvcRequestPool::~SvcRequestPool()
{
    nextAsyncReqId_ = SVC_UNTRACKED_REQ_ID;
    delete svcRequestTracker_;
    delete svcRequestCntrs_;
}

/**
 *
 * @return
 */
fpi::AsyncHdr
SvcRequestPool::newSvcRequestHeader(const SvcRequestId& reqId,
                                    const fpi::FDSPMsgTypeId &msgTypeId,
                                    const fpi::SvcUuid &srcUuid,
                                    const fpi::SvcUuid &dstUuid,
                                    const fds_uint64_t dlt_version,
                                    const fpi::ReplicaId &replicaId,
                                    const int32_t &replicaVersion)
{
    fpi::AsyncHdr header;

    header.msg_src_id = reqId;
    header.msg_type_id = msgTypeId;
    header.msg_src_uuid = srcUuid;
    header.msg_dst_uuid = dstUuid;
    header.msg_code = 0;
    header.dlt_version = dlt_version;
    header.replicaId = replicaId;
    header.replicaVersion = replicaVersion;

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
                                    const fpi::SvcUuid &dstUuid,
                                    const fds_uint64_t dlt_version,
                                    const fpi::ReplicaId &replicaId,
                                    const int32_t &replicaVersion)
{
    boost::shared_ptr<fpi::AsyncHdr> hdr(new fpi::AsyncHdr());
    *hdr = newSvcRequestHeader(reqId, msgTypeId, srcUuid,
                               dstUuid, dlt_version, replicaId, replicaVersion);
    return hdr;
}

/**
* @brief Swaps request header
*
* @param req_hdr
*
* @return 
*/
fpi::AsyncHdr
SvcRequestPool::swapSvcReqHeader(const fpi::AsyncHdr &reqHdr)
{
    auto respHdr = reqHdr;
    respHdr.msg_src_uuid = reqHdr.msg_dst_uuid;
    respHdr.msg_dst_uuid = reqHdr.msg_src_uuid;
    return respHdr;
}
/**
 * Common handling as part of any async request creation
 * @param req
 */
void SvcRequestPool::asyncSvcRequestInitCommon_(SvcRequestIfPtr req)
{
    // Initialize this once the first time it's used from the config file

    req->setCompletionCb(finishTrackingCb_);
    req->setTimeoutMs(reqTimeout_);
    svcRequestTracker_->addForTracking(req->getRequestId(), req);
}

EPSvcRequestPtr
SvcRequestPool::newEPSvcRequest(const fpi::SvcUuid &peerEpId, int minor_version)
{
    auto reqId = getNextAsyncReqId_();
    
    // TODO(Rao): Kept here just for reference.  Once we totally decouple for platform
    // remove this code
#if 0
    fpi::SvcUuid myEpId;
    if (plat->plf_get_node_type() == fpi::FDSP_ORCH_MGR) {
        fds::assign(myEpId, gl_OmUuid);
    } else {
        fds::assign(myEpId, *Platform::plf_get_my_svc_uuid());
    }
#endif

    EPSvcRequestPtr req(new EPSvcRequest(MODULEPROVIDER(), reqId, selfUuid_, peerEpId));
    req->set_minor(minor_version);
    asyncSvcRequestInitCommon_(req);

    return req;
}

FailoverSvcRequestPtr SvcRequestPool::newFailoverSvcRequest(const EpIdProviderPtr epProvider,
                                                            fds_uint64_t const dlt_version)
{
    auto reqId = getNextAsyncReqId_();

    FailoverSvcRequestPtr req(new FailoverSvcRequest(MODULEPROVIDER(), reqId, selfUuid_, dlt_version, epProvider));
    asyncSvcRequestInitCommon_(req);

    return req;
}

FailoverSvcRequestPtr
SvcRequestPool::newFailoverSvcRequest(const std::vector<fpi::SvcUuid> &svcUuids, 
                                      fds_uint64_t const dlt_version)
{
    auto reqId = getNextAsyncReqId_();

    FailoverSvcRequestPtr req(new FailoverSvcRequest(MODULEPROVIDER(), reqId, selfUuid_, dlt_version, svcUuids));
    asyncSvcRequestInitCommon_(req);

    return req;
}

QuorumSvcRequestPtr SvcRequestPool::newQuorumSvcRequest(const EpIdProviderPtr epProvider, fds_uint64_t const dlt_ver)
{
    auto reqId = getNextAsyncReqId_();

    QuorumSvcRequestPtr req(new QuorumSvcRequest(MODULEPROVIDER(), reqId, selfUuid_, dlt_ver, epProvider));
    asyncSvcRequestInitCommon_(req);

    return req;
}

MultiPrimarySvcRequestPtr SvcRequestPool::newMultiPrimarySvcRequest(
    const std::vector<fpi::SvcUuid>& primarySvcs,
    const std::vector<fpi::SvcUuid>& optionalSvcs,
    fds_uint64_t const dlt_version)
{
    auto reqId = getNextAsyncReqId_();
    MultiPrimarySvcRequestPtr req(new MultiPrimarySvcRequest(MODULEPROVIDER(), reqId, selfUuid_, dlt_version, primarySvcs, optionalSvcs));
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
        svcRequestCntrs_->invokeerrors.incr();
        break;
    case ERR_SVC_REQUEST_TIMEOUT:
        svcRequestCntrs_->timedout.incr();
        break;
    }

    if (static_cast<SvcRequestId>(header->msg_src_id) == SVC_UNTRACKED_REQ_ID) {
        // TODO(Rao): For untracked requests invoke the error callback if one specified
        return;
    }

    /* Simulate an error for remote endpoint */
    boost::shared_ptr<std::string> payload;
    svcReqHandler_->asyncResp(header, payload);
}

void SvcRequestPool::postEmptyResponse(boost::shared_ptr<fpi::AsyncHdr> &header) {
    header->msg_code = ERR_OK;
    header->msg_type_id = FDSP_MSG_TYPEID(fpi::EmptyMsg); 
    boost::shared_ptr<std::string> payload;
    svcReqHandler_->asyncResp(header, payload);
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
    LOGDEBUG << "IO threadpool stats:\n";
    uint32_t i = 0;
    for (auto &w : svcSendTp_->workers) {
        LOGDEBUG << "Id: " << i << " completedCnt: " << w->completedCntr << std::endl;
    }

    LOGDEBUG << "Worker threadpool stats:\n";
    i = 0;
    for (auto &w : svcWorkerTp_->workers) {
        LOGDEBUG << "Id: " << i << " completedCnt: " << w->completedCntr << std::endl;
    }
}

SvcRequestCounters* SvcRequestPool::getSvcRequestCntrs() const
{
    return svcRequestCntrs_;
}

SvcRequestTracker* SvcRequestPool::getSvcRequestTracker() const
{
    return svcRequestTracker_;
}
void SvcRequestPool::setDltManager(DLTManagerPtr dltManager) {
    dltMgr = dltManager;
}

}  // namespace fds
