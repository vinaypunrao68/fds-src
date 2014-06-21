/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_RPCREQUEST_H_
#define SOURCE_INCLUDE_NET_RPCREQUEST_H_

#include <string>
#include <vector>
#include <functional>
#include <boost/shared_ptr.hpp>

#include <fds_timer.h>
#include <fds_typedefs.h>
#include <fdsp/fds_service_types.h>
#include <fds_error.h>
#include <fds_dmt.h>
#include <dlt.h>
#include <net/RpcFunc.h>

namespace fds {
/* Forward declarations */
struct EPAsyncRpcRequest;
struct FailoverRpcRequest;
struct QuorumRpcRequest;

/* Async rpc request identifier */
typedef uint64_t AsyncRpcRequestId;

/* Async rpc request callback types */
typedef std::function<void(const AsyncRpcRequestId&)> RpcRequestCompletionCb;
typedef std::function<void(boost::shared_ptr<std::string>)> RpcRequestSuccessCb;
typedef std::function<void(const Error&,
        boost::shared_ptr<std::string>)> RpcRequestErrorCb;
typedef std::function<bool (const Error&, boost::shared_ptr<std::string>)> EPAppStatusCb;
typedef std::function<void(EPAsyncRpcRequest*,
                           const Error&, boost::shared_ptr<std::string>)> EPAsyncRpcRespCb;
typedef std::function<void(FailoverRpcRequest*,
                           const Error&, boost::shared_ptr<std::string>)> FailoverRpcRespCb;
typedef std::function<void(QuorumRpcRequest*,
                           const Error&, boost::shared_ptr<std::string>)> QuorumRpcRespCb;

/* Async rpc request states */
enum AsyncRpcState {
    PRIOR_INVOCATION,
    INVOCATION_PROGRESS,
    RPC_COMPLETE
};

/**
* @brief Timer task for Async Rpc requests
*/
struct AsyncRpcTimer : FdsTimerTask {
    AsyncRpcTimer(const AsyncRpcRequestId &id,
                  const fpi::SvcUuid &myEpId, const fpi::SvcUuid &peerEpId);

    virtual void runTimerTask() override;

 protected:
    boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr> header_;
};

struct EpIdProvider {
    virtual fpi::SvcUuid getNextEp() = 0;
    virtual std::vector<fpi::SvcUuid> getEps() = 0;
};
typedef boost::shared_ptr<EpIdProvider> EpIdProviderPtr;

struct DltObjectIdEpProvider : EpIdProvider {
    explicit DltObjectIdEpProvider(const ObjectID &objId)
    {
    }
    // TODO(Rao): Remove, once dlt is managed by platform.  At that point
    // we don't need this interface
    explicit DltObjectIdEpProvider(DltTokenGroupPtr group)
    {
        auto &g = *group;
        for (uint32_t i = 0; i < g.getLength(); i++) {
            epIds_.push_back(g[i].toSvcUuid());
        }
    }
    virtual fpi::SvcUuid getNextEp() override
    {
        // TODO(Rao): Impl
        fds_verify(!"Not impl");
        return fpi::SvcUuid();
    }
    virtual std::vector<fpi::SvcUuid> getEps() override
    {
        return epIds_;
    }

 protected:
    /* List of endpoints.
     * NOTE: We may not need this memenber.  We can make getting endpoints
     * more dynamic and based on DMT
     */
    std::vector<fpi::SvcUuid> epIds_;
};

struct DmtVolumeIdEpProvider : EpIdProvider {
    explicit DmtVolumeIdEpProvider(const fds_volid_t& volId)
    {
        fds_verify(!"Not impl");
    }
    // TODO(Rao): Remove, once dlt is managed by platform.  At that point
    // we don't need this interface
    explicit DmtVolumeIdEpProvider(DmtColumnPtr group)
    {
        auto &g = *group;
        for (uint32_t i = 0; i < g.getLength(); i++) {
            epIds_.push_back(g[i].toSvcUuid());
        }
    }
    virtual fpi::SvcUuid getNextEp() override
    {
        // TODO(Rao): Impl
        fds_verify(!"Not impl");
        return fpi::SvcUuid();
    }
    virtual std::vector<fpi::SvcUuid> getEps() override
    {
        return epIds_;
    }

 protected:
    /* List of endpoints.
     * NOTE: We may not need this memenber.  We can make getting endpoints
     * more dynamic and based on DMT
     */
    std::vector<fpi::SvcUuid> epIds_;
};


/**
 * Base class for async rpc requests
 */
struct AsyncRpcRequestIf {
    AsyncRpcRequestIf();

    AsyncRpcRequestIf(const AsyncRpcRequestId &id, const fpi::SvcUuid &myEpId);

    virtual ~AsyncRpcRequestIf();

    virtual void invoke() = 0;

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) = 0;

    virtual void complete(const Error& error);

    virtual bool isComplete();

    virtual std::string logString() = 0;

    void setRpcFunc(RpcFunc rpc);

    void setTimeoutMs(const uint32_t &timeout_ms);

    uint32_t getTimeout();

    AsyncRpcRequestId getRequestId();

    void setRequestId(const AsyncRpcRequestId &id);

    void setCompletionCb(RpcRequestCompletionCb &completionCb);

    static void postError(boost::shared_ptr<fpi::AsyncHdr> header);

 protected:
    void invokeCommon_(const fpi::SvcUuid &epId);
    std::stringstream& logRpcReqCommon_(std::stringstream &oss,
                                        const std::string &type);

    /* Lock for synchronizing response handling */
    fds_mutex respLock_;
    /* Wrapper around rpc function call */
    RpcFunc rpc_;
    /* Request Id */
    AsyncRpcRequestId id_;
    /* My endpoint id */
    fpi::SvcUuid myEpId_;
    /* Async rpc state */
    AsyncRpcState state_;
    /* Error if any */
    Error error_;
    /* Timeout */
    uint32_t timeoutMs_;
    /* Timer */
    FdsTimerTaskPtr timer_;
    /* Completion cb */
    RpcRequestCompletionCb completionCb_;
};
typedef boost::shared_ptr<AsyncRpcRequestIf> AsyncRpcRequestIfPtr;


/**
 * Wrapper around asynchronous rpc request
 */
struct EPAsyncRpcRequest : AsyncRpcRequestIf {
    EPAsyncRpcRequest();

    EPAsyncRpcRequest(const AsyncRpcRequestId &id,
                      const fpi::SvcUuid &myEpId,
                      const fpi::SvcUuid &peerEpId);

    ~EPAsyncRpcRequest();

    virtual void invoke() override;

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    fpi::SvcUuid getPeerEpId() const;

    void onResponseCb(EPAsyncRpcRespCb cb);

 protected:
    fpi::SvcUuid peerEpId_;
    /* Reponse callback */
    EPAsyncRpcRespCb respCb_;

    friend class FailoverRpcRequest;
};
typedef boost::shared_ptr<EPAsyncRpcRequest> EPAsyncRpcRequestPtr;

/**
* @brief Base class for mutiple endpoint based requests
*/
struct MultiEpAsyncRpcRequest : AsyncRpcRequestIf {
    MultiEpAsyncRpcRequest();

    MultiEpAsyncRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    void addEndpoint(const fpi::SvcUuid& peerEpId);

    void onEPAppStatusCb(EPAppStatusCb cb);

 protected:
    EPAsyncRpcRequestPtr getEpReq_(fpi::SvcUuid &peerEpId);

    /* Endpoint request collection */
    std::vector<EPAsyncRpcRequestPtr> epReqs_;
    /* Callback to invoke before failing over to the next endpoint */
    EPAppStatusCb epAppStatusCb_;
};

/**
* @brief Use this class for issuing failover style rpc to a set of endpoints.  It will
* invoke the rpc agains each of the provides endpoints in order, until a success or they
* are exhausted.
*/
struct FailoverRpcRequest : MultiEpAsyncRpcRequest {
    FailoverRpcRequest();

    FailoverRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    FailoverRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const EpIdProviderPtr epProvider);


    ~FailoverRpcRequest();

    virtual void invoke() override;

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void onResponseCb(FailoverRpcRespCb cb);

 protected:
    bool moveToNextHealthyEndpoint_();

    void invokeInternal_();

    /* Next endpoint to invoke the request on */
    uint32_t curEpIdx_;

    /* Response callback */
    FailoverRpcRespCb respCb_;
};
typedef boost::shared_ptr<FailoverRpcRequest> FailoverRpcRequestPtr;


struct QuorumRpcRequest : MultiEpAsyncRpcRequest {
    QuorumRpcRequest();

    QuorumRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    QuorumRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const EpIdProviderPtr epProvider);

    ~QuorumRpcRequest();

    virtual void invoke() override;

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void setQuorumCnt(const uint32_t cnt);

    void onResponseCb(QuorumRpcRespCb cb);

 protected:
    uint32_t successAckd_;
    uint32_t errorAckd_;
    uint32_t quorumCnt_;
    QuorumRpcRespCb respCb_;
};
typedef boost::shared_ptr<QuorumRpcRequest> QuorumRpcRequestPtr;
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
        peerEpId_ = uuid;
    }

    virtual ~EPRpcRequest() {
    }

    void setEndpointId(const fpi::SvcUuid &uuid) {
        peerEpId_ = uuid;
    }

    fpi::SvcUuid getEndpointId() const {
        return peerEpId_;
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
    fpi::SvcUuid peerEpId_;
    /* Error if any was set */
    Error error_;
};
// typedef boost::shared_ptr<EPRpcRequest> EPRpcRequestPtr;
#endif



}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCREQUEST_H_
