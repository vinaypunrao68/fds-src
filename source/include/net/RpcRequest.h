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

namespace fds {

class RpcFuncIf {
 public:
    virtual void invoke() = 0;
    virtual void setHeader(const fpi::AsyncHdr &hdr) = 0;

 protected:
};
typedef boost::shared_ptr<RpcFuncIf> RpcFuncPtr;

template <class ServiceT, class ArgT>
class RpcFunc1 : public RpcFuncIf {
 public:
    RpcFunc1(std::function<void (boost::shared_ptr<ArgT> arg)> rpc,
            boost::shared_ptr<ArgT> arg)
    : rpc_(rpc), rpcArg_(arg)
    {
    }
    virtual void invoke() override {
        // TODO(Rao): Lookup the endpoint and issue the call
        fds_assert(false);
    }
    virtual void setHeader(const fpi::AsyncHdr &hdr) {
        rpcArg_->header = hdr;
    }

 protected:
    std::function<void (boost::shared_ptr<ArgT> arg)> rpc_;
    boost::shared_ptr<ArgT> rpcArg_;
};

template <class ReturnT, class ServiceT, class ArgT>
class RpcRetFunc1 : public  RpcFunc1<ServiceT, ArgT> {

};

/* Async rpc request identifier */
typedef uint64_t AsyncRpcRequestId;

/* Async rpc request callback types */
typedef std::function<void(VoidPtr)> RpcRequestSuccessCb;
typedef std::function<void(const Error&, VoidPtr)> RpcRequestErrorCb;
typedef std::function<void(const Error&, VoidPtr, bool&)> RpcFailoverCb;

/* Async rpc request states */
enum AsyncRpcState {
    PRIOR_INVOCATION,
    INVOCATION_PROGRESS,
    RPC_COMPLETE
};

/**
 * Based class for async rpc requests
 */
class AsyncRpcRequestIf {
 public:
    AsyncRpcRequestIf()
    : AsyncRpcRequestIf(0)
    {
    }

    AsyncRpcRequestIf(const AsyncRpcRequestId &id)
    : id_(id)
    {
    }

    virtual ~AsyncRpcRequestIf() {}

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) = 0;

    virtual void complete(const Error& error) {
        state_ = RPC_COMPLETE;
        error_ = error;
    }

    /**
     *
     * @return
     */
    virtual bool isComplete()
    {
        return state_ == RPC_COMPLETE;
    }


    void setRpcFunc(RpcFuncPtr rpc) {
        rpc_ = rpc;
    }

    void setTimeoutMs(const uint32_t &timeout_ms) {
        timeoutMs_ = timeout_ms;
    }
    uint32_t getTimeout() {
        return timeoutMs_;
    }
    AsyncRpcRequestId getRequestId() {
        return id_;
    }
    void setRequestId(const AsyncRpcRequestId &id) {
        id_ = id;
    }

 protected:
    RpcFuncPtr rpc_;
    /* Request Id */
    AsyncRpcRequestId id_;
    /* Async rpc state */
    AsyncRpcState state_;
    /* Error if any */
    Error error_;
    /* Timeout */
    uint32_t timeoutMs_;
};
typedef boost::shared_ptr<AsyncRpcRequestIf> AsyncRpcRequestIfPtr;

/**
 * Wrapper around asynchronous rpc request
 */
class EPAsyncRpcRequest : public AsyncRpcRequestIf {
 public:
    EPAsyncRpcRequest()
     : EPAsyncRpcRequest(0, fpi::SvcUuid())
    {
    }

    EPAsyncRpcRequest(const AsyncRpcRequestId &id,
            const fpi::SvcUuid &uuid)
    : AsyncRpcRequestIf(id)
    {
        svcId_ = uuid;
    }

    void onSuccessCb(RpcRequestSuccessCb &cb) {
        successCb_ = cb;
    }

    void onErrorCb(RpcRequestErrorCb &cb) {
        errorCb_ = cb;
    }

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override {
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

 protected:
    /* Response callbacks.  If set they are invoked in handleResponse() */
    fpi::SvcUuid svcId_;
    RpcRequestSuccessCb successCb_;
    RpcRequestErrorCb errorCb_;
};
typedef boost::shared_ptr<EPAsyncRpcRequest> EPAsyncRpcRequestPtr;

struct AsyncRpcEpInfo {
    AsyncRpcEpInfo(const fpi::SvcUuid &id)
    : epId(id), status(ERR_OK)
    {
    }

    fpi::SvcUuid epId;
    Error status;
};

/**
 *
 */
class FailoverRpcRequest : public AsyncRpcRequestIf {
 public:
    /**
     *
     */
    FailoverRpcRequest()
     : FailoverRpcRequest(0, std::vector<fpi::SvcUuid>())
    {
    }

    /**
     *
     * @param id
     * @param uuid
     */
    FailoverRpcRequest(const AsyncRpcRequestId& id,
            const std::vector<fpi::SvcUuid>& uuid)
    : AsyncRpcRequestIf(0)
    {
    }

    /**
     * For adding endpoint.
     * NOTE: Only invoke during initialization.  Donot invoke once
     * the request is in progress
     * @param uuid
     */
    void addEndpoint(const fpi::SvcUuid& uuid) {
        eps_.push_back(AsyncRpcEpInfo(uuid));
    }

    /**
     * NOTE: Only invoke during initialization.  Donot invoke once
     * the request is in progress
     * @param cb
     */
    void onFailoverCb(RpcFailoverCb& cb) {
        failoverCb_ = cb;
    }

    /**
     * NOTE: Only invoke during initialization.  Donot invoke once
     * the request is in progress
     *
     * @param cb
     */
    void onSuccessCb(RpcRequestSuccessCb& cb) {
        successCb_ = cb;
    }

    /**
     * NOTE: Only invoke during initialization.  Donot invoke once
     * the request is in progress
     *
     * @param cb
     */
    void onErrorCb(RpcRequestErrorCb& cb) {
        errorCb_ = cb;
    }

    /**
     * Not thread safe.  Client should invoke this function only once.
     */
    void invoke()
    {
        state_ = INVOCATION_PROGRESS;
        AsyncHdr hdr;
        // TODO(Rao): populate the header.  Need a factory function to populate the header.
        rpc_->setHeader(hdr);
        try {
            rpc_->invoke();
        } catch (...) {
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
    void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
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
                if (header->msg_src_id != eps_[curEpIdx_].epId) {
                    /* Response isn't from the last endpoint we issued the request against.
                     * Don't do anything here
                     */
                    return;
                }
                errdEpId = header->msg_src_id;
                curEpIdx_++;
                if (curEpIdx_ == eps_.size()) {
                    complete(ERR_RPC_FAILED);
                    if (errorCb_) {
                        errorCb_(header->msg_code, payload);
                    }
                    gAsyncRpcTracker->removeFromTracking(this->getRequestId());
                } else {
                    if (failoverCb_) {
                        bool complete = false;
                        failoverCb_(header->msg_code, complete);
                        if (complete) {
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

 protected:

    /* Lock for synchronizing response handling */
    fds_mutex respLock_;

    /* Next endpoint to invoke the request on */
    uint8_t nextEp_;

    /* Endpoint collection */
    std::vector<AsyncRpcEpInfo> eps_;

    /* Callback to invoke before failing over to the next endpoint */
    RpcFailoverCb failoverCb_;
    /* Callback to invoke when response is succesfully received */
    RpcRequestSuccessCb successCb_;
    /* Callback to invoke when all the endpoints have failed */
    RpcRequestErrorCb errorCb_;
};
typedef boost::shared_ptr<FailoverRpcRequest> FailoverRpcRequestPtr;

#if 0
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



/**
 * Base class for rpc request
 */
template <class ServiceT, class ArgT>
class RpcRequestIf {
 public:
    virtual ~RpcRequestIf() {}

    void setRpc(std::function<void(ArgT)> rpc, ArgT& arg) {
        rpc_ = rpc;
        rpcArg_ = arg;
    }

    virtual void handleError(const Error&e, VoidPtr resp) = 0;
    virtual void invoke() = 0;

    void setError(const Error &e) {
        error_ = e;
    }

    Error getError() const {
        return error_;
    }

 protected:
    /* RPC Request error if any */
    Error error_;
    /* Rpc to invoke */
    std::function<void(ArgT)> rpc_;
    /* Argument to rpc call */
    ArgT rpcArg_;
};


/**
 * RPC request for single endpoint
 */
template <class ServiceT, class ArgT>
class EPRpcRequest : public RpcRequestIf<ServiceT, ArgT> {
 public:
    EPRpcRequest()
     : EPRpcRequest(fpi::SvcUuid()) {
    }

    explicit EPRpcRequest(const fpi::SvcUuid &uuid) {
        epId_ = uuid;
    }

    virtual ~EPRpcRequest() {
    }

    void setEndpointId(const fpi::SvcUuid &uuid) {
        epId_ = uuid;
    }

    fpi::SvcUuid getEndpointId() const {
        return epId_;
    }

    virtual void handleError(const Error&e, VoidPtr resp) override {
        // TODO(Rao): Handle error
    }

    virtual void invoke() {
        // TODO(Rao): Implement
    }

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

 protected:
    /* Endpoint id */
    fpi::SvcUuid epId_;
    /* Error if any was set */
    Error error_;
};
// typedef boost::shared_ptr<EPRpcRequest> EPRpcRequestPtr;
#endif



}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCREQUEST_H_
