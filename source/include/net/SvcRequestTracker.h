/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCREQUESTTRACKER_H_
#define SOURCE_INCLUDE_NET_SVCREQUESTTRACKER_H_

#include <unordered_map>
#include <string>

#include <concurrency/Mutex.h>
#include <fds_counters.h>
#include <net/SvcRequest.h>

namespace fds {

/* Forward declarations */
struct CommonModuleProviderIf;

/**
 * Tracker svc requests.  svc requests are tracked by their id
 */
class SvcRequestTracker : HasModuleProvider {
 public:
    explicit SvcRequestTracker(CommonModuleProviderIf *moduleProvider);
    bool addForTracking(const SvcRequestId& id, SvcRequestIfPtr req);
    SvcRequestIfPtr popFromTracking(const SvcRequestId& id);
    SvcRequestIfPtr getSvcRequest(const SvcRequestId &id);
    uint64_t getOutstandingSvcReqsCount();

 protected:
    fds_mutex svcReqMaplock_;
    std::unordered_map<SvcRequestId, SvcRequestIfPtr> svcReqMap_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCREQUESTTRACKER_H_
