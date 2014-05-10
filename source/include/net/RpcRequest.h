/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_RPCREQUEST_H_
#define SOURCE_INCLUDE_NET_RPCREQUEST_H_

#include <string>
#include <vector>
#include <functional>
#include <boost/shared_ptr.hpp>

#include <fds_typedefs.h>
#include <fdsp/fds_service_types.h>
#include <fds_error.h>

/**
 * Helper macro for invoking an rpc
 * req - RpcRequest context object
 * SvcType - class type of the service
 * method - method to invoke
 * ... - arguments to the method
 */
#define INVOKE_RPC(req, SvcType, method, ...)                               \
    do {                                                                    \
        boost::shared_ptr<SvcType> client;                                  \
        auto ep = NetMgr::ep_mgr_singleton()->\
            svc_lookup((req).getEndpointId(), 0 , 0);                       \
        Error status = ep->ep_get_status();                                 \
        if (status != ERR_OK) {                                             \
            (req).setError(status);                                         \
            GLOGWARN << "Not invoking the method: " ## method;              \
            break;                                                          \
        } else {                                                            \
            client = ep->svc_rpc<SvcType>();                                \
        }                                                                   \
        try {                                                               \
            client->method(__VA_ARGS__);                                    \
        } catch(...) {                                                      \
            (req).handleError(ERR_INVALID, VoidPtr(nullptr));               \
        }                                                                   \
    } while (false)

namespace fds {

/* Async rpc request identifier */
typedef uint64_t AsyncRpcRequestId;

/**
 * Base class for rpc request
 */
class RpcRequestIf {
 public:
    virtual ~RpcRequestIf() {}

    virtual void handleError(const Error&e, VoidPtr resp) = 0;

    void setError(const Error &e);
    Error getError() const;

 protected:
    /* RPC Request error if any */
    Error error_;
};

/**
 * Based class for async rpc requests
 */
class AsyncRpcRequestIf {
 public:
    virtual ~AsyncRpcRequestIf() {}

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) = 0;
    virtual void complete(const Error &status) = 0;

    void setTimeoutMs(const uint32_t &timeout_ms);
    uint32_t getTimeout();

    AsyncRpcRequestId getRequestId();
    void setRequestId(const AsyncRpcRequestId &id);

 protected:
    /* Request Id */
    AsyncRpcRequestId id_;
};
typedef boost::shared_ptr<AsyncRpcRequestIf> AsyncRpcRequestIfPtr;

/**
 * RPC request for single endpoint
 */
class EPRpcRequest : public RpcRequestIf {
 public:
    EPRpcRequest();
    explicit EPRpcRequest(const fpi::SvcUuid &uuid);
    virtual ~EPRpcRequest();

    void setEndpointId(const fpi::SvcUuid &uuid);
    fpi::SvcUuid getEndpointId() const;

    virtual void handleError(const Error&e, VoidPtr resp) override;
    Error getError() const;

    /**
     * Wrapper for invoking the member function on an endpoint
     * Specialization for void return type
     * @param mf - member function
     * @param args - arguments to member function
     */
    template <typename T, typename ...Args>
    void invoke(void (T::*mf)(Args...), Args &&... args)
    {
        T *client = nullptr;
        Error e;
        // TODO(Rao): Get client from ep id
        try {
            (client->*mf)(std::forward<Args>(args)...);
        } catch(...) {
            handleError(e, VoidPtr(nullptr));
        }
    }


#if 0
    /**
     * Wrapper for invoking the member function on an endpoint
     * @param mf - member function
     * @param args - arguments to member function
     */
    template <typename T, typename R, typename ...Args>
    R invoke(R (T::*mf)(Args...), Args &&... args)
    {
        T *client = nullptr;
        Error e;
        R ret;
        // TODO(Rao): Get client from ep id
        try {
            ret = (client->*mf)(std::forward<Args>(args)...);
        } catch(...) {
            handleError(e, VoidPtr(nullptr));
        }
        return ret;
    }
#endif

 protected:
    /* Endpoint id */
    fpi::SvcUuid epId_;
    /* Error if any was set */
    Error error_;
};
typedef boost::shared_ptr<EPRpcRequest> EPRpcRequestPtr;

/* Async rpc request callback type */
typedef std::function<void(VoidPtr)> RpcRequestSuccessCb;
typedef std::function<void(const Error &, VoidPtr)> RpcRequestErrorCb;

/**
 * Wrapper around asynchronous rpc request
 */
class EPAsyncRpcRequest : public EPRpcRequest, public AsyncRpcRequestIf {
 public:
    EPAsyncRpcRequest();
    EPAsyncRpcRequest(const AsyncRpcRequestId &id,
            const fpi::SvcUuid &uuid);

    void onSuccessCb(RpcRequestSuccessCb &cb);
    void onErrorCb(RpcRequestErrorCb &cb);

    virtual void complete(const Error &status) override;
    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

 protected:
    /* Response callbacks.  If set they are invoked in handleResponse() */
    RpcRequestSuccessCb successCb_;
    RpcRequestErrorCb errorCb_;
};
typedef boost::shared_ptr<EPAsyncRpcRequest> EPAsyncRpcRequestPtr;

/**
 *
 */
class FailoverRpcRequest : public AsyncRpcRequestIf {
 public:
    FailoverRpcRequest();
    FailoverRpcRequest(const AsyncRpcRequestId &id,
            const std::vector<fpi::SvcUuid> &uuid);

    void addEndpoint(const fpi::SvcUuid &uuid);
    void onFailoverCb(RpcRequestErrorCb &cb);
    void onSuccessCb(RpcRequestSuccessCb &cb);
    void onErrorCb(RpcRequestErrorCb &cb);

    virtual void complete(const Error &status) override;
    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

 protected:
    /* Lock for this request */
    fds_spinlock lock_;

    /* Next endpoint to invoke the request on */
    uint8_t nextEp_;

    /* Endpoint collection */
    std::vector<fpi::SvcUuid> eps_;

    /* Callback to invoke before failing over to the next endpoint */
    RpcRequestErrorCb failoverCb_;
    /* Callback to invoke when response is succesfully received */
    RpcRequestErrorCb successCb_;
    /* Callback to invoke when all the endpoints have failed */
    RpcRequestErrorCb errorCb_;
};
typedef boost::shared_ptr<FailoverRpcRequest> FailoverRpcRequestPtr;

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCREQUEST_H_
