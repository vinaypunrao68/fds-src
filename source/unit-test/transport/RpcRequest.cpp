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
    : EPRpcRequest(0)
{
}

/**
 *
 * @param ep
 */
EPRpcRequest::EPRpcRequest(const fpi::SvcUuid &uuid)
{
    epId_ = uuid;
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
void EPRpcRequest::setEndpointId(const fpi::SvcUuid &uuid)
{
    epId_ = uuid;
}

/**
 *
 * @return
 */
fpi::SvcUuid EPRpcRequest::getEndpointId() const
{
    return epId_;
}

/**
 *
 * @return
 */
Error EPRpcRequest::getError() const
{
    return error_;
}

/**
 *
 * @param e
 * @param
 * @param resp
 */
void EPRpcRequest::handleError(const Error&e, VoidPtr resp)
{
    if (e != ERR_OK) {
        GLOGERROR << "Encountered an error: " << e;
    }
}

/**
 *
 */
EPAsyncRpcRequest::EPAsyncRpcRequest()
    : EPAsyncRpcRequest(0, 0)
{
}

/**
 *
 * @param ep
 * @param tracker
 */
EPAsyncRpcRequest::EPAsyncRpcRequest(const AsyncRpcRequestId &id,
        const fpi::SvcUuid &uuid)
    : EPRpcRequest(uuid)
{
    id_ = id;
}

/**
 *
 * @param cb
 */
void EPAsyncRpcRequest::onSuccessResponse(RpcRequestSuccessCb &cb)
{
    successCb_ = cb;
}

/**
 *
 * @param cb
 */
void EPAsyncRpcRequest::onFailedResponse(RpcRequestErrorCb &cb)
{
    errorCb_ = cb;
}

/**
 *
 * @param
 * @param resp
 */
void EPAsyncRpcRequest::handleResponse(VoidPtr resp)
{
    Error e;
    // TODO(Rao): Parse out the error
    if (e != ERR_OK) {
        if (errorCb_) {
            errorCb_(e, resp);
        } else {
            handleError(e, resp);
        }
        return;
    }

    if (successCb_) {
        successCb_(resp);
    }
}

}  // namespace fds
