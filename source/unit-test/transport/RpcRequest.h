/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_RPC_REQUEST_H_
#define SOURCE_INCLUDE_RPC_REQUEST_H_

#include <functional>
#include <boost/shared_ptr.hpp>
#include <fds_error.h>

namespace fds {

/* Forward declarations */
class AsyncRpcRequestTracker;
typedef boost::shared_ptr<AsyncRpcRequestTracker> AsyncRpcRequestTrackerPtr;

/* Async rpc request identifier */
typedef uint64_t AsyncRpcRequestId;

class RpcEndpoint {};
typedef boost::shared_ptr<RpcEndpoint> RpcEndpointPtr;

/**
 * Base class for rpc request
 */
class RpcRequestIf {
public:
    virtual ~RpcRequestIf() {}
    virtual void invoke() = 0;
    virtual void handleError(Error&e, RpcRequestIf&, VoidPtr resp) = 0;
};

/**
 * Based class for async rpc requests
 */
class AsyncRpcRequestIf {
public:
    virtual ~AsyncRpcRequestIf() {}
    AsyncRpcRequestId getRequestId();
    void setRequestId(const AsyncRpcRequestId &id);
    virtual void handleResponse(AsyncRpcRequestIf&, VoidPtr resp) = 0;

protected:
    /* Request Id */
    AsyncRpcRequestId id_;
};

/**
 * RPC request for single endpoint
 */
class EPRpcRequest : public RpcRequestIf {
public:
    EPRpcRequest();
    EPRpcRequest(RpcEndpointPtr ep);
    virtual ~EPRpcRequest();

    void setEndpoint(RpcEndpointPtr ep);
    RpcEndpointPtr getEndpoint();

    virtual void invoke() override;
    virtual void handleError(Error&e, RpcRequestIf&, VoidPtr resp) override;

    template<typename F, typename... ArgTypes>
    Error invoke(F &&f, ArgTypes... args) {
        Error e;
        try {
            // TODO(Rao)
            // 1. Check endpoint is valid, if so invoke the function.  If not valid return the error
            // that is stored in endpoint
            auto mem_fn = std::bind(f, _1, args...);
            mem_fn(ep_.get());
        } catch (...) {
            // TODO(Rao): do proper error handling
            e = ERR_INVALID;
        }
        if (e != ERR_OK) {
            handleError(e, static_cast<RpcRequestIf&>(*this), VoidPtr(nullptr));
        }
        return e;
    }

protected:
    /* Endpoint reference */
    RpcEndpointPtr ep_;
};
typedef boost::shared_ptr<EPRpcRequest> EPRpcRequestPtr;

/* Async rpc request callback type */
typedef std::function<void()> AsyncRpcRequestCb;


/**
 * Wrapper around asynchronous rpc request
 */
class EPAsyncRpcRequest : public EPRpcRequest, public AsyncRpcRequestIf {
public:
    EPAsyncRpcRequest();
    EPAsyncRpcRequest(const AsyncRpcRequestId &id,
            RpcEndpointPtr ep, AsyncRpcRequestTrackerPtr tracker);

    void setRequestTracker(AsyncRpcRequestTrackerPtr tracker);
    AsyncRpcRequestTrackerPtr getRequestTracker();

    void onSuccessResponse(AsyncRpcRequestCb &cb);
    void onFailedResponse(AsyncRpcRequestCb &cb);

    virtual void handleResponse(AsyncRpcRequestIf&, VoidPtr resp) override;
protected:

    /* Back pointer to tracker */
    AsyncRpcRequestTrackerPtr tracker_;

    /* Response callbacks.  If set they are invoked in handleResponse() */
    AsyncRpcRequestCb successCb_;
    AsyncRpcRequestCb errorCb_;
};

#if 0
class RequestTracker {
public:
    void addForTracking(AsyncRpcRequestPtr req);
    void removeFromTracking(AsyncRpcRequestPtr req);
};
boost::shared_ptr<RequestTracker> RequestTrackerPtr;

class MultiEndpointRpcRequest : public AsyncRpcRequest {
public:

};

class BroadcastRpcRequest : public MultiEndpointRpcRequest {
public:
};

class FailoverRpcRequest : public MultiEndpointRpcRequest {
public:
};
#endif

}  // namespace fds

#endif  // SOURCE_INCLUDE_RPC_REQUEST_H_
