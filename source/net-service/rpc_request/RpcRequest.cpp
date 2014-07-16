/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>
#include <vector>

#include <concurrency/ThreadPool.h>
#include <net/net-service.h>
#include <net/RpcRequest.h>
#include <net/RpcRequestPool.h>
#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <thread>

namespace fds {

AsyncRpcTimer::AsyncRpcTimer(const AsyncRpcRequestId &id,
                             const fpi::SvcUuid &myEpId,
                             const fpi::SvcUuid &peerEpId)
    : FdsTimerTask(*NetMgr::ep_mgr_singleton()->ep_mgr_singleton()->ep_get_timer())
{
    header_.reset(new fpi::AsyncHdr());
    *header_ = gRpcRequestPool->newAsyncHeader(id, peerEpId, myEpId);
    header_->msg_code = ERR_RPC_TIMEOUT;
}

/**
* @brief Sends a timeout error BaseAsyncSvcHandler.  Note this call is executed
* on a threadpool
*/
void AsyncRpcTimer::runTimerTask()
{
    GLOGWARN << "Timeout: " << fds::logString(*header_);
    AsyncRpcRequestIf::postError(header_);
}

AsyncRpcRequestIf::AsyncRpcRequestIf()
    : AsyncRpcRequestIf(0, fpi::SvcUuid())
{
}

AsyncRpcRequestIf::AsyncRpcRequestIf(const AsyncRpcRequestId &id,
                                     const fpi::SvcUuid &myEpId)
    : id_(id),
      myEpId_(myEpId),
      state_(PRIOR_INVOCATION),
      timeoutMs_(0)
{
}

AsyncRpcRequestIf::~AsyncRpcRequestIf()
{
}


/**
 * Marks the rpc request as complete and invokes the completion callback
 * @param error
 */
void AsyncRpcRequestIf::complete(const Error& error) {
    DBG(GLOGDEBUG << logString() << error);

    fds_assert(state_ != RPC_COMPLETE);
    state_ = RPC_COMPLETE;
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
 * @return True if rpc request is in complete state
 */
bool AsyncRpcRequestIf::isComplete()
{
    return state_ == RPC_COMPLETE;
}


void AsyncRpcRequestIf::setRpcFunc(RpcFunc rpc) {
    rpc_ = rpc;
}

void AsyncRpcRequestIf::setTimeoutMs(const uint32_t &timeout_ms) {
    timeoutMs_ = timeout_ms;
}

uint32_t AsyncRpcRequestIf::getTimeout() {
    return timeoutMs_;
}

AsyncRpcRequestId AsyncRpcRequestIf::getRequestId() {
    return id_;
}

void AsyncRpcRequestIf::setRequestId(const AsyncRpcRequestId &id) {
    id_ = id;
}

void AsyncRpcRequestIf::setCompletionCb(RpcRequestCompletionCb &completionCb)
{
    completionCb_ = completionCb;
}

/**
 * Common invocation method for all async requess.
 * In order to synchronize invocation and response handling  we use
 * NetMgr::ep_task_executor to schedule request specific invocatio work here.
 */
void AsyncRpcRequestIf::invoke()
{
    static SynchronizedTaskExecutor<uint64_t>* taskExecutor =
        NetMgr::ep_mgr_singleton()->ep_get_task_executor();
    /* Execute on synchronized task exector so that invocation and response
     * handling is synchronized.
     */
    taskExecutor->schedule(id_, std::bind(&AsyncRpcRequestIf::invokeWork_, this));
}


/**
 * Common invocation handler across all async requests
 * Make sure access to this function is synchronized.  Invocation and handling
 * of the response SHOULDN'T happen simultaneously.  Currently we ensure this by
 * using NetMgr::ep_task_executor 
 * @param epId
 */
void AsyncRpcRequestIf::invokeCommon_(const fpi::SvcUuid &peerEpId)
{
    auto header = RpcRequestPool::newAsyncHeader(id_, myEpId_, peerEpId);

    DBG(GLOGDEBUG << fds::logString(header));

    try {
        /* Invoke rpc */
        rpc_(header);
       /* start the timer */
       if (timeoutMs_) {
           timer_.reset(new AsyncRpcTimer(id_, myEpId_, peerEpId));
           bool ret = NetMgr::ep_mgr_singleton()->ep_get_timer()->\
                      schedule(timer_, std::chrono::milliseconds(timeoutMs_));
           fds_assert(ret == true);
       }
    } catch(std::exception &e) {
        fds_assert(!"Unknown exception");
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_RPC_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code
            << " exception: " << e.what();
        postError(respHdr);
    } catch(...) {
        fds_assert(!"Unknown exception");
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_RPC_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code;
        postError(respHdr);
    }
}


/**
* @brief Common method for posting errors typically encountered in invocation code paths
* for all async rpc requests.  Here the we simulate as if the error is coming from 
* the endpoint.  Error is posted to a threadpool.
*
* @param header
*/
void AsyncRpcRequestIf::postError(boost::shared_ptr<fpi::AsyncHdr> header)
{
    fds_assert(header->msg_code != ERR_OK);

    /* Counter adjustment */
    switch (header->msg_code) {
    case ERR_RPC_INVOCATION:
        gAsyncRpcCntrs->invokeerrors.incr();
        break;
    case ERR_RPC_TIMEOUT:
        gAsyncRpcCntrs->timedout.incr();
        break;
    }

    /* Simulate an error for remote endpoint */
    boost::shared_ptr<std::string> payload;
    header->msg_type_id = fpi::NullMsgTypeId;
    NetMgr::ep_mgr_singleton()->ep_mgr_thrpool()->schedule(
        &BaseAsyncSvcHandler::asyncRespHandler, header, payload);
}


std::stringstream& AsyncRpcRequestIf::logRpcReqCommon_(std::stringstream &oss,
                                                       const std::string &type)
{
    oss << " " << type << " Req Id: " << id_ << " From: " << myEpId_.svc_uuid;
    return oss;
}

EPAsyncRpcRequest::EPAsyncRpcRequest()
    : EPAsyncRpcRequest(0, fpi::SvcUuid(), fpi::SvcUuid())
{
}

EPAsyncRpcRequest::EPAsyncRpcRequest(const AsyncRpcRequestId &id,
                                     const fpi::SvcUuid &myEpId,
                                     const fpi::SvcUuid &peerEpId)
    : AsyncRpcRequestIf(id, myEpId)
{
    peerEpId_ = peerEpId;
}

EPAsyncRpcRequest::~EPAsyncRpcRequest()
{
}

/**
* @brief Worker function for doing the invocation work
* NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
*/
void EPAsyncRpcRequest::invokeWork_()
{
    fds_verify(error_ == ERR_OK);
    fds_verify(state_ == PRIOR_INVOCATION);

    // TODO(Rao): Determine endpoint is healthy or not
    bool epHealthy = true;
    state_ = INVOCATION_PROGRESS;
    if (epHealthy) {
       invokeCommon_(peerEpId_);
    } else {
        GLOGERROR << logString() << " No healthy endpoints left";
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_, peerEpId_, myEpId_);
        respHdr->msg_code = ERR_RPC_INVOCATION;
        postError(respHdr);
    }
}

/**
 * @brief
 *
 * @param header
 * @param payload
 * NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
 */
// TODO(Rao): logging, invoking cb, error for each endpoint
void EPAsyncRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
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
    if (respCb_) {
        respCb_(this, header->msg_code, payload);
    }
    complete(ERR_OK);
}

/**
* @brief 
*
* @return 
*/
std::string EPAsyncRpcRequest::logString()
{
    std::stringstream oss;
    logRpcReqCommon_(oss, "EPAsyncRpcRequest") << " To: " << peerEpId_.svc_uuid;
    return oss.str();
}

/**
* @brief 
*
* @param cb
*/
void EPAsyncRpcRequest::onResponseCb(EPAsyncRpcRespCb cb)
{
    respCb_ = cb;
}

/**
* @brief 
*
* @return 
*/
fpi::SvcUuid EPAsyncRpcRequest::getPeerEpId() const
{
    return peerEpId_;
}

/**
* @brief 
*/
MultiEpAsyncRpcRequest::MultiEpAsyncRpcRequest()
    : MultiEpAsyncRpcRequest(0, fpi::SvcUuid(), std::vector<fpi::SvcUuid>())
{
}

/**
* @brief 
*
* @param id
* @param myEpId
* @param peerEpIds
*/
MultiEpAsyncRpcRequest::MultiEpAsyncRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds)
    : AsyncRpcRequestIf(id, myEpId)
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
void MultiEpAsyncRpcRequest::addEndpoint(const fpi::SvcUuid& peerEpId)
{
    epReqs_.push_back(EPAsyncRpcRequestPtr(
            new EPAsyncRpcRequest(id_, myEpId_, peerEpId)));
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param cb
 */
void MultiEpAsyncRpcRequest::onEPAppStatusCb(EPAppStatusCb cb)
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
EPAsyncRpcRequestPtr MultiEpAsyncRpcRequest::getEpReq_(fpi::SvcUuid &peerEpId)
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
FailoverRpcRequest::FailoverRpcRequest()
: FailoverRpcRequest(0, fpi::SvcUuid(), std::vector<fpi::SvcUuid>())
{
}


/**
* @brief 
*
* @param id
* @param myEpId
* @param peerEpIds
*/
FailoverRpcRequest::FailoverRpcRequest(const AsyncRpcRequestId& id,
                                       const fpi::SvcUuid &myEpId,
                                       const std::vector<fpi::SvcUuid>& peerEpIds)
    : MultiEpAsyncRpcRequest(id, myEpId, peerEpIds),
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
FailoverRpcRequest::FailoverRpcRequest(const AsyncRpcRequestId& id,
                                       const fpi::SvcUuid &myEpId,
                                       const EpIdProviderPtr epProvider)
    : FailoverRpcRequest(id, myEpId, epProvider->getEps())
{
}

FailoverRpcRequest::~FailoverRpcRequest()
{
}



/**
 * Invocation work function
 * NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
 */
void FailoverRpcRequest::invokeWork_()
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
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_,
                                                     epReqs_[curEpIdx_]->peerEpId_,
                                                     myEpId_);
        respHdr->msg_code = ERR_RPC_INVOCATION;
        postError(respHdr);
    }
}

/**
 * @brief
 * Handles asynchronous response.  Response source can be from
 * 1. Network
 * 2. Local where we simulate an error.  This can happen when invocation fails
 * immediately
 * NOTE: We make the assumption that response always comes on a different thread
 * than rpc invocation thread.
 *
 * Success case handling: Invoke the registered response cb
 *
 * Error case handling: Invoke application error handler (Only for application errors)
 * if one is registered.  Move on to the next healthy endpont and invoke rpc. If
 * we don't have healthy endpoints, then invoke response cb with error.
 *
 * @param header
 * @param payload
 * NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
 */
// TODO(Rao): logging, invoking cb, error for each endpoint
void FailoverRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
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
        gAsyncRpcCntrs->appsuccess.incr();
        return;
    }

    /* Move to the next healhy endpoint and invoke */
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    if (!healthyEpExists) {
        if (respCb_) {
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        complete(ERR_RPC_FAILED);
        gAsyncRpcCntrs->apperrors.incr();
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
std::string FailoverRpcRequest::logString()
{
    std::stringstream oss;
    logRpcReqCommon_(oss, "FailoverRpcRequest")
        << " curEpIdx: " << static_cast<uint32_t>(curEpIdx_);
    return oss.str();
}

/**
 * Moves to the next healyth endpoint in the sequence start from curEpIdx_
 * @return True if healthy endpoint is found in the sequence.  False otherwise
 */
bool FailoverRpcRequest::moveToNextHealthyEndpoint_()
{
    if (state_ == PRIOR_INVOCATION) {
        curEpIdx_ = 0;
    } else {
        curEpIdx_++;
    }

    for (; curEpIdx_ < epReqs_.size(); curEpIdx_++) {
        // TODO(Rao): Pass the right rpc version id
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
            epReqs_[curEpIdx_]->rpc_ = rpc_;
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
void FailoverRpcRequest::onResponseCb(FailoverRpcRespCb cb)
{
    respCb_ = cb;
}

/**
* @brief 
*/
QuorumRpcRequest::QuorumRpcRequest()
    : QuorumRpcRequest(0, fpi::SvcUuid(), std::vector<fpi::SvcUuid>())
{
}

/**
* @brief 
*
* @param id
* @param myEpId
* @param peerEpIds
*/
QuorumRpcRequest::QuorumRpcRequest(const AsyncRpcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   const std::vector<fpi::SvcUuid>& peerEpIds)
    : MultiEpAsyncRpcRequest(id, myEpId, peerEpIds)
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
QuorumRpcRequest::QuorumRpcRequest(const AsyncRpcRequestId& id,
                                   const fpi::SvcUuid &myEpId,
                                   const EpIdProviderPtr epProvider)
: QuorumRpcRequest(id, myEpId, epProvider->getEps())
{
}
/**
* @brief 
*/
QuorumRpcRequest::~QuorumRpcRequest()
{
}

/**
* @brief 
*
* @param cnt
*/
void QuorumRpcRequest::setQuorumCnt(const uint32_t cnt)
{
    quorumCnt_ = cnt;
}

/**
* @brief Inovcation work function
* NOTE this function is exectued on NetMgr::ep_task_executor for synchronization
*/
void QuorumRpcRequest::invokeWork_()
{
    for (auto ep : epReqs_) {
        ep->setRpcFunc(rpc_);
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
void QuorumRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
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
        gAsyncRpcCntrs->appsuccess.incr();
    } else if (errorAckd_ > (epReqs_.size() - quorumCnt_)) {
        if (respCb_) {
            /* NOTE: We are using last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        complete(ERR_RPC_FAILED);
        gAsyncRpcCntrs->apperrors.incr();
        return;
    }
}

/**
* @brief 
*
* @return 
*/
std::string QuorumRpcRequest::logString()
{
    std::stringstream oss;
    logRpcReqCommon_(oss, "QuorumRpcRequest");
    return oss.str();
}

void QuorumRpcRequest::onResponseCb(QuorumRpcRespCb cb)
{
    respCb_ = cb;
}

}  // namespace fds
