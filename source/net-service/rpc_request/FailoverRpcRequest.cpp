/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <net/RpcRequest.h>
#include <net/AsyncRpcRequestTracker.h>
#include <net/RpcRequestPool.h>
#include <util/Log.h>
#include <fdsp_utils.h>

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
void FailoverRpcRequest::onSuccessCb(RpcRequestSuccessCb& cb)
{
    successCb_ = cb;
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 *
 * @param cb
 */
void FailoverRpcRequest::onErrorCb(RpcRequestErrorCb& cb)
{
    errorCb_ = cb;
}

/**
 * Not thread safe.  Client should invoke this function only once.
 */
void FailoverRpcRequest::invoke()
{
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    state_ = INVOCATION_PROGRESS;

    if (healthyEpExists) {
        invokeInternal_();
    } else {
        // TODO(Rao): post error to threadpool
    }
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

    uint32_t skipped_cnt = 0;
    for (; curEpIdx_ < eps_.size(); curEpIdx_++) {
        // TODO(Rao): Pass the right rpc version id
        auto ep = NetMgr::ep_mgr_singleton()->\
                    svc_lookup(eps_[curEpIdx_].epId, 0 , 0);
        eps_[curEpIdx_].status = ep->ep_get_status();
        if (eps_[curEpIdx_].status == ERR_OK) {
            return true;
        }
        skipped_cnt++;
    }

    GLOGDEBUG << "Req: " << id_ << " skipped cnt: " << skipped_cnt;
    return false;
}

/**
 *
 */
void FailoverRpcRequest::invokeInternal_()
{
    auto header = RpcRequestPool::newAsyncHeader(id_, eps_[curEpIdx_].epId);
    rpc_->setHeader(header);

    GLOGDEBUG << header;

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
            if (successCb_) {
                successCb_(payload);
            }
            complete(ERR_OK);
        } else {
            GLOGWARN << logString(*header);
            if (header->msg_src_uuid != eps_[curEpIdx_].epId) {
                /* Response isn't from the last endpoint we issued the request against.
                 * Don't do anything here
                 */
                return;
            }
            errdEpId = header->msg_src_uuid;
            bool healthyEpExists = moveToNextHealthyEndpoint_();
            if (!healthyEpExists) {
                if (errorCb_) {
                    errorCb_(header->msg_code, payload);
                }
                complete(ERR_RPC_FAILED);
            } else {
                if (failoverCb_) {
                    bool reqComplete = false;
                    failoverCb_(header->msg_code, payload, reqComplete);
                    if (reqComplete) {
                        // NOTE(Rao): We could invoke failure callback here
                        complete(ERR_RPC_USER_INTERRUPTED);
                        return;
                    }
                }
                invokeRpc = true;
            }
        }
    }

    if (invokeRpc) {
        invokeInternal_();
    }
    if (errdEpId.svc_uuid != 0) {
        // TODO(Rao): Notify an error
    }
}

} /* namespace fds */

