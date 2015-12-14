/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_COUNTERS_H_
#define SOURCE_STOR_MGR_INCLUDE_COUNTERS_H_

#include <fds_counters.h>
#include <map>
namespace fds {
namespace sm {
struct Counters : FdsCounters {
    explicit Counters(FdsCountersMgr *mgr);
    ~Counters() = default;
    void clear();
    void reset() {}
    void setScavengeInfo(fds_token_id token, uint64_t numObjects, uint64_t numMarkedForDeletion);

    SimpleNumericCounter scavengerRunning;
    SimpleNumericCounter scavengerStartedAt;
    SimpleNumericCounter compactorRunning;
    SimpleNumericCounter dataRemoved;
    SimpleNumericCounter dataCopied;
  protected:
    std::map<fds_token_id, std::pair<SimpleNumericCounter* ,SimpleNumericCounter* > > scanvengedTokens;
};
}  // namespace sm
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_COUNTERS_H_
