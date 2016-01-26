/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>
#include <vector>
#include <thread>

#include <concurrency/ThreadPool.h>
#include <net/SvcMgr.h>
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <fds_module_provider.h>
#include <util/fiu_util.h>
#include <util/timeutils.h>
#include <thrift/transport/TTransportUtils.h>  // For TException.
#include "fdsp/om_api_types.h"

namespace fds {

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
    apperrors("apperrors", this),
    serializationLat("serializationLat", this),
    deserializationLat("deserializationLat", this),
    sendPayloadLat("sendPayloadLat", this),
    sendLat("sendLat", this),
    reqLat("reqLat", this)
{
    (new SimpleNumericCounter("service.start.timestamp",this))->set(util::getTimeStampSeconds());
}

SvcRequestCounters::~SvcRequestCounters()
{
}

SvcRequestTimer::SvcRequestTimer(CommonModuleProviderIf* provider,
                                 const SvcRequestId &id,
                                 const fpi::FDSPMsgTypeId &msgTypeId,
                                 const fpi::SvcUuid &myEpId,
                                 const fpi::SvcUuid &peerEpId,
                                 const fpi::ReplicaId &replicaId,
                                 const int32_t &replicaVersion)
    : HasModuleProvider(provider),
    FdsTimerTask(*(MODULEPROVIDER()->getTimer()))
{
    header_.reset(new fpi::AsyncHdr());
    *header_ = MODULEPROVIDER()->getSvcMgr()->\
               getSvcRequestMgr()->newSvcRequestHeader(id, msgTypeId,
                                                       peerEpId, myEpId,
                                                       DLT_VER_INVALID,
                                                       replicaId, replicaVersion);
    header_->msg_code = ERR_SVC_REQUEST_TIMEOUT;
}

/**
* @brief Sends a timeout error BaseAsyncSvcHandler.  Note this call is executed
* on a threadpool
*/
void SvcRequestTimer::runTimerTask()
{
    GLOGWARN << "Timeout: " << fds::logString(*header_);
    MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->postError(header_);
}

TrackableRequest::TrackableRequest()
: TrackableRequest(nullptr, SvcRequestPool::SVC_UNTRACKED_REQ_ID)
{
}

TrackableRequest::TrackableRequest(CommonModuleProviderIf* provider,
                               const SvcRequestId &id)
: HasModuleProvider(provider),
  id_(id),
  state_(PRIOR_INVOCATION),
  timeoutMs_(0)
{
}

inline bool TrackableRequest::isTracked() const {
    return id_ != SvcRequestPool::SVC_UNTRACKED_REQ_ID;
}

/**
 * Marks the svc request as complete and invokes the completion callback
 * @param error
 */
void TrackableRequest::complete(const Error& error) {
    DBG(GLOGDEBUG << logString() << " " << error);

    fds_assert(state_ != SVC_REQUEST_COMPLETE);
    state_ = SVC_REQUEST_COMPLETE;

    if (timer_) {
        MODULEPROVIDER()->getTimer()->cancel(timer_);
        timer_.reset();
    }

    if (completionCb_) {
        completionCb_(id_);
    }
}

SvcRequestIf::SvcRequestIf()
: SvcRequestIf(nullptr, SvcRequestPool::SVC_UNTRACKED_REQ_ID, fpi::SvcUuid())
{
}

SvcRequestIf::SvcRequestIf(CommonModuleProviderIf* provider,
                           const SvcRequestId &id,
                           const fpi::SvcUuid &myEpId)
    : TrackableRequest(provider, id),
    teidIsSet_(false),
    myEpId_(myEpId),
    fireAndForget_(false),
    minor_version(0)
{
}

SvcRequestIf::~SvcRequestIf()
{
}


void SvcRequestIf::setPayloadBuf(const fpi::FDSPMsgTypeId &msgTypeId,
                                 const boost::shared_ptr<std::string> &buf)
{
    fds_assert(!payloadBuf_);
    msgTypeId_ = msgTypeId;
    payloadBuf_ = buf;
}


/**
* @brief Marks the svc request as complete and invokes the completion callback
*
* @param error
* @param header
* @param payload
*/
void SvcRequestIf::completeReq(const Error& error,
                            const fpi::AsyncHdrPtr& header,
                            const StringPtr& payload) {
    respHeader_ = header;
    respPayload_ = payload;
    complete(error);
}

TaskExecutorId SvcRequestIf::getTaskExecutorId() {
    if (teidIsSet_) {
        return teid_;
    } else {
        return id_;
    }
}

void SvcRequestIf::setTaskExecutorId(const TaskExecutorId &teid) {
    teidIsSet_ = true;
    teid_ = teid;
}

void SvcRequestIf::unsetTaskExecutorId() {
    teidIsSet_ = false;
}

bool SvcRequestIf::taskExecutorIdIsSet() {
    return teidIsSet_;
}

/**
 * Common invocation method for all async requess.
 * In order to synchronize invocation and response handling  we use
 * SvcMgr:getTaskExecutor to schedule request specific invocatio work here.
 */
void SvcRequestIf::invoke()
{
    /* Disable to scheduling thread pool */
    fiu_do_on("svc.disable.schedule", invokeWork_(); return;);

    SynchronizedTaskExecutor<uint64_t>* taskExecutor = 
        MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
    /* Execute on synchronized task executor so that invocation and response
     * handling is synchronized.
     */
    if (teidIsSet_) {
        taskExecutor->scheduleOnHashKey(teid_, std::bind(&SvcRequestIf::invokeWork_, this));
    } else {
        taskExecutor->scheduleOnHashKey(static_cast<size_t>(id_),
                                        std::bind(&SvcRequestIf::invokeWork_, this));
    }
}


/**
 * Make sure access to this function is synchronized.  Sending and handling
 * of the response SHOULDN'T happen simultaneously.  Currently we ensure this by
 * using SvcMgr::taskExecutor_
 * @param epId
 */
void EPSvcRequest::sendPayload_()
{
    auto header = MODULEPROVIDER()->getSvcMgr()->\
    getSvcRequestMgr()->newSvcRequestHeaderPtr(id_, msgTypeId_, myEpId_, peerEpId_,
                                               dlt_version_, replicaId_, replicaVersion_);
    header->msg_type_id = msgTypeId_;

    DBG(GLOGDEBUG << fds::logString(*header));

    try {
        /* Fault injection */
        fiu_do_on("svc.fail.sendpayload_before",
                  throw util::FiuException("svc.fail.sendpayload_before"));
        /* send the payload */
        MODULEPROVIDER()->getSvcMgr()->sendAsyncSvcReqMessage(header, payloadBuf_);

        /* For fire and forget message simulate dummy response from endpoint */
        if (fireAndForget_) {
            DBG(LOGDEBUG << "Schedule empty response for Fire and forget request: "
                << logString());
            auto respHdr = MODULEPROVIDER()->getSvcMgr()->\
                           getSvcRequestMgr()->newSvcRequestHeaderPtr(id_, msgTypeId_,
                                                                      peerEpId_, myEpId_,
                                                                      dlt_version_,
                                                                      replicaId_,
                                                                      replicaVersion_);
            MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->postEmptyResponse(respHdr);
            return;
        }

       /* start the timer */
       if (timeoutMs_) {
           timer_.reset(new SvcRequestTimer(MODULEPROVIDER(), id_,
                                            msgTypeId_, myEpId_,
                                            peerEpId_, replicaId_,
                                            replicaVersion_));
           bool ret = MODULEPROVIDER()->getTimer()->\
                      schedule(timer_, std::chrono::milliseconds(timeoutMs_));
           fds_assert(ret == true);
       }
    } catch(std::exception &e) {
        auto respHdr = MODULEPROVIDER()->getSvcMgr()->\
                       getSvcRequestMgr()->newSvcRequestHeaderPtr(id_, msgTypeId_,
                                                                  peerEpId_, myEpId_,
                                                                  dlt_version_,
                                                                  replicaId_,
                                                                  replicaVersion_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code
            << " exception: " << e.what();
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->postError(respHdr);
        fds_assert(!"Unknown exception");
    } catch(...) {
        auto respHdr = MODULEPROVIDER()->getSvcMgr()->\
                       getSvcRequestMgr()->newSvcRequestHeaderPtr(id_, msgTypeId_,
                                                                  peerEpId_, myEpId_,
                                                                  dlt_version_,
                                                                  replicaId_,
                                                                  replicaVersion_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code;
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->postError(respHdr);
        fds_assert(!"Unknown exception");
    }
}

std::stringstream& SvcRequestIf::logSvcReqCommon_(std::stringstream &oss,
                                                       const std::string &type)
{
    oss << " " << type << " Req Id: " << id_
        << std::hex
        << " From: " << myEpId_.svc_uuid
        << std::dec;
    return oss;
}

EPSvcRequest::EPSvcRequest()
    : EPSvcRequest(nullptr, 0, fpi::SvcUuid(), fpi::SvcUuid())
{
}

EPSvcRequest::EPSvcRequest(CommonModuleProviderIf* provider,
                           const SvcRequestId &id,
                           const fpi::SvcUuid &myEpId,
                           const fpi::SvcUuid &peerEpId)
    : SvcRequestIf(provider, id, myEpId)
{
    peerEpId_ = peerEpId;
    replicaId_ = 0;
    replicaVersion_ = 0;
}

EPSvcRequest::~EPSvcRequest()
{
}

void EPSvcRequest::invoke() {
    if (!respCb_) {
        fireAndForget_ = true;
    }
    SvcRequestIf::invoke();
}

/**
* @brief Invoke without any synchronization.  Use it when calling from already
* synchronized context
*/
void EPSvcRequest::invokeDirect()
{
    if (!respCb_) {
        fireAndForget_ = true;
    }
    invokeWork_();
}

/**
* @brief Worker function for doing the invocation work
* NOTE this function is executed on SvcMgr::taskExecutor_ for synchronization
*/
void EPSvcRequest::invokeWork_()
{
    fds_verify(state_ == PRIOR_INVOCATION);

    // TODO(Rao): Determine endpoint is healthy or not
    state_ = INVOCATION_PROGRESS;

    SVCPERF(util::StopWatch sw; sw.start());
    sendPayload_();
    SVCPERF(MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->\
            sendPayloadLat.update(sw.getElapsedNanos()));
}

/**
 * @brief
 *
 * @param header
 * @param payload
 * NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
 */
void EPSvcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));

    if (isComplete()) {
        /* Request is already complete.  At this point we don't do anything on
         * the responses than just draining them out
         */
        return;
    }
    /* Complete the request */
    complete(header->msg_code);

    /* Notify actionable error to endpoint manager */
    if (header->msg_code != ERR_OK &&
        MODULEPROVIDER()->getSvcMgr()->isSvcActionableError(header->msg_code)) {
        MODULEPROVIDER()->getSvcMgr()->handleSvcError(header->msg_src_uuid, header->msg_code);
    }

    /* Invoke response callback */
    if (respCb_) {
        SVCPERF(ts.rspHndlrTs = util::getTimeStampNanos());
        respCb_(this, header->msg_code, payload);
    }

    /* adjust counters */
    if (header->msg_code == ERR_OK) {
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->appsuccess.incr();
    } else {
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->apperrors.incr();
    }
}

/**
* @brief
*
* @return
*/
std::string EPSvcRequest::logString()
{
    std::stringstream oss;
    logSvcReqCommon_(oss, "EPSvcRequest")
        << std::hex << " To: " << peerEpId_.svc_uuid
        << std::dec;
    return oss.str();
}

/**
* @brief
*
* @param cb
*/
void EPSvcRequest::onResponseCb(EPSvcRequestRespCb cb)
{
    respCb_ = cb;
}

/**
* @brief
*
* @return
*/
fpi::SvcUuid EPSvcRequest::getPeerEpId() const
{
    return peerEpId_;
}

/**
* @brief
*/
MultiEpSvcRequest::MultiEpSvcRequest()
    : SvcRequestIf(nullptr, 0, fpi::SvcUuid())
{
}

/**
* @brief
*
* @param id
* @param myEpId
* @param peerEpIds
*/
MultiEpSvcRequest::MultiEpSvcRequest(CommonModuleProviderIf* provider,
                                     const SvcRequestId& id,
                                     const fpi::SvcUuid &myEpId,
                                     fds_uint64_t const dlt_version,
                                     const std::vector<fpi::SvcUuid>& peerEpIds)
    : SvcRequestIf(provider, id, myEpId)
{
    for (const auto &uuid : peerEpIds) {
        addEndpoint(uuid, dlt_version, 0, 0);
    }
    dlt_version_ = dlt_version;
}

/**
 * For adding endpoint.
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param uuid
 */
void MultiEpSvcRequest::addEndpoint(const fpi::SvcUuid& peerEpId,
                                    fds_uint64_t const dlt_version,
                                    const fpi::ReplicaId &replicaId,
                                    const int32_t &replicaVersion)
{
    epReqs_.push_back(EPSvcRequestPtr(
            new EPSvcRequest(MODULEPROVIDER(), id_, myEpId_, peerEpId)));
    // Tag this against a specific DLT
    epReqs_.back()->dlt_version_ = dlt_version;
    epReqs_.back()->setReplicaId(replicaId);
    epReqs_.back()->setReplicaVersion(replicaVersion);
}

/**
 * For adding endpoints.
 * NOTE: Only invoke during initialization.  Do not invoke once
 * the request is in progress
 * @param peerEpIds
 */
void MultiEpSvcRequest::addEndpoints(const std::vector<fpi::SvcUuid> &peerEpIds,
                                     fds_uint64_t const dlt_version)
{
    for (const auto &uuid : peerEpIds) {
        addEndpoint(uuid, dlt_version, 0, 0);
    }
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param cb
 */
void MultiEpSvcRequest::onEPAppStatusCb(EPAppStatusCb cb)
{
    epAppStatusCb_ = cb;
}

/**
* @brief Returns the endpoint request identified by epId
* NOTE: If we manage a lot of endpoints, we should consider using a map
* here.
* @param epId
*
* @return
*/
EPSvcRequestPtr MultiEpSvcRequest::getEpReq_(const fpi::SvcUuid &peerEpId)
{
    for (const auto &ep : epReqs_) {
        if (ep->getPeerEpId() == peerEpId) {
            return ep;
        }
    }
    return nullptr;
}

/**
 *
 */
FailoverSvcRequest::FailoverSvcRequest()
: FailoverSvcRequest(nullptr, 0, fpi::SvcUuid(), DLT_VER_INVALID, std::vector<fpi::SvcUuid>())
{
}


/**
* @brief
*
* @param id
* @param myEpId
* @param peerEpIds
*/
FailoverSvcRequest::FailoverSvcRequest(CommonModuleProviderIf* provider,
                                       const SvcRequestId& id,
                                       const fpi::SvcUuid &myEpId,
                                       fds_uint64_t const dlt_version,
                                       const std::vector<fpi::SvcUuid>& peerEpIds)
    : MultiEpSvcRequest(provider, id, myEpId, dlt_version, peerEpIds),
      curEpIdx_(0)
{
}

/**
* @brief
*
* @param id
* @param myEpId
* @param epProvider
*/
// TODO(Rao): Need to store epProvider.  So that we iterate endpoint
// Ids when needes as opposed to using getEps(), like we are doing
// now.
FailoverSvcRequest::FailoverSvcRequest(CommonModuleProviderIf* provider,
                                       const SvcRequestId& id,
                                       const fpi::SvcUuid &myEpId,
                                       fds_uint64_t const dlt_version,
                                       const EpIdProviderPtr epProvider)
    : FailoverSvcRequest(provider, id, myEpId, dlt_version, epProvider->getEps())
{
}

FailoverSvcRequest::~FailoverSvcRequest()
{
}


void FailoverSvcRequest::invoke() {
    /* Response callback must be set for failover request.  Fire and forget isn't
     * supported
     */
    fds_verify(respCb_ && "Response callback must be set for failover requests");
    SvcRequestIf::invoke();
}

/**
 * Invocation work function
 * NOTE this function is executed on SvcMgr::taskExecutor_ for synchronization
 */
void FailoverSvcRequest::invokeWork_()
{
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    state_ = INVOCATION_PROGRESS;

    if (healthyEpExists) {
        epReqs_[curEpIdx_]->invokeWork_();
    } else {
        DBG(GLOGDEBUG << logString() << " No healthy endpoints left");
        fds_assert(curEpIdx_ == epReqs_.size() - 1);
        /* No healthy endpoints left.  Lets post an error.  This error
         * we will simulate as if the error is from last endpoint.
         */
        auto respHdr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->\
                       newSvcRequestHeaderPtr(id_,
                                              msgTypeId_,
                                              epReqs_[curEpIdx_]->peerEpId_,
                                              myEpId_,
                                              epReqs_[curEpIdx_]->dlt_version_,
                                              epReqs_[curEpIdx_]->replicaId_,
                                              epReqs_[curEpIdx_]->replicaVersion_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr()->postError(respHdr);
    }
}

/**
 * @brief
 * Handles asynchronous response.  Response source can be from
 * 1. Network
 * 2. Local where we simulate an error.  This can happen when invocation fails
 * immediately
 * NOTE: We make the assumption that response always comes on a different thread
 * than message send thread
 *
 * Success case handling: Invoke the registered response cb
 *
 * Error case handling: Invoke application error handler (Only for application errors)
 * if one is registered.  Move on to the next healthy endpont and send the message. If
 * we don't have healthy endpoints, then invoke response cb with error.
 *
 * @param header
 * @param payload
 * NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
 */
// TODO(Rao): logging, invoking cb, error for each endpoint
void FailoverSvcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
                                        boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));

    fpi::SvcUuid errdEpId;

    if (isComplete()) {
        /* Request is already complete.  At this point we don't do anything on
         * the responses than just draining them out
         */
        return;
    }

    if (header->msg_src_uuid != epReqs_[curEpIdx_]->peerEpId_) {
        /* Response isn't from the last endpoint we issued the request against.
         * Don't do anything here
         */
        // TODO(Rao): We may need special handling for success case here
        return;
    }

    epReqs_[curEpIdx_]->complete(header->msg_code);

    bool bSuccess = (header->msg_code == ERR_OK);

    /* Handle the error */
    if (header->msg_code != ERR_OK) {
        /* Notify actionable error to endpoint manager */
        if (MODULEPROVIDER()->getSvcMgr()->isSvcActionableError(header->msg_code)) {
            MODULEPROVIDER()->getSvcMgr()->\
                handleSvcError(header->msg_src_uuid, header->msg_code);
        } else {
            /* Handle Application specific errors here */
            if (epAppStatusCb_) {
                bSuccess = epAppStatusCb_(header->msg_code, payload);
            }
        }
        if (!bSuccess) GLOGWARN << fds::logString(*header);
    }

    /* Handle the case where response from this endpoint is considered success */
    if (bSuccess) {
        complete(ERR_OK);
        if (respCb_) {
            SVCPERF(ts.rspHndlrTs = util::getTimeStampNanos());
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->appsuccess.incr();
        return;
    }

    /* Move to the next healhy endpoint and invoke */
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    if (!healthyEpExists) {
        complete(ERR_SVC_REQUEST_FAILED);
        if (respCb_) {
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->apperrors.incr();
        return;
    }

    epReqs_[curEpIdx_]->invokeWork_();
}

/**
* @brief
*
* @return
*/
std::string FailoverSvcRequest::logString()
{
    std::stringstream oss;
    logSvcReqCommon_(oss, "FailoverSvcRequest")
        << " curEpIdx: " << static_cast<uint32_t>(curEpIdx_);
    return oss.str();
}

/**
 * Moves to the next healthy endpoint in the sequence start from curEpIdx_
 * @return True if healthy endpoint is found in the sequence.  False otherwise
 */
bool FailoverSvcRequest::moveToNextHealthyEndpoint_()
{
    if (state_ == PRIOR_INVOCATION) {
        curEpIdx_ = 0;
    } else {
        ++curEpIdx_;
    }

    for (; curEpIdx_ < epReqs_.size(); ++curEpIdx_) {
        // TODO(Rao): Pass the right message version id
        Error epStatus = ERR_OK;
        // TODO(Rao): Uncomment once endpoint_lookup is implemented
        #if 0
        auto ep = NetMgr::ep_mgr_singleton()->\
                    endpoint_lookup(epReqs_[curEpIdx_]->peerEpId_);

        if (ep == nullptr) {
            epStatus = ERR_EP_NON_EXISTANT;
        } else {
            epStatus = ep->ep_get_status();
        }
        #endif

        if (epStatus == ERR_OK) {
            epReqs_[curEpIdx_]->setPayloadBuf(msgTypeId_, payloadBuf_);
            epReqs_[curEpIdx_]->setTimeoutMs(timeoutMs_);
            DBG(GLOGDEBUG << logString() << " Healthy endpoint: "
                << epReqs_[curEpIdx_]->peerEpId_.svc_uuid << " idx: " << curEpIdx_);
            return true;
        } else {
            /* When ep is not healthy invoke complete on associated ep request, except
             * the last ep request.  For the last unhealthy ep, complete is invoked in
             * handleResponse()
             */
            if (curEpIdx_ != epReqs_.size() - 1) {
                epReqs_[curEpIdx_]->complete(epStatus);
            }
            DBG(GLOGDEBUG << logString() << " Unhealthy endpoint: "
                << epReqs_[curEpIdx_]->peerEpId_.svc_uuid << " idx: " << curEpIdx_);
        }
    }

    /* We've exhausted all the endpoints.  Decrement so that curEpIdx_ stays valid. Next
     * we will post an error to simulated an error from last endpoint.  This will get
     * handled in handleResponse().  We do this so that user registered callbacks are
     * invoked.
     */
    fds_assert(curEpIdx_ == epReqs_.size());
    --curEpIdx_;

    return false;
}

/**
* @brief
*
* @param cb
*/
void FailoverSvcRequest::onResponseCb(FailoverSvcRequestRespCb cb)
{
    respCb_ = cb;
}

/**
* @brief
*/
QuorumSvcRequest::QuorumSvcRequest()
    : QuorumSvcRequest(nullptr, 0, fpi::SvcUuid(), DLT_VER_INVALID, std::vector<fpi::SvcUuid>())
{
}

/**
* @brief
*
* @param id
* @param myEpId
* @param peerEpIds
*/
QuorumSvcRequest::QuorumSvcRequest(CommonModuleProviderIf *provider,
                                   const SvcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   fds_uint64_t const dlt_ver,
                                   const std::vector<fpi::SvcUuid>& peerEpIds)
    : MultiEpSvcRequest(provider, id, myEpId, dlt_ver, peerEpIds)
{
    successAckd_ = 0;
    errorAckd_ = 0;
    quorumCnt_ = peerEpIds.size();
    waitForAllResponses_ = false;
}

/**
* @brief
*
* @param id
* @param myEpId
* @param epProvider
*/
// TODO(Rao): Need to store epProvider.  So that we iterate endpoint
// Ids when needes as opposed to using getEps(), like we are doing
// now.
QuorumSvcRequest::QuorumSvcRequest(CommonModuleProviderIf* provider,
                                   const SvcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   fds_uint64_t const dlt_ver,
                                   const EpIdProviderPtr epProvider)
: QuorumSvcRequest(provider, id, myEpId, dlt_ver, epProvider->getEps())
{
}
/**
* @brief
*/
QuorumSvcRequest::~QuorumSvcRequest()
{
}

/**
* @brief
*
* @param cnt
*/
void QuorumSvcRequest::setQuorumCnt(const uint32_t cnt)
{
    quorumCnt_ = cnt;
}


void QuorumSvcRequest::setWaitForAllResponses(bool flag)
{
    waitForAllResponses_ = flag;
}

void QuorumSvcRequest::invoke() {
    if (!respCb_) {
        fireAndForget_ = true;
        for (auto &ep : epReqs_) {
            ep->fireAndForget_ = true;
        }
    }

    SvcRequestIf::invoke();
}

/**
* @brief Inovcation work function
* NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
*/
void QuorumSvcRequest::invokeWork_()
{
    for (auto &ep : epReqs_) {
        ep->setPayloadBuf(msgTypeId_, payloadBuf_);
        ep->setTimeoutMs(timeoutMs_);
        ep->invokeWork_();
    }
}

/**
* @brief
*
* @param header
* @param payload
* NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
*/
void QuorumSvcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
                                      boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));

    fpi::SvcUuid errdEpId;

    auto epReq = getEpReq_(header->msg_src_uuid);
    if (isComplete()) {
        /* Drop completed requests */
        GLOGWARN << logString() << " Already completed";
        return;
    } else if (!epReq) {
        /* Drop responses from uknown endpoint src ids */
        GLOGWARN << logString() << " Unkonwn EpId";
        return;
    }

    epReq->completeReq(header->msg_code, header, payload);

    bool bSuccess = (header->msg_code == ERR_OK);
    /* Handle the error */
    if (header->msg_code != ERR_OK) {
        GLOGWARN << fds::logString(*header);
        /* Notify actionable error to service manager */
        if (MODULEPROVIDER()->getSvcMgr()->isSvcActionableError(header->msg_code)) {
            MODULEPROVIDER()->getSvcMgr()->\
                handleSvcError(header->msg_src_uuid, header->msg_code);
        } else {
            /* Handle Application specific errors here */
            if (epAppStatusCb_) {
                bSuccess = epAppStatusCb_(header->msg_code, payload);
            }
        }
    }

    /* Update the endpoint ack counts */
    if (bSuccess) {
        ++successAckd_;
    } else {
        if (ERR_OK == response_) {
            response_ = header->msg_code;
        }
        ++errorAckd_;
    }

    /* Take action based on the ack counts */
    if (successAckd_ == quorumCnt_) {
        if (respCb_ && !waitForAllResponses_) {
            SVCPERF(ts.rspHndlrTs = util::getTimeStampNanos());
            respCb_(this, ERR_OK, payload);
            respCb_ = nullptr;
        }
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->appsuccess.incr();
    } else if (errorAckd_ > (epReqs_.size() - quorumCnt_)) {
        if (respCb_ && !waitForAllResponses_) {
            /* NOTE: We are using first non-ERR_OK code in this case */
            respCb_(this, response_, payload);
            respCb_ = nullptr;
        }
        MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->apperrors.incr();
    }

    if (successAckd_+ errorAckd_ == epReqs_.size()) {
        auto completionCode = (successAckd_ >= quorumCnt_) ? ERR_OK : response_;
        if (waitForAllResponses_ && respCb_) {
            respCb_(this, completionCode, payload);
            respCb_ = nullptr;
        }
        /* Recevied all responses */
        complete(completionCode);
    }
}

/**
* @brief
*
* @return
*/
std::string QuorumSvcRequest::logString()
{
    std::stringstream oss;
    logSvcReqCommon_(oss, "QuorumSvcRequest");
    return oss.str();
}

void QuorumSvcRequest::onResponseCb(QuorumSvcRequestRespCb cb)
{
    respCb_ = cb;
}

MultiPrimarySvcRequest::MultiPrimarySvcRequest(CommonModuleProviderIf* provider,
                                               const SvcRequestId& id,
                                               const fpi::SvcUuid &myEpId,
                                               fds_uint64_t const dlt_version,
                                               const std::vector<fpi::SvcUuid>& primarySvcs,
                                               const std::vector<fpi::SvcUuid>& optionalSvcs)
: MultiEpSvcRequest(provider, id, myEpId, dlt_version, {})
{
    primaryAckdCnt_ = 0;
    totalAckdCnt_ = 0;
    addEndpoints(primarySvcs, dlt_version);
    addEndpoints(optionalSvcs, dlt_version);
    primariesCnt_ = primarySvcs.size();
}

void MultiPrimarySvcRequest::invoke() {
    /* Fire and forget is disallowed */
    fds_verify(respCb_);
    MultiEpSvcRequest::invoke();
}

std::string MultiPrimarySvcRequest::logString()
{
    std::stringstream oss;
    logSvcReqCommon_(oss, "MultiPrimarySvcRequest");
    oss << " primaries cnt: " << unsigned(primariesCnt_);
    return oss.str();
}

void MultiPrimarySvcRequest::invokeWork_()
{
    for (auto &ep : epReqs_) {
        ep->setPayloadBuf(msgTypeId_, payloadBuf_);
        ep->setTimeoutMs(timeoutMs_);
        ep->invokeWork_();
    }
}

EPSvcRequestPtr MultiPrimarySvcRequest::getEpReq_(const fpi::SvcUuid &peerEpId,
                                                  bool &isPrimary)
{
    int idx = 0;
    isPrimary = false;

    for (auto ep : epReqs_) {
        if (ep->getPeerEpId() == peerEpId) {
            if (idx < primariesCnt_) {
                isPrimary = true;
            }
            return ep;
        }
        idx++;
    }
    return nullptr;
}

void MultiPrimarySvcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
                                            boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));

    fpi::SvcUuid errdEpId;

    bool isPrimary = false;
    auto epReq = getEpReq_(header->msg_src_uuid, isPrimary);
    if (isComplete()) {
        /* Drop completed requests */
        GLOGWARN << logString() << " Already completed";
        return;
    } else if (!epReq) {
        /* Drop responses from uknown endpoint src ids */
        GLOGWARN << logString() << " Unknown EpId";
        return;
    } else if (epReq->isComplete()) {
        GLOGWARN << epReq->logString() << " Already completed";
        return;
    }
    epReq->completeReq(header->msg_code, header, payload);

    bool bSuccess = (header->msg_code == ERR_OK);
    /* Handle the error */
    if (header->msg_code != ERR_OK) {
        /* Notify actionable error to service manager */
        if (MODULEPROVIDER()->getSvcMgr()->isSvcActionableError(header->msg_code)) {
            MODULEPROVIDER()->getSvcMgr()->\
                handleSvcError(header->msg_src_uuid, header->msg_code);
        } else {
            /* Handle Application specific errors here */
            if (epAppStatusCb_) {
                bSuccess = epAppStatusCb_(header->msg_code, payload);
            }
        }
    }

    /* Update the endpoint ack counts */
    if (isPrimary) {
        primaryAckdCnt_++;
    }
    totalAckdCnt_++;
    
    /* Update who failed primary or optionals */
    if (!bSuccess) {
        if (isPrimary) {
            if (ERR_OK == response_) {
                response_ = header->msg_code;
            }
            failedPrimaries_.push_back(epReq);
            GLOGWARN << fds::logString(*header) << " response from primary failed - "
                     << "[rcvd acks: " << (int)primaryAckdCnt_ << " of " << (int)primariesCnt_
                     << " failed:" << failedPrimaries_.size() << "] - [total: "<< (int)totalAckdCnt_ << "]";
        } else {
            failedOptionals_.push_back(epReq);
            GLOGWARN << fds::logString(*header) << " response from optional failed : "  << failedOptionals_.size();
        }
    }

    GLOGDEBUG   << "[acks= total:" << (int)totalAckdCnt_ << "/" << epReqs_.size()
                << " primary:" << (int)primaryAckdCnt_ << "/" << (int)primariesCnt_
                << " fail-primary:" << failedPrimaries_.size()
                << " fail-optional:" << failedOptionals_.size()
                << " reqid:" << static_cast<SvcRequestId>(header->msg_src_id)
                << " cb:" << (respCb_!=NULL)
                << "]";

    /* Invoke response cb once all primaries responded */
    if (primaryAckdCnt_ == primariesCnt_ && respCb_) {
        GLOGDEBUG << "primacks rcvd for reqid:" << static_cast<SvcRequestId>(header->msg_src_id);
        respCb_(this, response_, responsePayload(0));
        respCb_ = 0;
    }

    /* Once all responses have come back complete the request and invoke failedOptionalsCb_
     * if required.
     */
    if (totalAckdCnt_ == epReqs_.size()) {
        GLOGDEBUG << "allacks rcvd reqid:" << static_cast<SvcRequestId>(header->msg_src_id);
        // FIXME(szmyd): Wed 01 Jul 2015 12:45:06 PM PDT
        // Shouldn't be using the last error we get...something else more intelligent?
        complete(response_);
        if (allRespondedCb_) {
            allRespondedCb_(this, response_, responsePayload(0));
        }
        if (response_ == ERR_OK) {
            MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->appsuccess.incr();
        } else {
            MODULEPROVIDER()->getSvcMgr()->getSvcRequestCntrs()->apperrors.incr();
        }
    }
}

}  // namespace fds
