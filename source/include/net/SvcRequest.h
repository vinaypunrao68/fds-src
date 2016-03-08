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
#include <fdsp/svc_types_types.h>
#include <fds_error.h>
#include <fds_dmt.h>
#include <dlt.h>
#include <fdsp_utils.h>
#include <concurrency/taskstatus.h>
#include <fds_counters.h>
#include <fds_module_provider.h>

namespace fds {
/* Forward declarations */
struct EPSvcRequest;
struct FailoverSvcRequest;
struct QuorumSvcRequest;
struct MultiPrimarySvcRequest;
template <class T> struct SvcRequestRetrier;

using StringPtr = boost::shared_ptr<std::string>;

/* Async svc request identifier */
using SvcRequestId = uint64_t;
using TaskExecutorId = size_t;

/* Async svc request callback types */
struct SvcRequestIf;
using SvcRequestIfPtr = boost::shared_ptr<SvcRequestIf>;
using EPSvcRequestPtr = boost::shared_ptr<EPSvcRequest>;
using SvcRequestCompletionCb = std::function<SvcRequestIfPtr(const SvcRequestId&)>;
using SvcRequestSuccessCb = std::function<void(boost::shared_ptr<std::string>)>;
using SvcRequestErrorCb = std::function<void(const Error&, boost::shared_ptr<std::string>)>;
using EPAppStatusCb = std::function<bool (const Error&, boost::shared_ptr<std::string>)>;
using EPSvcRequestRespCb = std::function<void(EPSvcRequest*, const Error&, boost::shared_ptr<std::string>)>;
using FailoverSvcRequestRespCb = std::function<void(FailoverSvcRequest*,
                                                    const Error&,
                                                    boost::shared_ptr<std::string>)>;
using QuorumSvcRequestRespCb = std::function<void(QuorumSvcRequest*,
                                                  const Error&,
                                                  boost::shared_ptr<std::string>)>;
using MultiPrimarySvcRequestRespCb = std::function<void(MultiPrimarySvcRequest*,
                                                        const Error&,
                                                        StringPtr)>;
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
};

template <class ReqT, class RespMsgT>
struct SvcRequestCbTask : concurrency::TaskStatus {
    void respCb(ReqT* req, const Error &e, boost::shared_ptr<std::string> respPayload)
    {
        response = fds::deserializeFdspMsg<RespMsgT>(const_cast<Error&>(e), respPayload);
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
struct SvcRequestTimer : HasModuleProvider, FdsTimerTask {
    SvcRequestTimer(CommonModuleProviderIf* provider,
                    const SvcRequestId &id,
                    const fpi::FDSPMsgTypeId &msgTypeId,
                    const fpi::SvcUuid &myEpId,
                    const fpi::SvcUuid &peerEpId,
                    const fpi::ReplicaId &replicaId,
                    const int32_t &replicaVersion);

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
* @brief Interface for all tracked requests
*/
struct TrackableRequest : HasModuleProvider {
    TrackableRequest();
    TrackableRequest(CommonModuleProviderIf* provider,
                   const SvcRequestId &id);
    virtual ~TrackableRequest() = default;
    inline SvcRequestId getRequestId() const { return id_; }
    inline bool isTracked() const;
    inline void setRequestId(const SvcRequestId &id) { id_ = id; }
    inline void setTimeoutMs(const uint32_t &timeout_ms) { timeoutMs_ = timeout_ms; }
    inline uint32_t getTimeout() const { return timeoutMs_; }
    inline void setCompletionCb(SvcRequestCompletionCb &completionCb) {
        completionCb_ = completionCb;
    }
    inline bool isComplete() const { return state_ == SVC_REQUEST_COMPLETE; }

    virtual std::string logString() = 0;
    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) = 0;
    virtual void complete(const Error& error);
 protected:
    /* Request Id */
    SvcRequestId id_;
    /* Async svc request state */
    SvcRequestState state_;
    /* Timeout */
    uint32_t timeoutMs_;
    /* Timer */
    FdsTimerTaskPtr timer_;
    /* Completion cb */
    SvcRequestCompletionCb completionCb_;
};

/**
 * Base class for async svc requests
 */
struct SvcRequestIf : TrackableRequest {
    SvcRequestIf();

    SvcRequestIf(CommonModuleProviderIf* provider,
                 const SvcRequestId &id, const fpi::SvcUuid &myEpId);

    virtual ~SvcRequestIf();

    template<class PayloadT>
    void setPayload(const fpi::FDSPMsgTypeId &msgTypeId,
                    const boost::shared_ptr<PayloadT> &payload)
    {
        boost::shared_ptr<std::string>buf;
        /* NOTE: Doing the serialization on calling thread */
        fds::serializeFdspMsg(*payload, buf);
        setPayloadBuf(msgTypeId, buf);
    }
    void setPayloadBuf(const fpi::FDSPMsgTypeId &msgTypeId,
                       const boost::shared_ptr<std::string> &buf);

    template<class PayloadT>
    boost::shared_ptr<PayloadT> getRequestPayload(const fpi::FDSPMsgTypeId &msgTypeId) {
        if (msgTypeId != msgTypeId_) return NULL;
        Error e;
        return fds::deserializeFdspMsg<PayloadT>(e, payloadBuf_);
    }

    virtual void invoke();

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) = 0;

    virtual void completeReq(const Error& error,
                          const fpi::AsyncHdrPtr& header,
                          const StringPtr& payload);


    TaskExecutorId getTaskExecutorId();

    void setTaskExecutorId(const TaskExecutorId &teid);
    void unsetTaskExecutorId();
    bool taskExecutorIdIsSet();

    inline const fpi::AsyncHdrPtr& responseHeader() const { return respHeader_; }
    inline Error responseStatus() const { return respHeader_->msg_code; }
    inline const StringPtr& responsePayload() const { return respPayload_; }

 public:

    /* DLT Version (if applicable) */
    fds_uint64_t dlt_version_ { DLT_VER_INVALID };

 protected:
    virtual void invokeWork_() = 0;
    std::stringstream& logSvcReqCommon_(std::stringstream &oss,
                                        const std::string &type);

    /* Task executor ID. When set, replaces the Request ID for task serialization. */
    TaskExecutorId teid_;
    bool teidIsSet_;
    /* My endpoint id */
    fpi::SvcUuid myEpId_;
    /* Message type id */
    fpi::FDSPMsgTypeId msgTypeId_;
    /* Payload buffer */
    boost::shared_ptr<std::string> payloadBuf_;
    /* Response header */
    fpi::AsyncHdrPtr respHeader_;
    /* Response payload */
    StringPtr respPayload_;
    /* Where the request is fire and forget or not */
    bool fireAndForget_;
    /* Minor version */
    int minor_version;
};

/**
 * Wrapper around asynchronous svc request
 */
struct EPSvcRequest : SvcRequestIf {
    EPSvcRequest();

    EPSvcRequest(CommonModuleProviderIf* provider,
                 const SvcRequestId &id,
                 const fpi::SvcUuid &myEpId,
                 const fpi::SvcUuid &peerEpId);

    ~EPSvcRequest();

    virtual void invoke() override;
    void invokeDirect();
    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    fpi::SvcUuid getPeerEpId() const;

    void onResponseCb(EPSvcRequestRespCb cb);
    inline void set_minor(int minor) { minor_version = minor; }
    inline void setReplicaId(const fpi::ReplicaId &replicaId) { replicaId_ = replicaId; }
    inline void setReplicaVersion(const int32_t &version) { replicaVersion_ = version; }

 protected:
    virtual void invokeWork_() override;
    void sendPayload_();

    /**
     * TODO(Rao): Consider refactoring so that we keep relevant memebers in async request
     * header instead of individual members here, so that we can use the same request header
     * when sending across the wire
     */
    fpi::SvcUuid                    peerEpId_;
    fpi::ReplicaId                  replicaId_;
    int32_t                         replicaVersion_;
    /* Reponse callback */
    EPSvcRequestRespCb              respCb_;

    friend class FailoverSvcRequest;
    friend class QuorumSvcRequest;
    friend class MultiPrimarySvcRequest;
    friend class VolumeGroupBroadcastRequest;
    friend class VolumeGroupFailoverRequest;
    friend class SvcRequestRetrier<EPSvcRequest>;
};

/**
* @brief Base class for mutiple endpoint based requests
*/
struct MultiEpSvcRequest : SvcRequestIf {
    MultiEpSvcRequest();

    MultiEpSvcRequest(CommonModuleProviderIf* provider,
                      const SvcRequestId& id,
                      const fpi::SvcUuid &myEpId,
                      fds_uint64_t const dlt_version,
                      const std::vector<fpi::SvcUuid>& peerEpIds);

    void addEndpoint(const fpi::SvcUuid& peerEpId,
                     fds_uint64_t const dlt_version,
                     const fpi::ReplicaId &replicaId,
                     const int32_t &replicaVersion);
    void addEndpoints(const std::vector<fpi::SvcUuid> &peerEpIds, fds_uint64_t const dlt_version);

    void onEPAppStatusCb(EPAppStatusCb cb);

    inline const fpi::AsyncHdrPtr& responseHeader(uint8_t epIdx) const {
        return epReqs_[epIdx]->responseHeader();
    }
    inline Error responseStatus(uint8_t epIdx) const {
        return epReqs_[epIdx]->responseStatus();
    }
    inline const StringPtr& responsePayload(uint8_t epIdx) const {
        return epReqs_[epIdx]->responsePayload();
    }
    inline const EPSvcRequestPtr& ep(uint8_t epIdx) const {
        return epReqs_[epIdx];
    }

 protected:
    EPSvcRequestPtr getEpReq_(const fpi::SvcUuid &peerEpId);

    /* Endpoint request collection */
    std::vector<EPSvcRequestPtr> epReqs_;
    /* Callback to invoke before failing over to the next endpoint */
    EPAppStatusCb epAppStatusCb_;
    /* Keep track of the worst error we've seen */
    Error response_ {ERR_OK};
};

/**
* @brief Use this class for issuing failover style request to a set of endpoints.  It will
* invoke the request agains each of the provides endpoints in order, until a success or they
* are exhausted.
*/
struct FailoverSvcRequest : MultiEpSvcRequest {
    FailoverSvcRequest();

    FailoverSvcRequest(CommonModuleProviderIf* provider,
                       const SvcRequestId& id,
                       const fpi::SvcUuid &myEpId,
                       fds_uint64_t const dlt_version,
                       const std::vector<fpi::SvcUuid>& peerEpIds);

    FailoverSvcRequest(CommonModuleProviderIf* provider,
                       const SvcRequestId& id,
                       const fpi::SvcUuid &myEpId,
                       fds_uint64_t const dlt_version,
                       const EpIdProviderPtr epProvider);


    ~FailoverSvcRequest();

    virtual void invoke() override;

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void onResponseCb(FailoverSvcRequestRespCb cb);

    /**
    * @brief  Returns uuid of last service that responsed.  Only use after responsed cb
    * has been invoked
    */
    fpi::SvcUuid getLastRespondedSvcUuid() const;

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

    QuorumSvcRequest(CommonModuleProviderIf* provider,
                     const SvcRequestId& id,
                     const fpi::SvcUuid &myEpId,
                     fds_uint64_t const dlt_ver,
                     const std::vector<fpi::SvcUuid>& peerEpIds);

    QuorumSvcRequest(CommonModuleProviderIf* provider,
                     const SvcRequestId& id,
                     const fpi::SvcUuid &myEpId,
                     fds_uint64_t const dlt_ver,
                     const EpIdProviderPtr epProvider);

    ~QuorumSvcRequest();

    virtual void invoke() override;

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;

    virtual std::string logString() override;

    void setQuorumCnt(const uint32_t cnt);

    void setWaitForAllResponses(bool flag);

    void onResponseCb(QuorumSvcRequestRespCb cb);

 protected:
    virtual void invokeWork_() override;

    uint32_t successAckd_;
    uint32_t errorAckd_;
    uint32_t quorumCnt_;
    bool waitForAllResponses_;
    QuorumSvcRequestRespCb respCb_;
};
typedef boost::shared_ptr<QuorumSvcRequest> QuorumSvcRequestPtr;

/**
* @brief Use this request to manage messaging to few primaries and few optionals in a consistency
* group.
* Provided callback hooks
* -onPrimariesResponsdedCb(cb) - Here cb is invoked when responses from all primaries have been received.
* -onAllRespondedCb(cb) - Here cb is invoked when responses from all services have been received.

*/
struct MultiPrimarySvcRequest : MultiEpSvcRequest {
    using MultiEpSvcRequest::MultiEpSvcRequest;
    MultiPrimarySvcRequest(CommonModuleProviderIf* provider,
                           const SvcRequestId& id,
                           const fpi::SvcUuid &myEpId,
                           fds_uint64_t const dlt_version,
                           const std::vector<fpi::SvcUuid>& primarySvcs,
                           const std::vector<fpi::SvcUuid>& optionalSvcs);
    void invoke() override;
    /**
     * @brief Handling response.  This call is expected to be called in a synchnorized manner.
     * In response handling, once responses from all primaries have been received then respCb_
     * is invoked.
     * Once responses from all endpoints including optionals have been receieved then
     * allRespondedCb_ is invoked.
     *
     * @param header
     * @param payload
     */
    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload) override;
    std::string logString() override;
    void onPrimariesRespondedCb(MultiPrimarySvcRequestRespCb cb) {
        respCb_ = cb;
    }
    void onAllRespondedCb(MultiPrimarySvcRequestRespCb cb) {
        allRespondedCb_ = cb;
    }
    const std::vector<EPSvcRequestPtr>& getFailedPrimaries() const {
        return failedPrimaries_;
    }
    const std::vector<EPSvcRequestPtr>& getFailedOptionals() const {
        return failedOptionals_;
    }

 protected:
    /**
     * @brief Inovcation work function
     * NOTE this function is exectued on SvcMgr::taskExecutor_ for synchronization
     */
    virtual void invokeWork_() override;
    /**
     * @brief Returns endpoint request identified by peerEpId
     *
     * @param peerEpId
     * @param isPrimary - return true if peerEpId is also primary
     *
     * @return 
     */
    EPSvcRequestPtr getEpReq_(const fpi::SvcUuid &peerEpId, bool &isPrimary);

    /* Primary acks received */
    uint8_t                         primaryAckdCnt_;
    /* Total acks received */
    uint8_t                         totalAckdCnt_;
    /* # of primary services */
    uint8_t                         primariesCnt_;
    /* Primaries that have failed */
    std::vector<EPSvcRequestPtr>    failedPrimaries_;
    /* Optionals that have failed */
    std::vector<EPSvcRequestPtr>    failedOptionals_;
    /* Invoked once responses from all primaries have been received */
    MultiPrimarySvcRequestRespCb    respCb_; 
    /* Invoked once response from all endpoints is received */
    MultiPrimarySvcRequestRespCb    allRespondedCb_;
};
using MultiPrimarySvcRequestPtr = boost::shared_ptr<MultiPrimarySvcRequest>;

/**
* @brief Use an object of this class to wait on MultiPrimarySvcRequest.  It is helpful
* for making async MultiPrimarySvcRequest into a sync call.
*/
struct MultiPrimarySvcRequestCbTask : concurrency::TaskStatus {
    MultiPrimarySvcRequestCbTask() {
        error = ERR_INVALID;
        cb = std::bind(&MultiPrimarySvcRequestCbTask::allRespondedCb, this,
			std::placeholders::_1,  // NOLINT
			std::placeholders::_2,
            std::placeholders::_3);  // NOLINT
    }
    void allRespondedCb(MultiPrimarySvcRequest *req, const Error &e, StringPtr payload) {
        error = e;
        done();
    }
    bool success() {
        return error == ERR_OK;
    }
    void reset()
    {
        error = ERR_INVALID;
        concurrency::TaskStatus::reset(1);
    }

    Error                           error;
    MultiPrimarySvcRequestRespCb    cb;
};

/**
* @brief Wrapper for retrying SvcRequests.
* Currently supports retrying EPSvcRequest.  In theory any request with respCb_ as a member
* with type of SvcRequestRetrier::ResponseCb should work.
*
* @tparam ReqT
*/
template<class ReqT>
struct SvcRequestRetrier : TrackableRequest {
    using NewSvcRequestFunc = std::function<SHPTR<ReqT> ()>;
    using RetryPredicate = std::function<bool(SvcRequestRetrier*, const Error&)>; 
    using ResponseCb = std::function<void (ReqT*,
                                           const Error&,
                                           SHPTR<std::string>)>;
    SvcRequestRetrier(CommonModuleProviderIf* provider,
                      const NewSvcRequestFunc &newSvcReqFunc,
                      const RetryPredicate &pred)
    {
        newSvcReqFunc_ = newSvcReqFunc;
        retryPredicate_ = pred;
        retriedCnt_ = 0;
        backOffMs_ = 30;
    }
    void responseCb(ReqT* r, const Error& e, boost::shared_ptr<std::string> payload) {
        if (e == ERR_OK) {
            respCb_(r, e, payload);
            complete(ERR_OK);
        } else {
            bool retry = retryPredicate_(this, e);
            if (retry) {
                scheduleRetry();
            } else {
                complete(e);
            }
        }
    }
    void invoke()
    {
        auto req = newSvcReqFunc_();
        fds_assert(!req->respCb_);
        fds_assert(req->isTracked());
        req->responseCb(std::bind(&responseCb, this));
        req->invoke();
    }
    void scheduleRetry()
    {
        auto timer = MODULEPROVIDER()->getTimer();
        timer->schedule(std::chrono::milliseconds(backOffMs_ << retriedCnt_),
                        [this]() {
                            retriedCnt_++;
                            /* NOTE: For request types that don't synchronize inside will not
                             * work with the following invocation
                             */
                            invoke();
                        });
    }
    void onResponseCb(const ResponseCb &respCb)
    {
        respCb_ = respCb;
    }
    void withBackOff(int32_t backOffMs)
    {
        backOffMs_ = backOffMs;
    }
    inline int32_t getRetriedCnt() const { return retriedCnt_; }
 protected:
    NewSvcRequestFunc       newSvcReqFunc_;
    RetryPredicate          retryPredicate_;
    ResponseCb              respCb_;
    int32_t                 retriedCnt_;
    int32_t                 backOffMs_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCREQUEST_H_
