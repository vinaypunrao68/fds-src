/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <AsyncRpcRequestTracker.h>
namespace fds {

/**
 * Constructor
 */
AsyncRpcRequestTracker::AsyncRpcRequestTracker()
{
}

/**
 * Destructor
 */
AsyncRpcRequestTracker::~AsyncRpcRequestTracker()
{
}

/**
 * Add the request for tracking
 * @param req
 */
void AsyncRpcRequestTracker::addForTracking(AsyncRpcRequestPtr req)
{
    fds_scoped_lock l(lock_);
    requestMap_[req->getRequestId()] = req;
}

/**
 * Add the request for tracking
 * @param req
 */
void AsyncRpcRequestTracker::removeFromTracking(AsyncRpcRequestPtr req)
{
    fds_scoped_lock l(lock_);
    requestMap_.erase(req->getRequestId());
}

}  // namespace fds
