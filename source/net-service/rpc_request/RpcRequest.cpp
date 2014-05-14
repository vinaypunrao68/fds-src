/* Copyright 2014 Formation Data Systems, Inc. */

#include <string>

#include <net/RpcRequest.h>
#include <net/AsyncRpcRequestTracker.h>

namespace fds {

AsyncRpcRequestIf::AsyncRpcRequestIf()
: AsyncRpcRequestIf(0)
{
}

AsyncRpcRequestIf::AsyncRpcRequestIf(const AsyncRpcRequestId &id)
: id_(id)
{
}

AsyncRpcRequestIf::~AsyncRpcRequestIf() {}


void AsyncRpcRequestIf::complete(const Error& error) {
    state_ = RPC_COMPLETE;
    error_ = error;
    gAsyncRpcTracker->removeFromTracking(id_);
}

/**
 *
 * @return
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

EPAsyncRpcRequest::EPAsyncRpcRequest()
    : EPAsyncRpcRequest(0, fpi::SvcUuid())
{
}

EPAsyncRpcRequest::EPAsyncRpcRequest(const AsyncRpcRequestId &id,
        const fpi::SvcUuid &uuid)
    : AsyncRpcRequestIf(id)
{
    svcId_ = uuid;
}

void EPAsyncRpcRequest::onSuccessCb(RpcRequestSuccessCb &cb) {
    successCb_ = cb;
}

void EPAsyncRpcRequest::onErrorCb(RpcRequestErrorCb &cb) {
    errorCb_ = cb;
}

void EPAsyncRpcRequest::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    Error e;
    // TODO(Rao): Parse out the error
    // parse out the payload
    if (e != ERR_OK) {
        if (errorCb_) {
            errorCb_(e, payload);
        } else {
            // TODO(Rao): Handle the error
        }
        return;
    }

    if (successCb_) {
        successCb_(payload);
    }
}
}  // namespace fds

