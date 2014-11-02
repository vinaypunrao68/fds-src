/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCREQUEST_H_
#define SOURCE_INCLUDE_NET_SVCREQUEST_H_

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
#include <fdsp_utils.h>
#include <concurrency/taskstatus.h>
#include <fds_counters.h>

#define SVCPERF(statement) statement

namespace fds {
/* Forward declarations */
struct EPSvcRequest;
struct FailoverSvcRequest;
struct QuorumSvcRequest;

namespace net {
template<class PayloadT> boost::shared_ptr<PayloadT>
ep_deserialize(Error &e, boost::shared_ptr<std::string> payload);
}

/* Async svc request identifier */
typedef uint64_t SvcRequestId;

/* Async svc request callback types */
typedef std::function<void(const SvcRequestId&)> SvcRequestCompletionCb;
typedef std::function<void(boost::shared_ptr<std::string>)> SvcRequestSuccessCb;
typedef std::function<void(const Error&,
        boost::shared_ptr<std::string>)> SvcRequestErrorCb;
typedef std::function<bool (const Error&, boost::shared_ptr<std::string>)> EPAppStatusCb;
typedef std::function<void(EPSvcRequest*,
                           const Error&, boost::shared_ptr<std::string>)> EPSvcRequestRespCb;
typedef std::function<void(FailoverSvcRequest*,
                           const Error&, boost::shared_ptr<std::string>)> FailoverSvcRequestRespCb;
typedef std::function<void(QuorumSvcRequest*,
                           const Error&, boost::shared_ptr<std::string>)> QuorumSvcRequestRespCb;

#define RESPONSE_MSG_HANDLER(func, ...) \
    std::bind(&func, this, ##__VA_ARGS__ , std::placeholders::_1, \
              std::placeholders::_2, std::placeholders::_3)

/* Async svc request states */
enum SvcRequestState {
    PRIOR_INVOCATION,
    INVOCATION_PROGRESS,
    SVC_REQUEST_COMPLETE
};

/**
 * @brief Svc request counters
 */
class SvcRequestCounters : public FdsCounters
{
 public:
    SvcRequestCounters(const std::string &id, FdsCountersMgr *mgr);
    ~SvcRequestCounters();

    /* Number of requests that have timedout */
    NumericCounter      timedout;
    /* Number of requests that experienced transport error */
    NumericCounter      invokeerrors;
    /* Number of responses that resulted in app acceptance */
    NumericCounter      appsuccess;
    /* Number of responses that resulted in app rejections */
    NumericCounter      apperrors;

    /* Serialization latency */
    LatencyCounter      serializationLat;
    /* Deserialization latency */
    LatencyCounter      deserializationLat;
    /* Send latency */
    LatencyCounter      sendLat;
    LatencyCounter      sendPayloadLat;
    /* Request latency */
    LatencyCounter      reqLat;
};
extern SvcRequestCounters* gSvcRequestCntrs;

template <class ReqT, class RespMsgT>
struct SvcRequestCbTask : concurrency::TaskStatus {
    void respCb(ReqT* req, const Error &e, boost::shared_ptr<std::string> respPayload)
    {
        response = net::ep_deserialize<RespMsgT>(const_cast<Error&>(e), respPayload);
        error = e;
        done();
    }
    SvcRequestCbTask() {
        error = ERR_INVALID;
        cb = std::bind(&SvcRequestCbTask<ReqT, RespMsgT>::respCb, this,
			std::placeholders::_1,  // NOLINT
			std::placeholders::_2, std::placeholders::_3);  // NOLINT
    }
    bool success() {
        return error == ERR_OK;
    }
    void reset()
    {
        response.reset(nullptr);
        error = ERR_INVALID;
        concurrency::TaskStatus::reset(1);
    }

    boost::shared_ptr<RespMsgT> response;
    Error error;
    std::function<void(ReqT*, const Error&, boost::shared_ptr<std::string>)> cb;
};

/**
* @brief Timer task for Async svc requests
*/
struct SvcRequestTimer : FdsTimerTask {
    SvcRequestTimer(const SvcRequestId &id,
                    const fpi::FDSPMsgTypeId &msgTypeId,
                    const fpi::SvcUuid &myEpId,
                    const fpi::SvcUuid &peerEpId);

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
 * Base class for async svc requests
 */
struct SvcRequestIf {
    SvcRequestIf();

    SvcRequestIf(const SvcRequestId &id, const fpi::SvcUuid &myEpId);

    virtual ~SvcRequestIf();

    template<class PayloadT>
    void setPayload(const fpi::FDSPMsgTypeId &msgTypeId,
                    const boost::shared_ptr<PayloadT> &payload)
    {
        boost::shared_ptr<std::string>buf;
        /* NOTE: Doing the serialization on calling thread */
        SVCPERF(util::StopWatch sw; sw.start());
        fds::serializeFdspMsg(*payload, buf);
        SVCPERF(gSvcRequestCntrs->serializationLat.update(sw.getElapsedNanos()));
        setPayloadBuf(msgTypeId, buf);
    }
    void setPayloadBuf(const fpi::FDSPMsgTypeId &msgTypeId,
                       boost::shared_ptr<std::string> &buf);

    virtual void invoke();

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) = 0;

    virtual void complete(const Error& error);

    virtual bool isComplete();

    virtual std::string logString() = 0;

    void setTimeoutMs(const uint32_t &timeout_ms);

    uint32_t getTimeout();

    SvcRequestId getRequestId();

    void setRequestId(const SvcRequestId &id);

    void setCompletionCb(SvcRequestCompletionCb &completionCb);

 public:
    struct SvcReqTs {
        /* Request latency stop watch */
        mutable uint64_t        rqStartTs;
        mutable uint64_t        rqSendStartTs;
        mutable uint64_t        rqSendEndTs;
        mutable uint64_t        rqRcvdTs;
        mutable uint64_t        rqHndlrTs;
        mutable uint64_t        rspSerStartTs;
        mutable uint64_t        rspSendStartTs;
        mutable uint64_t        rspRcvdTs;
        mutable uint64_t        rspHndlrTs;
    } ts;

 protected:
    virtual void invokeWork_() = 0;
    void sendPayload_(const fpi::SvcUuid &epId);
    std::stringstream& logSvcReqCommon_(std::stringstream &oss,
                                        const std::string &type);

    /* Lock for synchronizing response handling */
    fds_mutex respLock_;
    /* Request Id */
    SvcRequestId id_;
    /* My endpoint id */
    fpi::SvcUuid myEpId_;
    /* Async svc request state */
    SvcRequestState state_;
    /* Error if any */
    Error error_;
    /* Timeout */
    uint32_t timeoutMs_;
    /* Timer */
    FdsTimerTaskPtr timer_;
    /* Message type id */
    fpi::FDSPMsgTypeId msgTypeId_;
    /* Payload buffer */
    boost::shared_ptr<std::string> payloadBuf_;
    /* Completion cb */
    SvcRequestCompletionCb completionCb_;
    /* Minor version */
    int minor_version;
};
typedef boost::shared_ptr<SvcRequestIf> SvcRequestIfPtr;

#define EpInvokeRpc(SendIfT, func, svc_id, maj, min, ...)                       \
    do {                                                                        \
        auto net = NetMgr::ep_mgr_singleton();                                  \
        auto eph = net->svc_get_handle<SendIfT>(svc_id, maj, min);              \
        fds_verify(eph != NULL);                                                \
        try {                                                                   \
            eph->svc_rpc<SendIfT>()->func(__VA_ARGS__);                         \
            GLOGDEBUG << "[Svc] sent RPC "                                      \
                << std::hex << svc_id.svc_uuid << std::dec;                     \
        } catch(std::exception &e) {                                            \
            GLOGDEBUG << "[Svc] RPC error " << e.what();                        \
        } catch(...) {                                                          \
            GLOGDEBUG << "[Svc] Unknown RPC error ";                            \
            fds_assert(!"Unknown exception");                                   \
        }                                                                       \
    } while (0)

/**
 * Wrapper around asynchronous svc request
 */
struct EPSvcRequest : SvcRequestIf {
    EPSvcRequest();

    EPSvcRequest(const SvcRequestId &id,
                      const fpi::SvcUuid &myEpId,
                      const fpi::SvcUuid &peerEpId);

    ~EPSvcRequest();

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    fpi::SvcUuid getPeerEpId() const;

    void onResponseCb(EPSvcRequestRespCb cb);
    inline void set_minor(int minor) { minor_version = minor; }

 protected:
    virtual void invokeWork_() override;

    fpi::SvcUuid peerEpId_;
    /* Reponse callback */
    EPSvcRequestRespCb respCb_;

    friend class FailoverSvcRequest;
    friend class QuorumSvcRequest;
};
typedef boost::shared_ptr<EPSvcRequest> EPSvcRequestPtr;

/**
* @brief Base class for mutiple endpoint based requests
*/
struct MultiEpSvcRequest : SvcRequestIf {
    MultiEpSvcRequest();

    MultiEpSvcRequest(const SvcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    void addEndpoint(const fpi::SvcUuid& peerEpId);

    void onEPAppStatusCb(EPAppStatusCb cb);

 protected:
    EPSvcRequestPtr getEpReq_(fpi::SvcUuid &peerEpId);

    /* Endpoint request collection */
    std::vector<EPSvcRequestPtr> epReqs_;
    /* Callback to invoke before failing over to the next endpoint */
    EPAppStatusCb epAppStatusCb_;
};

/**
* @brief Use this class for issuing failover style request to a set of endpoints.  It will
* invoke the request agains each of the provides endpoints in order, until a success or they
* are exhausted.
*/
struct FailoverSvcRequest : MultiEpSvcRequest {
    FailoverSvcRequest();

    FailoverSvcRequest(const SvcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    FailoverSvcRequest(const SvcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const EpIdProviderPtr epProvider);


    ~FailoverSvcRequest();

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void onResponseCb(FailoverSvcRequestRespCb cb);

 protected:
    virtual void invokeWork_() override;

    bool moveToNextHealthyEndpoint_();

    /* Next endpoint to invoke the request on */
    uint32_t curEpIdx_;

    /* Response callback */
    FailoverSvcRequestRespCb respCb_;
};
typedef boost::shared_ptr<FailoverSvcRequest> FailoverSvcRequestPtr;


/**
* @brief Use this class for issuing quorum style request to a set of endpoints.  It will
* broadcast the request against all of the endponints.  When quorum number of
* succesfull responses are received the request is considered a success otherwise
* it's considered failure.
*/
struct QuorumSvcRequest : MultiEpSvcRequest {
    QuorumSvcRequest();

    QuorumSvcRequest(const SvcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const std::vector<fpi::SvcUuid>& peerEpIds);

    QuorumSvcRequest(const SvcRequestId& id,
            const fpi::SvcUuid &myEpId,
            const EpIdProviderPtr epProvider);

    ~QuorumSvcRequest();

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void setQuorumCnt(const uint32_t cnt);

    void onResponseCb(QuorumSvcRequestRespCb cb);

 protected:
    virtual void invokeWork_() override;

    uint32_t successAckd_;
    uint32_t errorAckd_;
    uint32_t quorumCnt_;
    QuorumSvcRequestRespCb respCb_;
};
typedef boost::shared_ptr<QuorumSvcRequest> QuorumSvcRequestPtr;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCREQUEST_H_
