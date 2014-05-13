/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <net/RpcRequest.h>
#include <AsyncRpcRequestTracker.h>

namespace fds {

/**
 *
 */
FailoverRpcRequest::FailoverRpcRequest()
: FailoverRpcRequest(0, std::vector<fpi::SvcUuid>())
{
}

/**
 *
 * @param id
 * @param uuid
 */
FailoverRpcRequest::FailoverRpcRequest(const AsyncRpcRequestId& id,
        const std::vector<fpi::SvcUuid>& uuid)
    : AsyncRpcRequestIf(0),
      curEpIdx_(0)
{
}

/**
 * For adding endpoint.
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param uuid
 */
void FailoverRpcRequest::addEndpoint(const fpi::SvcUuid& uuid) {
    eps_.push_back(AsyncRpcEpInfo(uuid));
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param cb
 */
void FailoverRpcRequest::onFailoverCb(RpcFailoverCb& cb) {
    failoverCb_ = cb;
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 *
 * @param cb
 */
void FailoverRpcRequest::onSuccessCb(RpcRequestSuccessCb& cb) {
    successCb_ = cb;
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 *
 * @param cb
 */
void FailoverRpcRequest::onErrorCb(RpcRequestErrorCb& cb) {
    errorCb_ = cb;
}

/**
 * Not thread safe.  Client should invoke this function only once.
 */
void FailoverRpcRequest::invoke()
{
    state_ = INVOCATION_PROGRESS;
    moveToNextHealthyEndpoint_();
    invokeInternal_();
}

/**
 *
 */
void FailoverRpcRequest::moveToNextHealthyEndpoint_() {
    // TODO(Rao): Impl
}

/**
 *
 */
void FailoverRpcRequest::invokeInternal_()
{
    fpi::AsyncHdr hdr;
    // TODO(Rao): populate the header.  Need a factory function to populate the header.
    rpc_->setHeader(hdr);
    try {
        rpc_->invoke();
    } catch(...) {
        // TODO(Rao): Invoke endpoint error handling
        // Post an error to threadpool
    }
}
/**
 * @brief
 *
 * @param header
 * @param payload
 */
// TODO(Rao): logging, invoking cb, error for each endpoint
void FailoverRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
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
            complete(ERR_OK);
            if (successCb_) {
                successCb_(payload);
            }
            gAsyncRpcTracker->removeFromTracking(this->getRequestId());
        } else {
            if (header->msg_src_uuid != eps_[curEpIdx_].epId) {
                /* Response isn't from the last endpoint we issued the request against.
                 * Don't do anything here
                 */
                return;
            }
            errdEpId = header->msg_src_uuid;
            // TODO(Rao): Find the next healthy replica
            curEpIdx_++;
            if (curEpIdx_ == eps_.size()) {
                complete(ERR_RPC_FAILED);
                if (errorCb_) {
                    errorCb_(header->msg_code, payload);
                }
                gAsyncRpcTracker->removeFromTracking(this->getRequestId());
            } else {
                if (failoverCb_) {
                    bool reqComplete = false;
                    failoverCb_(header->msg_code, payload, reqComplete);
                    if (reqComplete) {
                        // TODO(Rao): We could invoke failure callback here
                        complete(ERR_RPC_USER_INTERRUPTED);
                        return;
                    }
                }
                invokeRpc = true;
            }
        }
    }

    if (invokeRpc) {
        invoke();
    }
    if (errdEpId.svc_uuid != 0) {
        // TODO(Rao): Notify an error
    }
}

} /* namespace fds */

