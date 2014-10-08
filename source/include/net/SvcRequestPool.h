/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCREQUESTPOOL_H_
#define SOURCE_INCLUDE_NET_SVCREQUESTPOOL_H_

#include <vector>
#include <string>

#include <concurrency/Mutex.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestTracker.h>
#include <platform/platform-lib.h>

namespace fds {

/**
 * @brief Svc request counters
 */
class SvcRequestCounters : public FdsCounters
{
 public:
    SvcRequestCounters(const std::string &id, FdsCountersMgr *mgr);
    ~SvcRequestCounters();

    /* Number of requests that have timedout */
    NumericCounter timedout;
    /* Number of requests that experienced transport error */
    NumericCounter invokeerrors;
    /* Number of responses that resulted in app acceptance */
    NumericCounter appsuccess;
    /* Number of responses that resulted in app rejections */
    NumericCounter apperrors;
};

/**
 * Svc request factory. Use this class for constructing various Svc request objects
 */
class SvcRequestPool {
 public:
    SvcRequestPool();
    ~SvcRequestPool();

    EPSvcRequestPtr newEPSvcRequest(const fpi::SvcUuid &peerEpId, int minor_version);
    inline EPSvcRequestPtr newEPSvcRequest(const fpi::SvcUuid &peerEpId) {
        extern const NodeUuid gl_OmUuid;
        if (peerEpId.svc_uuid == (int64_t)gl_OmUuid.uuid_get_val()) {
            //  this is hack as doing incremental work with both old and new channels
            //  retrace back when finish the work
            //  The hard-coding version is at net-platform.cpp:317
            //  XXX todo(liwu)
            return newEPSvcRequest(peerEpId, NET_SVC_CTRL);
        } else {
            return newEPSvcRequest(peerEpId, 0);
        }
    }
    FailoverSvcRequestPtr newFailoverSvcRequest(const EpIdProviderPtr epProvider);
    QuorumSvcRequestPtr newQuorumSvcRequest(const EpIdProviderPtr epProvider);

    void postError(boost::shared_ptr<fpi::AsyncHdr> &header);

    static fpi::AsyncHdr newSvcRequestHeader(const SvcRequestId& reqId,
                                             const fpi::FDSPMsgTypeId &msgTypeId,
                                             const fpi::SvcUuid &srcUuid,
                                             const fpi::SvcUuid &dstUuid);
    static boost::shared_ptr<fpi::AsyncHdr> newSvcRequestHeaderPtr(
            const SvcRequestId& reqId,
            const fpi::FDSPMsgTypeId &msgTypeId,
            const fpi::SvcUuid &srcUuid,
            const fpi::SvcUuid &dstUuid);

 protected:
    void asyncSvcRequestInitCommon_(SvcRequestIfPtr req);

    std::atomic<uint64_t> nextAsyncReqId_;
    /* Common completion callback for svc requests */
    SvcRequestCompletionCb finishTrackingCb_;
};

extern SvcRequestCounters* gSvcRequestCntrs;
extern SvcRequestPool *gSvcRequestPool;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCREQUESTPOOL_H_
