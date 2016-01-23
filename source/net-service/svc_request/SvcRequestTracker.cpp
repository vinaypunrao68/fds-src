/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <fds_module_provider.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcRequest.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestTracker.h>

namespace fds {

SvcRequestTracker::SvcRequestTracker(CommonModuleProviderIf *moduleProvider)
    : HasModuleProvider(moduleProvider)
{
}

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
 * Pop the request from tracking
 * @param id
 */
SvcRequestIfPtr
SvcRequestTracker::popFromTracking(const SvcRequestId& id)
{
    DBG(GLOGDEBUG << "Req Id: " << id);

    fds_scoped_lock l(svcReqMaplock_);
    auto itr = svcReqMap_.find(id);
    if (itr != svcReqMap_.end()) {
        auto r = itr->second;
        svcReqMap_.erase(itr);
        return r;
    }
    return nullptr;
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

/**
 * Return number of outstanding requests for cli debug
 */
uint64_t
SvcRequestTracker::getOutstandingSvcReqs() {
    return svcReqMap_.size();
}

}  // namespace fds
