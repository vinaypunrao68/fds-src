/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ASYNC_RPC_REQUEST_TRACKER_H_
#define SOURCE_INCLUDE_ASYNC_RPC_REQUEST_TRACKER_H_

#include <boost/shared_ptr.hpp>
#include <concurrency/Mutex.h>
#include <unordered_map>


namespace fds {

/* Forward declarations */
class AsyncRpcRequest;
typedef boost::shared_ptr<AsyncRpcRequest> AsyncRpcRequestPtr;
typedef uint64_t AsyncRpcRequestId;

/**
 * For tracking requests
 */
class AsyncRpcRequestTracker
{
public:
    AsyncRpcRequestTracker();
    virtual ~AsyncRpcRequestTracker();

    void addForTracking(AsyncRpcRequestPtr req);
    void removeFromTracking(AsyncRpcRequestPtr req);

protected:
    fds_spinlock lock_;
    std::unorderd_map<AsyncRpcRequestId, AsyncRpcRequestPtr> requestMap_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_ASYNC_RPC_REQUEST_TRACKER_H_
