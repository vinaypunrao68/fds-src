/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>
#include <vector>
#include <thread>

#include <concurrency/ThreadPool.h>
#include <net/net-service.h>
#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>

namespace fds {

SvcRequestTimer::SvcRequestTimer(const SvcRequestId &id,
                             const fpi::SvcUuid &myEpId,
                             const fpi::SvcUuid &peerEpId)
    : FdsTimerTask(*NetMgr::ep_mgr_singleton()->ep_mgr_singleton()->ep_get_timer())
{
    header_.reset(new fpi::AsyncHdr());
    *header_ = gSvcRequestPool->newSvcRequestHeader(id, peerEpId, myEpId);
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
    error_ = error;

    if (timer_) {
        NetMgr::ep_mgr_singleton()->ep_get_timer()->cancel(timer_);
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
 * NetMgr::ep_task_executor to schedule request specific invocatio work here.
 */
void SvcRequestIf::invoke()
{
#if 1
    static SynchronizedTaskExecutor<uint64_t>* taskExecutor =
        NetMgr::ep_mgr_singleton()->ep_get_task_executor();
    /* Execute on synchronized task exector so that invocation and response
     * handling is synchronized.
     */
    taskExecutor->schedule(id_, std::bind(&SvcRequestIf::invokeWork_, this));
#else
    invokeWork_();
#endif
}


/**
 * Common send handler across all async requests
 * Make sure access to this function is synchronized.  Sending and handling
 * of the response SHOULDN'T happen simultaneously.  Currently we ensure this by
 * using NetMgr::ep_task_executor 
 * @param epId
 */
int injerr_socket_close = 0;  // gdb set this value to trigger remote connection down error
using boost::lockfree::detail::unlikely;
void SvcRequestIf::sendPayload_(const fpi::SvcUuid &peerEpId)
{
    auto header = SvcRequestPool::newSvcRequestHeader(id_, myEpId_, peerEpId);
    header.msg_type_id = msgTypeId_;

    DBG(GLOGDEBUG << fds::logString(header));

    try {
        /* send the payload */
 do_again:
        auto ep = NetMgr::ep_mgr_singleton()->\
                  svc_get_handle<fpi::BaseAsyncSvcClient>(header.msg_dst_uuid, 0 , minor_version);
        if (!ep) {
            throw std::runtime_error("Null client");
        }
        if (unlikely(injerr_socket_close)) {
            int fd = ep->ep_sock->getSocketFD();
            close(fd);  // mimick remote gone TCP channe tear-down
            injerr_socket_close = 0;
            LOGNORMAL << " injected one socket connection losing error ";
            goto do_again;
        }
        ep->svc_rpc<fpi::BaseAsyncSvcClient>()->asyncReqt(header, *payloadBuf_);
        GLOGDEBUG << fds::logString(header) << " sent payload size: " << payloadBuf_->size();

       /* start the timer */
       if (timeoutMs_) {
           timer_.reset(new SvcRequestTimer(id_, myEpId_, peerEpId));
           bool ret = NetMgr::ep_mgr_singleton()->ep_get_timer()->\
                      schedule(timer_, std::chrono::milliseconds(timeoutMs_));
           fds_assert(ret == true);
       }
    } catch(std::exception &e) {
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code
            << " exception: " << e.what();
        gSvcRequestPool->postError(respHdr);
        fds_assert(!"Unknown exception");
    } catch(...) {
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, peerEpId, myEpId_);
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

/**
* @brief Worker function for doing the invocation work
* NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
*/
void EPSvcRequest::invokeWork_()
{
    fds_verify(error_ == ERR_OK);
    fds_verify(state_ == PRIOR_INVOCATION);

    // TODO(Rao): Determine endpoint is healthy or not
    bool epHealthy = true;
    state_ = INVOCATION_PROGRESS;
    if (epHealthy) {
       sendPayload_(peerEpId_);
    } else {
        GLOGERROR << logString() << " No healthy endpoints left";
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_, peerEpId_, myEpId_);
        respHdr->msg_code = ERR_SVC_REQUEST_INVOCATION;
        gSvcRequestPool->postError(respHdr);
    }
}

/**
 * @brief
 *
 * @param header
 * @param payload
 * NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
 */
void EPSvcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << logString());

    if (isComplete()) {
        /* Request is already complete.  At this point we don't do anything on
         * the responses than just draining them out
         */
        return;
    }

    /* Notify actionable error to endpoint manager */
    if (header->msg_code != ERR_OK &&
        NetMgr::ep_mgr_singleton()->ep_actionable_error(header->msg_code)) {
        NetMgr::ep_mgr_singleton()->ep_handle_error(
            header->msg_src_uuid, header->msg_code);
    }

    /* Invoke response callback */
    if (respCb_) {
        respCb_(this, header->msg_code, payload);
    }

    /* adjust counters */
    if (header->msg_code == ERR_OK) {
        gSvcRequestCntrs->appsuccess.incr();
    } else {
        gSvcRequestCntrs->apperrors.incr();
    }

    /* Complete the request */
    complete(header->msg_code);
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
 * NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
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
         * We will simulate as if the error is from last endpoint
         */
        auto respHdr = SvcRequestPool::newSvcRequestHeaderPtr(id_,
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
 * NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
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
        GLOGWARN << fds::logString(*header);
        /* Notify actionable error to endpoint manager */
        if (NetMgr::ep_mgr_singleton()->ep_actionable_error(header->msg_code)) {
            NetMgr::ep_mgr_singleton()->ep_handle_error(
                header->msg_src_uuid, header->msg_code);
        } else {
            /* Handle Application specific errors here */
            if (epAppStatusCb_) {
                bSuccess = epAppStatusCb_(header->msg_code, payload);
            }
        }
    }

    /* Handle the case where response from this endpoint is considered success */
    if (bSuccess) {
        if (respCb_) {
            respCb_(this, ERR_OK, payload);
        }
        complete(ERR_OK);
        gSvcRequestCntrs->appsuccess.incr();
        return;
    }

    /* Move to the next healhy endpoint and invoke */
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    if (!healthyEpExists) {
        if (respCb_) {
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        complete(ERR_SVC_REQUEST_FAILED);
        gSvcRequestCntrs->apperrors.incr();
        return;
    }

    /* NOTE: We may consider moving this outside the lockscope */
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
 * Moves to the next healyth endpoint in the sequence start from curEpIdx_
 * @return True if healthy endpoint is found in the sequence.  False otherwise
 */
bool FailoverSvcRequest::moveToNextHealthyEndpoint_()
{
    if (state_ == PRIOR_INVOCATION) {
        curEpIdx_ = 0;
    } else {
        curEpIdx_++;
    }

    for (; curEpIdx_ < epReqs_.size(); curEpIdx_++) {
        // TODO(Rao): Pass the right message version id
        auto ep = NetMgr::ep_mgr_singleton()->\
                    endpoint_lookup(epReqs_[curEpIdx_]->peerEpId_);
        Error epStatus = ERR_OK;

        // TODO(Rao): Uncomment once endpoint_lookup is implemented
        #if 0
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
    curEpIdx_--;

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
* NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
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
* NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
*/
void QuorumSvcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
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
        /* Notify actionable error to endpoint manager */
        if (NetMgr::ep_mgr_singleton()->ep_actionable_error(header->msg_code)) {
            NetMgr::ep_mgr_singleton()->ep_handle_error(
                header->msg_src_uuid, header->msg_code);
        } else {
            /* Handle Application specific errors here */
            if (epAppStatusCb_) {
                bSuccess = epAppStatusCb_(header->msg_code, payload);
            }
        }
    }

    /* Update the endpoint ack counts */
    if (bSuccess) {
        successAckd_++;
    } else {
        errorAckd_++;
    }

    /* Take action based on the ack counts */
    if (successAckd_ == quorumCnt_) {
        if (respCb_) {
            respCb_(this, ERR_OK, payload);
        }
        complete(ERR_OK);
        gSvcRequestCntrs->appsuccess.incr();
    } else if (errorAckd_ > (epReqs_.size() - quorumCnt_)) {
        if (respCb_) {
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        complete(ERR_SVC_REQUEST_FAILED);
        gSvcRequestCntrs->apperrors.incr();
        return;
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
