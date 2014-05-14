/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <net/AsyncRpcRequestTracker.h>
#include <net/net-service.h>

namespace fds {

AsyncRpcRequestTracker* gAsyncRpcTracker = new AsyncRpcRequestTracker();

/**
 * Add the request for tracking
 * @param id
 * @param req
 */
bool AsyncRpcRequestTracker::addForTracking(const AsyncRpcRequestId& id,
        AsyncRpcRequestIfPtr req)
{
    fds_scoped_lock l(asyncReqMaplock_);
    if (asyncReqMap_.find(id) == asyncReqMap_.end()) {
        asyncReqMap_[id] = req;
        return true;
    }
    return false;
}

/**
 * Remove the request from tracking
 * @param id
 */
bool AsyncRpcRequestTracker::removeFromTracking(const AsyncRpcRequestId& id)
{
    fds_scoped_lock l(asyncReqMaplock_);
    auto itr = asyncReqMap_.find(id);
    if (itr != asyncReqMap_.end()) {
        asyncReqMap_.erase(itr);
        return true;
    }
    return false;
}

/**
 * Returns rpc request identified by id
 * @param id
 * @return
 */
AsyncRpcRequestIfPtr
AsyncRpcRequestTracker::getAsyncRpcRequest(const AsyncRpcRequestId& id)
{
    fds_scoped_lock l(asyncReqMaplock_);
    auto itr = asyncReqMap_.find(id);
    if (itr != asyncReqMap_.end()) {
        return itr->second;
    }
    return nullptr;
}
}  // namespace fds
