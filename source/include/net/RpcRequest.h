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
#include <net/net-service.h>
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

#define RESPONSE_MSG_HANDLER(func, ...) \
    std::bind(&func, this, ##__VA_ARGS__ , std::placeholders::_1, \
              std::placeholders::_2, std::placeholders::_3)

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

    virtual void invoke();

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
    virtual void invokeWork_() = 0;
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

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    fpi::SvcUuid getPeerEpId() const;

    void onResponseCb(EPAsyncRpcRespCb cb);

 protected:
    virtual void invokeWork_() override;

    fpi::SvcUuid peerEpId_;
    /* Reponse callback */
    EPAsyncRpcRespCb respCb_;

    friend class FailoverRpcRequest;
    friend class QuorumRpcRequest;
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

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void onResponseCb(FailoverRpcRespCb cb);

 protected:
    virtual void invokeWork_() override;

    bool moveToNextHealthyEndpoint_();

    /* Next endpoint to invoke the request on */
    uint32_t curEpIdx_;

    /* Response callback */
    FailoverRpcRespCb respCb_;
};
typedef boost::shared_ptr<FailoverRpcRequest> FailoverRpcRequestPtr;


/**
* @brief Use this class for issuing quorum style rpc to a set of endpoints.  It will
* invoke broadcast the rpc against all of the endponints.  When quorum number of
* succesfull responses are received the request is considered a success otherwise
* it's considered failure.
*/
struct QuorumRpcRequest : MultiEpAsyncRpcRequest {
    QuorumRpcRequest();

    QuorumRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    QuorumRpcRequest(const AsyncRpcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const EpIdProviderPtr epProvider);

    ~QuorumRpcRequest();

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void setQuorumCnt(const uint32_t cnt);

    void onResponseCb(QuorumRpcRespCb cb);

 protected:
    virtual void invokeWork_() override;

    uint32_t successAckd_;
    uint32_t errorAckd_;
    uint32_t quorumCnt_;
    QuorumRpcRespCb respCb_;
};
typedef boost::shared_ptr<QuorumRpcRequest> QuorumRpcRequestPtr;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCREQUEST_H_
