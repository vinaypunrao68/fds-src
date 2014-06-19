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

void AsyncRpcRequestIf::onSuccessCb(RpcRequestSuccessCb cb) {
    successCb_ = cb;
}

void AsyncRpcRequestIf::onErrorCb(RpcRequestErrorCb cb) {
    errorCb_ = cb;
}

void AsyncRpcRequestIf::setCompletionCb(RpcRequestCompletionCb &completionCb)
{
    completionCb_ = completionCb;
}
/**
 * Common invocation handler across all async requests
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
    } catch(...) {
        // TODO(Rao): Catch different exceptions
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_RPC_INVOCATION;
        GLOGERROR << logString() << " Error: " << respHdr->msg_code;
        postError(respHdr);
    }
}

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
 * Not thread safe.  Client should invoke this function only once.
 */
void EPAsyncRpcRequest::invoke()
{
    fds_verify(error_ == ERR_OK);

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
 */
// TODO(Rao): logging, invoking cb, error for each endpoint
void EPAsyncRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << logString());

    fds_scoped_lock l(respLock_);
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
* @brief 
*/
void QuorumRpcRequest::invoke()
{
    for (auto ep : epReqs_) {
        ep->setRpcFunc(rpc_);
        ep->setTimeoutMs(timeoutMs_);
        ep->invoke();
    }
}

/**
* @brief 
*
* @param header
* @param payload
*/
void QuorumRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
                                      boost::shared_ptr<std::string>& payload)
{
    DBG(GLOGDEBUG << logString());

    fpi::SvcUuid errdEpId;

    // fds_scoped_lock l(respLock_);
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
        if (successCb_) {
            successCb_(payload);
        }
        complete(ERR_OK);
    } else if (errorAckd_ > (epReqs_.size() - quorumCnt_)) {
        if (errorCb_) {
            errorCb_(header->msg_code, payload);
        }
        complete(ERR_RPC_FAILED);
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
