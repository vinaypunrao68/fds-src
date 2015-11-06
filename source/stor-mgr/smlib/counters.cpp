/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <counters.h>
namespace fds { namespace sm {
Counters::Counters(FdsCountersMgr *mgr) : FdsCounters("sm", mgr),
                                          scavengerRunning("sm.scavenger.running", this),
                                          scavengerStartedAt("sm.scavenger.start.timestamp", this),
                                          compactorRunning("sm.scavenger.compactor.running", this) {
    for (auto i = 0; i < 256 ; i++) {
        scanvengedTokens.insert(std::make_pair<fds_token_id, std::pair<SimpleNumericCounter* ,SimpleNumericCounter* > >(
            i, std::make_pair<SimpleNumericCounter* ,SimpleNumericCounter*>(
                new SimpleNumericCounter("sm.scavenger.token.total", fds_volid_t(i), this),
                new SimpleNumericCounter("sm.scavenger.token.deleted", fds_volid_t(i), this))));
    }
}

void Counters::clear() {
    for (auto& item : scanvengedTokens) {
        item.second.first->set(0);
        item.second.second->set(0);
    }
}

void Counters::setScavengeInfo(fds_token_id token, uint64_t numObjects, uint64_t numMarkedForDeletion) {
    scanvengedTokens[token].first->set(numObjects);
    scanvengedTokens[token].second->set(numMarkedForDeletion);
}

}  // namespace sm
}  // namespace fds
