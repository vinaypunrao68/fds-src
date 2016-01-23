/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCREQUESTPOOL_H_
#define SOURCE_INCLUDE_NET_SVCREQUESTPOOL_H_

#include <vector>
#include <string>

#include <concurrency/Mutex.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestTracker.h>
#include <concurrency/LFThreadpool.h>

namespace FDS_ProtocolInterface {
class SvcUuid;
}
namespace fpi = FDS_ProtocolInterface;

#include "EclipseWorkarounds.h"

namespace fds {

/* Forward declarations */
struct CommonModuleProviderIf;
class PlatNetSvcHandler;
using PlatNetSvcHandlerPtr = boost::shared_ptr<PlatNetSvcHandler>;

/**
 * Svc request factory. Use this class for constructing various Svc request objects
 */
class SvcRequestPool : HasModuleProvider {
 public:
    SvcRequestPool(CommonModuleProviderIf *moduleProvider,
                   const fpi::SvcUuid &selfUuid,
                   PlatNetSvcHandlerPtr handler);
    ~SvcRequestPool();

    template<typename RequestT, typename ...Params>
    SHPTR<RequestT> newSvcRequest(Params&&... params) {
        auto reqId = getNextAsyncReqId_();
        auto req = SHPTR<RequestT>(new RequestT(MODULEPROVIDER(), reqId, selfUuid_,
                                                std::forward<Params>(params)...));
        req->setRequestId(reqId);
        asyncSvcRequestInitCommon_(req);
        return req;
    }

    EPSvcRequestPtr newEPSvcRequest(const fpi::SvcUuid &peerEpId, int minor_version);
    inline EPSvcRequestPtr newEPSvcRequest(const fpi::SvcUuid &peerEpId) {
        return newEPSvcRequest(peerEpId, 0);
    }
    FailoverSvcRequestPtr newFailoverSvcRequest(const EpIdProviderPtr epProvider, fds_uint64_t const dlt_version = DLT_VER_INVALID);
    QuorumSvcRequestPtr newQuorumSvcRequest(const EpIdProviderPtr epProvider, fds_uint64_t const dlt_version = DLT_VER_INVALID);
    MultiPrimarySvcRequestPtr newMultiPrimarySvcRequest(
        const std::vector<fpi::SvcUuid>& primarySvcs,
        const std::vector<fpi::SvcUuid>& optionalSvcs,
        fds_uint64_t const dlt_version = DLT_VER_INVALID);

    void postError(boost::shared_ptr<fpi::AsyncHdr> &header);
    void postEmptyResponse(boost::shared_ptr<fpi::AsyncHdr> &header);

    fpi::AsyncHdr newSvcRequestHeader(const SvcRequestId& reqId,
                                      const fpi::FDSPMsgTypeId &msgTypeId,
                                      const fpi::SvcUuid &srcUuid,
                                      const fpi::SvcUuid &dstUuid,
                                      const fds_uint64_t dlt_version,
                                      const fpi::ReplicaId &replicaId,
                                      const int32_t &replicaVersion);
    boost::shared_ptr<fpi::AsyncHdr> newSvcRequestHeaderPtr(
        const SvcRequestId& reqId,
        const fpi::FDSPMsgTypeId &msgTypeId,
        const fpi::SvcUuid &srcUuid,
        const fpi::SvcUuid &dstUuid,
        fds_uint64_t const dlt_version,
        const fpi::ReplicaId &replicaId,
        const int32_t &replicaVersion);

    LFMQThreadpool* getSvcSendThreadpool();
    LFMQThreadpool* getSvcWorkerThreadpool();
    void dumpLFTPStats();

    SvcRequestCounters* getSvcRequestCntrs() const;
    SvcRequestTracker* getSvcRequestTracker() const;
    /// Sets a DLT manager with the pool so that it
    /// be used to set DLT versions on created headers.
    /// If it's not set, the version will default to invalid.
    void setDltManager(DLTManagerPtr dltManager);

    /* For debug cli */
    uint64_t getOutstandingRequestsCount();

    static fpi::AsyncHdr swapSvcReqHeader(const fpi::AsyncHdr &reqHdr);
    static SvcRequestId SVC_UNTRACKED_REQ_ID;

 protected:
    inline SvcRequestId getNextAsyncReqId_() {
        SvcRequestId id =  ++nextAsyncReqId_;
        /* Ensure nextAsyncReqId_ isn't SVC_UNTRACKED_REQ_ID */
        while (id == SVC_UNTRACKED_REQ_ID) {
            id =  ++nextAsyncReqId_;
        }
        return id;
    }

    void asyncSvcRequestInitCommon_(SvcRequestIfPtr req);

    template<typename T>
    T get_config(std::string const& option);

    /* align it to 64, so atomic doesn't share with cacheline with other
     * vars.  This is to prevent false-sharing and cache ping-pong.
     */
    alignas(64) std::atomic<SvcRequestId> nextAsyncReqId_;
    /* Common completion callback for svc requests */
    SvcRequestCompletionCb finishTrackingCb_;
    /* Lock free threadpool on which svc requests are sent */
    std::unique_ptr<LFMQThreadpool> svcSendTp_;
    /* Lock free threadpool on which work is done */
    std::unique_ptr<LFMQThreadpool> svcWorkerTp_;

    /* Cached self uuid */
    fpi::SvcUuid selfUuid_;
    /* Timeout */
    uint32_t reqTimeout_;
    /* Request tracker */
    SvcRequestTracker *svcRequestTracker_;
    /* Request counters */
    SvcRequestCounters *svcRequestCntrs_;
    /* Svc request handler */
    PlatNetSvcHandlerPtr svcReqHandler_;
    /* DLT manager to use for setting/checking request routing */
    DLTManagerPtr dltMgr;
};
extern SvcRequestPool *gSvcRequestPool;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCREQUESTPOOL_H_
