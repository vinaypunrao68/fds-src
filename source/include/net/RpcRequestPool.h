/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_RPCREQUESTPOOL_H_
#define SOURCE_INCLUDE_NET_RPCREQUESTPOOL_H_

#include <vector>

#include <concurrency/Mutex.h>
#include <net/RpcRequest.h>
#include <net/AsyncRpcRequestTracker.h>

namespace fds {

/**
 * Rpc request factory. Use this class for constructing various RPC request objects
 */
class RpcRequestPool {
 public:
    RpcRequestPool();
    ~RpcRequestPool();

    EPAsyncRpcRequestPtr newEPAsyncRpcRequest(const fpi::SvcUuid &myEpId,
                                              const fpi::SvcUuid &peerEpId);

    FailoverRpcRequestPtr newFailoverRpcRequest(
        const fpi::SvcUuid& myEpId,
        const std::vector<fpi::SvcUuid>& peerEpIds);

    FailoverRpcRequestPtr newFailoverRpcRequest(const EpIdProviderPtr epProvider);

    static fpi::AsyncHdr newAsyncHeader(const AsyncRpcRequestId& reqId,
            const fpi::SvcUuid &srcUuid, const fpi::SvcUuid &dstUuid);

    static boost::shared_ptr<fpi::AsyncHdr> newAsyncHeaderPtr(
            const AsyncRpcRequestId& reqId,
            const fpi::SvcUuid &srcUuid,
            const fpi::SvcUuid &dstUuid);

 protected:
    void asyncRpcInitCommon_(AsyncRpcRequestIfPtr req);

    std::atomic<uint64_t> nextAsyncReqId_;
    /* Common completion callback for rpc requests */
    RpcRequestCompletionCb finishTrackingCb_;
};

extern RpcRequestPool *gRpcRequestPool;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCREQUESTPOOL_H_
