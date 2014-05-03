/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <RpcRequest.h>
namespace fds {

/**
 *
 * @param id
 */
void AsyncRpcRequestIf::setRequestId(const AsyncRpcRequestId &id)
{
    id_ = id;
}

/**
 *
 * @return
 */
AsyncRpcRequestId AsyncRpcRequestIf::getRequestId()
{
    return id_;
}

/**
 *
 */
EPRpcRequest::EPRpcRequest()
    : EPRpcRequest(RpcEndpointPtr(nullptr))
{
}

/**
 *
 * @param ep
 */
EPRpcRequest::EPRpcRequest(RpcEndpointPtr ep)
{
    ep_ = ep;
}

/**
 *
 */
EPRpcRequest::~EPRpcRequest()
{
}

/**
 *
 * @param ep
 */
void EPRpcRequest::setEndpoint(RpcEndpointPtr ep)
{
    ep_ = ep;
}

/**
 *
 * @return
 */
RpcEndpointPtr EPRpcRequest::getEndpoint()
{
    return ep_;
}

/**
 *
 * @param e
 * @param
 * @param resp
 */
void EPRpcRequest::handleError(Error&e, RpcRequestIf&, VoidPtr resp)
{
    if (e != ERR_OK) {
        GLOGERROR << "Encountered an error: " << e;
    }
}

/**
 *
 */
void EPRpcRequest::invoke()
{
}

/**
 *
 */
EPAsyncRpcRequest::EPAsyncRpcRequest()
    : EPAsyncRpcRequest(0, RpcEndpointPtr(nullptr), AsyncRpcRequestTrackerPtr(nullptr))
{
}

/**
 *
 * @param ep
 * @param tracker
 */
EPAsyncRpcRequest::EPAsyncRpcRequest(const AsyncRpcRequestId &id,
        RpcEndpointPtr ep, AsyncRpcRequestTrackerPtr tracker)
    : EPRpcRequest(ep)
{
    id_ = id;
    tracker_ = tracker;
}

/**
 *
 * @param tracker
 */
void EPAsyncRpcRequest::setRequestTracker(AsyncRpcRequestTrackerPtr tracker)
{
    tracker_ = tracker;
}
/**
 *
 * @return
 */
AsyncRpcRequestTrackerPtr EPAsyncRpcRequest::getRequestTracker()
{
    return tracker_;
}

/**
 *
 * @param cb
 */
void EPAsyncRpcRequest::onSuccessResponse(AsyncRpcRequestCb &cb)
{
    successCb_ = cb;
}

/**
 *
 * @param cb
 */
void EPAsyncRpcRequest::onFailedResponse(AsyncRpcRequestCb &cb)
{
    errorCb_ = cb;
}

/**
 *
 * @param
 * @param resp
 */
void EPAsyncRpcRequest::handleResponse(AsyncRpcRequestIf&, VoidPtr resp)
{
}

}  // namespace fds
