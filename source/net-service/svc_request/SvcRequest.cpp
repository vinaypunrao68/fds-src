/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>
#include <vector>
#include <thread>

#include <concurrency/ThreadPool.h>
#include <fdsp/fds_service_types.h>
#include <net/SvcMgr.h>
#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <fds_module_provider.h>
#include <util/fiu_util.h>
#include <thrift/transport/TTransportUtils.h>  // For TException.

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
}

SvcRequestCounters::~SvcRequestCounters()
{
}

SvcRequestTimer::SvcRequestTimer(const SvcRequestId &id,
                                 const fpi::FDSPMsgTypeId &msgTypeId,
                                 const fpi::SvcUuid &myEpId,
                                 const fpi::SvcUuid &peerEpId)
    : FdsTimerTask(*(gModuleProvider->getTimer()))
{
    header_.reset(new fpi::AsyncHdr());
    *header_ = gSvcRequestPool->newSvcRequestHeader(id, msgTypeId, peerEpId, myEpId);
    header_->msg_code = ERR_SVC_REQUEST_TIMEOUT;
}

/**
* @brief Sends a timeout error BaseAsyncSvcHandler.  Note this call is executed
* on a threadpool
*/
void SvcRequestTimer::runTimerTask()
{
    GLOGWARN << "Timeout: " << fds::logString(*header_);
    gSvcRequestPool->postError(header_);
}

SvcRequestIf::SvcRequestIf()
    : SvcRequestIf(0, fpi::SvcUuid())
{
}

SvcRequestIf::SvcRequestIf(const SvcRequestId &id,
                                     const fpi::SvcUuid &myEpId)
    : id_(id),
      myEpId_(myEpId),
      state_(PRIOR_INVOCATION),
      timeoutMs_(0),
      minor_version(0)
{
}

SvcRequestIf::~SvcRequestIf()
{
}


void SvcRequestIf::setPayloadBuf(const fpi::FDSPMsgTypeId &msgTypeId,
                                      boost::shared_ptr<std::string> &buf)
{
    fds_assert(!payloadBuf_);
    msgTypeId_ = msgTypeId;
    payloadBuf_ = buf;
}

/**
 * Marks the svc request as complete and invokes the completion callback
 * @param error
 */
void SvcRequestIf::complete(const Error& error) {
    DBG(GLOGDEBUG << logString() << error);

    fds_assert(state_ != SVC_REQUEST_COMPLETE);
    state_ = SVC_REQUEST_COMPLETE;

    if (timer_) {
        gModuleProvider->getTimer()->cancel(timer_);
        timer_.reset();
    }

    if (completionCb_) {
        completionCb_(id_);
    }
}

/**
 *
 * @return True if svc request is in complete state
 */
bool SvcRequestIf::isComplete()
{
    return state_ == SVC_REQUEST_COMPLETE;
}


void SvcRequestIf::setTimeoutMs(const uint32_t &timeout_ms) {
    timeoutMs_ = timeout_ms;
}

uint32_t SvcRequestIf::getTimeout() {
    return timeoutMs_;
}

SvcRequestId SvcRequestIf::getRequestId() {
    return id_;
}

void SvcRequestIf::setRequestId(const SvcRequestId &id) {
    id_ = id;
}

void SvcRequestIf::setCompletionCb(SvcRequestCompletionCb &completionCb)
{
    completionCb_ = completionCb;
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
    fiu_do_on("svc.use.lftp",
              gSvcRequestPool->getSvcWorkerThreadpool()->\
              scheduleWithAffinity(id_, &SvcRequestIf::invoke2, this); return;);

    static SynchronizedTaskExecutor<uint64_t>* taskExecutor = 
        gModuleProvider->getSvcMgr()->getTaskExecutor();
    /* Execute on synchronized task exector so that invocation and response
     * handling is synchronized.
     */
    taskExecutor->schedule(id_, std::bind(&SvcRequestIf::invokeWork_, this));
}


/**
 * Common send handler across all async requests
 * Make sure access to this function is synchronized.  Sending and handling
 * of the response SHOULDN'T happen simultaneously.  Currently we ensure this by
 * using SvcMgr::taskExecutor_
 * @param epId
 */
void SvcRequestIf::sendPayload_(const fpi::SvcUuid &peerEpId)
{
    auto header = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, myEpId_, peerEpId);
    header->msg_type_id = msgTypeId_;

    DBG(GLOGDEBUG << fds::logString(*header));

    try {
        /* Fault injection */
        fiu_do_on("svc.fail.sendpayload_before",
                  throw util::FiuException("svc.fail.sendpayload_before"));
        /* send the payload */
        gModuleProvider->getSvcMgr()->sendAsyncSvcMessage(header, payloadBuf_);

       /* start the timer */
       if (timeoutMs_) {
           timer_.reset(new SvcRequestTimer(id_, msgTypeId_, myEpId_, peerEpId));
           bool ret = gModuleProvider->getTimer()->\
                      schedule(timer_, std::chrono::milliseconds(timeoutMs_));
           fds_assert(ret == true);
       }
    } catch(util::FiuException &e) {
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code
            << " exception: " << e.what();
        gSvcRequestPool->postError(respHdr);
    } catch(apache::thrift::TException &tx) {
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGWARN << logString() << " Warning: " << respHdr->msg_code
            << " exception: " << tx.what();
        gSvcRequestPool->postError(respHdr);
    } catch(std::runtime_error &e) {
        GLOGERROR << logString() << " No healthy endpoints left";
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code
            << " exception: " << e.what();
        gSvcRequestPool->postError(respHdr);
    } catch(std::exception &e) {
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code
            << " exception: " << e.what();
        gSvcRequestPool->postError(respHdr);
        fds_assert(!"Unknown exception");
    } catch(...) {
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code;
        gSvcRequestPool->postError(respHdr);
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


void SvcRequestIf::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
                                  boost::shared_ptr<std::string>& payload)
{
    this->handleResponseImpl(header, payload);

    // Don't cause a feedback loop if Om is down.
    if (fpi::CtrlSvcEventTypeId == header->msg_type_id) {
        return;
    }
    switch (header->msg_code) {
        case ERR_DISK_WRITE_FAILED:
        case ERR_DISK_READ_FAILED:
        case ERR_SVC_REQUEST_TIMEOUT:
            {
            // The who, what and result of the event
            fpi::CtrlSvcEventPtr pkt(new fpi::CtrlSvcEvent());
            pkt->evt_src_svc_uuid = header->msg_src_uuid;
            pkt->evt_code = header->msg_code;
            pkt->evt_msg_type_id  = header->msg_type_id;

            auto req = gSvcRequestPool->newEPSvcRequest(gl_OmUuid.toSvcUuid());
            req->setPayload(FDSP_MSG_TYPEID(fpi::CtrlSvcEvent), pkt);
            req->invoke();
            }
            break;
        default:
            break;
    }
}

EPSvcRequest::EPSvcRequest()
    : EPSvcRequest(0, fpi::SvcUuid(), fpi::SvcUuid())
{
}

EPSvcRequest::EPSvcRequest(const SvcRequestId &id,
                                     const fpi::SvcUuid &myEpId,
                                     const fpi::SvcUuid &peerEpId)
    : SvcRequestIf(id, myEpId)
{
    peerEpId_ = peerEpId;
}

EPSvcRequest::~EPSvcRequest()
{
}

void EPSvcRequest::invoke2()
{
}

void FailoverSvcRequest::invoke2()
{
}

void QuorumSvcRequest::invoke2()
{
}

/**
* @brief Worker function for doing the invocation work
* NOTE this function is executed on SvcMgr::taskExecutor_ for synchronization
*/
void EPSvcRequest::invokeWork_()
{
    fds_verify(state_ == PRIOR_INVOCATION);

    // TODO(Rao): Determine endpoint is healthy or not
    bool epHealthy = true;
    state_ = INVOCATION_PROGRESS;
    if (epHealthy) {
        SVCPERF(util::StopWatch sw; sw.start());
        sendPayload_(peerEpId_);
        SVCPERF(gSvcRequestCntrs->sendPayloadLat.update(sw.getElapsedNanos()));
    } else {
        GLOGERROR << logString() << " No healthy endpoints left";
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, msgTypeId_, peerEpId_, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        gSvcRequestPool->postError(respHdr);
    }
}

/**
 * @brief
 *
 * @param header
 * @param payload
 * NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
 */
void EPSvcRequest::handleResponseImpl(boost::shared_ptr<fpi::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << logString());

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
        gModuleProvider->getSvcMgr()->isSvcActionableError(header->msg_code)) {
        gModuleProvider->getSvcMgr()->handleSvcError(header->msg_src_uuid, header->msg_code);
    }

    /* Invoke response callback */
    if (respCb_) {
        SVCPERF(ts.rspHndlrTs = util::getTimeStampNanos());
        respCb_(this, header->msg_code, payload);
    }

    /* adjust counters */
    if (header->msg_code == ERR_OK) {
        gSvcRequestCntrs->appsuccess.incr();
    } else {
        gSvcRequestCntrs->apperrors.incr();
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
    : MultiEpSvcRequest(0, fpi::SvcUuid(), std::vector<fpi::SvcUuid>())
{
}

/**
* @brief
*
* @param id
* @param myEpId
* @param peerEpIds
*/
MultiEpSvcRequest::MultiEpSvcRequest(const SvcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds)
    : SvcRequestIf(id, myEpId)
{
    for (auto uuid : peerEpIds) {
        addEndpoint(uuid);
    }
}

/**
 * For adding endpoint.
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param uuid
 */
void MultiEpSvcRequest::addEndpoint(const fpi::SvcUuid& peerEpId)
{
    epReqs_.push_back(EPSvcRequestPtr(
            new EPSvcRequest(id_, myEpId_, peerEpId)));
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
EPSvcRequestPtr MultiEpSvcRequest::getEpReq_(fpi::SvcUuid &peerEpId)
{
    for (auto ep : epReqs_) {
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
: FailoverSvcRequest(0, fpi::SvcUuid(), std::vector<fpi::SvcUuid>())
{
}


/**
* @brief
*
* @param id
* @param myEpId
* @param peerEpIds
*/
FailoverSvcRequest::FailoverSvcRequest(const SvcRequestId& id,
                                       const fpi::SvcUuid &myEpId,
                                       const std::vector<fpi::SvcUuid>& peerEpIds)
    : MultiEpSvcRequest(id, myEpId, peerEpIds),
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
FailoverSvcRequest::FailoverSvcRequest(const SvcRequestId& id,
                                       const fpi::SvcUuid &myEpId,
                                       const EpIdProviderPtr epProvider)
    : FailoverSvcRequest(id, myEpId, epProvider->getEps())
{
}

FailoverSvcRequest::~FailoverSvcRequest()
{
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
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_,
                                                              msgTypeId_,
                                                              epReqs_[curEpIdx_]->peerEpId_,
                                                              myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        gSvcRequestPool->postError(respHdr);
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
void FailoverSvcRequest::handleResponseImpl(boost::shared_ptr<fpi::AsyncHdr>& header,
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
        if (gModuleProvider->getSvcMgr()->isSvcActionableError(header->msg_code)) {
            gModuleProvider->getSvcMgr()->\
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
            respCb_(this, header->msg_code, payload);
        }
        gSvcRequestCntrs->appsuccess.incr();
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
        gSvcRequestCntrs->apperrors.incr();
        return;
    }

    fiu_do_on("svc.use.lftp", epReqs_[curEpIdx_]->invoke2(); return;);
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
             * handleResponseImpl()
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
     * handled in handleResponseImpl().  We do this so that user registered callbacks are
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
    : QuorumSvcRequest(0, fpi::SvcUuid(), std::vector<fpi::SvcUuid>())
{
}

/**
* @brief
*
* @param id
* @param myEpId
* @param peerEpIds
*/
QuorumSvcRequest::QuorumSvcRequest(const SvcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   const std::vector<fpi::SvcUuid>& peerEpIds)
    : MultiEpSvcRequest(id, myEpId, peerEpIds)
{
    successAckd_ = 0;
    errorAckd_ = 0;
    quorumCnt_ = peerEpIds.size();
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
QuorumSvcRequest::QuorumSvcRequest(const SvcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   const EpIdProviderPtr epProvider)
: QuorumSvcRequest(id, myEpId, epProvider->getEps())
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

/**
* @brief Inovcation work function
* NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
*/
void QuorumSvcRequest::invokeWork_()
{
    for (auto ep : epReqs_) {
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
void QuorumSvcRequest::handleResponseImpl(boost::shared_ptr<fpi::AsyncHdr>& header,
                                          boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << logString());

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

    epReq->complete(header->msg_code);

    bool bSuccess = (header->msg_code == ERR_OK);
    /* Handle the error */
    if (header->msg_code != ERR_OK) {
        GLOGWARN << fds::logString(*header);
        /* Notify actionable error to service manager */
        if (gModuleProvider->getSvcMgr()->isSvcActionableError(header->msg_code)) {
            gModuleProvider->getSvcMgr()->\
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
        ++errorAckd_;
    }

    /* Take action based on the ack counts */
    if (successAckd_ == quorumCnt_) {
        complete(ERR_OK);
        if (respCb_) {
            SVCPERF(ts.rspHndlrTs = util::getTimeStampNanos());
            respCb_(this, ERR_OK, payload);
        }
        gSvcRequestCntrs->appsuccess.incr();
    } else if (errorAckd_ > (epReqs_.size() - quorumCnt_)) {
        complete(ERR_SVC_REQUEST_FAILED);
        if (respCb_) {
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        gSvcRequestCntrs->apperrors.incr();
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

}  // namespace fds
