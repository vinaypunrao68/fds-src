/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <net/SvcRequestTracker.h>
#include <net/net-service.h>
#include <util/Log.h>
#include <fdsp_utils.h>

namespace fds {


/**
 * Add the request for tracking
 * @param id
 * @param req
 */
bool SvcRequestTracker::addForTracking(const SvcRequestId& id,
        SvcRequestIfPtr req)
{
    DBG(GLOGDEBUG << req->logString());

    SVCPERF(req->ts.rqStartTs = util::getTimeStampNanos());

    fds_scoped_lock l(svcReqMaplock_);
    auto pair = std::make_pair(id, req);
    auto ret = svcReqMap_.insert(pair);
    return ret.second;
}

/**
 * Remove the request from tracking
 * @param id
 */
bool SvcRequestTracker::removeFromTracking(const SvcRequestId& id)
{
    DBG(GLOGDEBUG << "Req Id: " << id);

    fds_scoped_lock l(svcReqMaplock_);
    auto itr = svcReqMap_.find(id);
    if (itr != svcReqMap_.end()) {
        svcReqMap_.erase(itr);
        return true;
    }
    return false;
}

/**
 * Returns svc request identified by id
 * @param id
 * @return
 */
SvcRequestIfPtr
SvcRequestTracker::getSvcRequest(const SvcRequestId& id)
{
    fds_scoped_lock l(svcReqMaplock_);
    auto itr = svcReqMap_.find(id);
    if (itr != svcReqMap_.end()) {
        return itr->second;
    }
    return nullptr;
}
}  // namespace fds
