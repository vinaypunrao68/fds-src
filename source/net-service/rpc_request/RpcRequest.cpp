/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>

#include <concurrency/ThreadPool.h>
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
    GLOGDEBUG << " id: " << id_;

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


void AsyncRpcRequestIf::setRpcFunc(RpcFuncPtr rpc) {
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
 * Common invocation handler across all async requests
 * @param epId
 */
void AsyncRpcRequestIf::invokeCommon_(const fpi::SvcUuid &peerEpId)
{
    auto header = RpcRequestPool::newAsyncHeader(id_, myEpId_, peerEpId);
    rpc_->setHeader(header);

    GLOGDEBUG << logString(header);

    try {
        rpc_->invoke();
    } catch(...) {
        // TODO(Rao): Catch different exceptions
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_, peerEpId, myEpId_);
        respHdr->msg_code = ERR_RPC_INVOCATION;
        postError(respHdr);
    }
}

void AsyncRpcRequestIf::postError(boost::shared_ptr<fpi::AsyncHdr> header)
{
    fds_assert(header->msg_code != ERR_OK);

    boost::shared_ptr<std::string> payload;
    /* NetMgr::ep_mgr_singleton()->ep_mgr_thrpool()->schedule(
        std::bind(&BaseAsyncSvcHandler::asyncRespHandler, header, payload)); */
    new std::thread(std::bind(&BaseAsyncSvcHandler::asyncRespHandler, header, payload));
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
    GLOGDEBUG << " id: " << id_;
}

void EPAsyncRpcRequest::onSuccessCb(RpcRequestSuccessCb cb) {
    successCb_ = cb;
}

void EPAsyncRpcRequest::onErrorCb(RpcRequestErrorCb cb) {
    errorCb_ = cb;
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
       /* start the timer */
       if (timeoutMs_) {
           timer_.reset(new AsyncRpcTimer(id_, myEpId_, peerEpId_));
           bool ret = NetMgr::ep_mgr_singleton()->ep_get_timer()->\
                      schedule(timer_, std::chrono::milliseconds(timeoutMs_));
           fds_assert(ret == true);
       }
    } else {
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
    GLOGDEBUG << " id: " << id_;

    // bool invokeRpc = false;
    fpi::SvcUuid errdEpId;

    {
        fds_scoped_lock l(respLock_);
        if (isComplete()) {
            /* Request is already complete.  At this point we don't do anything on
             * the responses than just draining them out
             */
            return;
        }

        if (header->msg_code == ERR_OK) {
            if (successCb_) {
                successCb_(payload);
            }
            complete(ERR_OK);
        } else {
            GLOGWARN << logString(*header);

            errdEpId = header->msg_src_uuid;
            if (errorCb_) {
                errorCb_(header->msg_code, payload);
            }
            complete(ERR_RPC_FAILED);
        }
    }

    if (errdEpId.svc_uuid != 0 &&
        NetMgr::ep_mgr_singleton()->ep_actionable_error(header->msg_code)) {
        NetMgr::ep_mgr_singleton()->ep_handle_error(errdEpId, header->msg_code);
    }
}
}  // namespace fds

