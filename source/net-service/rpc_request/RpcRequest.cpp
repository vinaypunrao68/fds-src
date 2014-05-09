/* Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <util/Log.h>
#include <net/RpcRequest.h>

namespace fds {
/**
 *
 * @param e
 */
void RpcRequestIf::setError(const Error &e)
{
    error_ = e;
}
/**
 *
 * @return
 */
Error RpcRequestIf::getError() const
{
    return error_;
}

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
    : EPRpcRequest(fpi::SvcUuid())
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
    : EPAsyncRpcRequest(0, fpi::SvcUuid())
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
void EPAsyncRpcRequest::onSuccessCb(RpcRequestSuccessCb &cb)
{
    successCb_ = cb;
}

/**
 *
 * @param cb
 */
void EPAsyncRpcRequest::onErrorCb(RpcRequestErrorCb &cb)
{
    errorCb_ = cb;
}

/**
 *
 */
void EPAsyncRpcRequest::complete(const Error &status)
{
    // TODO(Rao): Remove from tracking
}

/**
 *
 * @param
 * @param resp
 */
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
            handleError(e, payload);
        }
        return;
    }

    if (successCb_) {
        successCb_(payload);
    }
}

}  // namespace fds
