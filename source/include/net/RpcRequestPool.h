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

#if 0
    EPRpcRequestPtr newEPRpcRequest(const fpi::SvcUuid &uuid);
#endif
    EPAsyncRpcRequestPtr newEPAsyncRpcRequest(const fpi::SvcUuid &uuid);

    template <typename ServiceT, typename MemFuncT, typename Arg1T>
    FailoverRpcRequestPtr newFailoverRpcRequest(
            const std::vector<fpi::SvcUuid>& uuid_list, MemFuncT f, Arg1T a1)
    {
        auto reqId = nextAsyncReqId_++;

        FailoverRpcRequestPtr req(new FailoverRpcRequest(reqId, uuid_list));
        boost::shared_ptr<RpcFuncIf> rpc(new RpcFunc1<ServiceT, MemFuncT, Arg1T>(f, a1));

        req->setRpcFunc(rpc);
        gAsyncRpcTracker->addForTracking(reqId, req);

        return req;
    }

    static fpi::AsyncHdr newAsyncHeader(const AsyncRpcRequestId& reqId,
            const fpi::SvcUuid &dstUuid);

 protected:
    std::atomic<uint64_t> nextAsyncReqId_;
};

extern RpcRequestPool *gRpcRequestPool;
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCREQUESTPOOL_H_
