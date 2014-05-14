/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>

#include <net/RpcRequest.h>
#include <net/RpcRequestPool.h>
#include <util/Log.h>
#include <fdsp_utils.h>

namespace fds {

AsyncRpcRequestIf::AsyncRpcRequestIf()
    : AsyncRpcRequestIf(0)
{
}

AsyncRpcRequestIf::AsyncRpcRequestIf(const AsyncRpcRequestId &id)
    : id_(id),
      state_(PRIOR_INVOCATION),
      timeoutMs_(0)
{
}

AsyncRpcRequestIf::~AsyncRpcRequestIf() {}


/**
 * Marks the rpc request as complete and invokes the completion callback
 * @param error
 */
void AsyncRpcRequestIf::complete(const Error& error) {
    state_ = RPC_COMPLETE;
    error_ = error;
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
void AsyncRpcRequestIf::invokeCommon_(const fpi::SvcUuid &epId)
{
    auto header = RpcRequestPool::newAsyncHeader(id_, epId);
    rpc_->setHeader(header);

    GLOGDEBUG << logString(header);

    try {
        rpc_->invoke();
    } catch(...) {
        // TODO(Rao): Invoke endpoint error handling
        // Post an error to threadpool
    }
}

EPAsyncRpcRequest::EPAsyncRpcRequest()
    : EPAsyncRpcRequest(0, fpi::SvcUuid())
{
}

EPAsyncRpcRequest::EPAsyncRpcRequest(const AsyncRpcRequestId &id,
        const fpi::SvcUuid &uuid)
    : AsyncRpcRequestIf(id)
{
    epId_ = uuid;
}

void EPAsyncRpcRequest::onSuccessCb(RpcRequestSuccessCb &cb) {
    successCb_ = cb;
}

void EPAsyncRpcRequest::onErrorCb(RpcRequestErrorCb &cb) {
    errorCb_ = cb;
}

/**
 * Not thread safe.  Client should invoke this function only once.
 */
void EPAsyncRpcRequest::invoke()
{
    fds_verify(error_ == ERR_OK);

    state_ = INVOCATION_PROGRESS;
    bool epHealthy = true;
    // TODO(Rao): Determine endpoint is healthy or not

    if (epHealthy) {
       invokeCommon_(epId_);
    } else {
        // TODO(Rao): post error to threadpool
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
    bool invokeRpc = false;
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
        NetMgr::ep_mgr_singleton()->ep_handle_error(header->msg_code);
    }
}
}  // namespace fds

