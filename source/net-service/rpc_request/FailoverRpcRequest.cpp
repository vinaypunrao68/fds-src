/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <net/net-service.h>
#include <net/RpcRequest.h>
#include <net/RpcRequestPool.h>
#include <util/Log.h>
#include <fdsp_utils.h>

namespace fds {

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
    GLOGDEBUG << " id: " << id_;
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
        GLOGDEBUG << "No healthy endpoints left";
        fds_assert(curEpIdx_ == epReqs_.size() - 1);
        auto respHdr = RpcRequestPool::newAsyncHeaderPtr(id_,
                                                     epReqs_[curEpIdx_]->peerEpId_,
                                                     myEpId_);
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
void FailoverRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    GLOGDEBUG << " id: " << id_;

    // bool invokeRpc = false;
    fpi::SvcUuid errdEpId;

    fds_scoped_lock l(respLock_);
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
        GLOGWARN << logString(*header);
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
        return;
    }

    /* Move to the next healhy endpoint and invoke */
    bool healthyEpExists = moveToNextHealthyEndpoint_();
    if (!healthyEpExists) {
        if (respCb_) {
            /* NOTE: We are last failure code in this case */
            respCb_(this, header->msg_code, payload);
        }
        complete(ERR_RPC_FAILED);
        return;
    }

    /* NOTE: We may consider moving this outside the lockscope */
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
                    endpoint_lookup(epReqs_[curEpIdx_]->peerEpId_);
        Error epStatus = ERR_OK;

        if (ep == nullptr) {
            epStatus = ERR_EP_NON_EXISTANT;
        } else {
            epStatus = ep->ep_get_status();
        }

        if (epStatus == ERR_OK) {
            epReqs_[curEpIdx_]->rpc_ = rpc_;
            epReqs_[curEpIdx_]->setTimeoutMs(timeoutMs_);
            return true;
        } else {
            /* When ep is not healthy invoke complete on associated ep request, except
             * the last ep request.  For the last unhealthy ep, complete is invoked in
             * handleResponse()
             */
            if (curEpIdx_ != epReqs_.size() - 1) {
                epReqs_[curEpIdx_]->complete(epStatus);
            }
            GLOGDEBUG << "Unhealthy endpoint: "
                << epReqs_[curEpIdx_]->peerEpId_.svc_uuid;
        }

        skipped_cnt++;
    }

    /* We've exhausted all the endpoints.  Decrement so that curEpIdx_ stays valid. Next
     * we will post an error to simulated an error from last endpoint.  This will get 
     * handled in handleResponse().  We do this so that user registered callbacks are
     * invoked.
     */
    fds_assert(curEpIdx_ == epReqs_.size());
    curEpIdx_--;

    GLOGDEBUG << "Req: " << id_ << " skipped cnt: " << skipped_cnt;
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

} /* namespace fds */

