/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <net/RpcRequest.h>
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
        const std::vector<fpi::SvcUuid>& svcUuids)
    : AsyncRpcRequestIf(0),
      curEpIdx_(0)
{
    for (auto uuid : svcUuids) {
        addEndpoint(uuid);
    }
}

/**
 * For adding endpoint.
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param uuid
 */
void FailoverRpcRequest::addEndpoint(const fpi::SvcUuid& uuid)
{
    epReqs_.push_back(EPAsyncRpcRequestPtr(new EPAsyncRpcRequest(id_, uuid)));
}

/**
 * NOTE: Only invoke during initialization.  Donot invoke once
 * the request is in progress
 * @param cb
 */
void FailoverRpcRequest::onFailoverCb(RpcRequestFailoverCb& cb)
{
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
        epReqs_[curEpIdx_]->invoke();
    } else {
        // TODO(Rao): post error to threadpool.  Simulate the error as it's coming from
        // last endpoint
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

    fds_scoped_lock l(respLock_);
    if (isComplete()) {
        /* Request is already complete.  At this point we don't do anything on
         * the responses than just draining them out
         */
        return;
    }

    if (header->msg_src_uuid != epReqs_[curEpIdx_]->epId_) {
        /* Response isn't from the last endpoint we issued the request against.
         * Don't do anything here
         */
        // TODO(Rao): We may need special handling for success case here
        return;
    }

    /* Forward the response the current endpoint request */
    epReqs_[curEpIdx_]->handleResponse(header, payload);
}

/**
 * Call back from the current endpoint request with a succesful reponse
 * @param payload
 */
void FailoverRpcRequest::epReqSuccessCb_(boost::shared_ptr<std::string> payload)
{
    if (successCb_) {
        successCb_(payload);
    }
    complete(ERR_OK);
}

/**
 * Call back from the current endpoint request on an error
 * @param payload
 */
void FailoverRpcRequest::epReqErrorCb_(const Error& e,
        boost::shared_ptr<std::string> payload)
{
    if (failoverCb_) {
        bool reqComplete = false;
        failoverCb_(e, payload, reqComplete);
        if (reqComplete) {
            // NOTE: errorCb_ isn't invoked here
            complete(ERR_RPC_USER_INTERRUPTED);
            return;
        }
    }

    /* Move to the next healhy endpoint and invoke */
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    if (!healthyEpExists) {
        if (errorCb_) {
            errorCb_(e, payload);
        }
        complete(ERR_RPC_FAILED);
        return;
    }

    epReqs_[curEpIdx_]->invoke();
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
    for (; curEpIdx_ < epReqs_.size(); curEpIdx_++) {
        // TODO(Rao): Pass the right rpc version id
        auto ep = NetMgr::ep_mgr_singleton()->\
                    endpoint_lookup(epReqs_[curEpIdx_]->epId_);

        if (ep == nullptr) {
            epReqs_[curEpIdx_]->error_ = ERR_EP_NON_EXISTANT;
        } else {
            epReqs_[curEpIdx_]->error_ = ep->ep_get_status();
        }

        if (epReqs_[curEpIdx_]->error_ == ERR_OK) {
            epReqs_[curEpIdx_]->rpc_ = rpc_;
            epReqs_[curEpIdx_]->successCb_ =
                    std::bind(&FailoverRpcRequest::epReqSuccessCb_,
                            this, std::placeholders::_1);
            epReqs_[curEpIdx_]->errorCb_ =
                    std::bind(&FailoverRpcRequest::epReqErrorCb_,
                            this, std::placeholders::_1, std::placeholders::_2);
            return true;
        }
        skipped_cnt++;
    }

    GLOGDEBUG << "Req: " << id_ << " skipped cnt: " << skipped_cnt;
    return false;
}

#if 0
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
        invokeCommon_(eps_[curEpIdx_].epId);
    }

    if (errdEpId.svc_uuid != 0 &&
        NetMgr::ep_mgr_singleton()->ep_actionable_error(header->msg_code)) {
        NetMgr::ep_mgr_singleton()->ep_handle_error(header->msg_code);
    }
}
#endif

} /* namespace fds */

