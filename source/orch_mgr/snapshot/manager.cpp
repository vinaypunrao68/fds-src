/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <orchMgr.h>

namespace fds {
namespace snapshot {

Manager::Manager(OrchMgr* om) : om(om), svcHandler(om) {
}

void Manager::init() {
    snapPolicyDispatcher = new fds::snapshot::PolicyDispatcher(om);
    snapScheduler = new fds::snapshot::Scheduler(om, snapPolicyDispatcher);
}

bool Manager::loadFromConfigDB() {
    std::vector<fpi::SnapshotPolicy> vecSnapshotPolicies;
    om->getConfigDB()->listSnapshotPolicies(vecSnapshotPolicies);
    LOGNORMAL << "loaded [" << vecSnapshotPolicies.size() << "] policies";
    for (auto policy : vecSnapshotPolicies) {
        snapScheduler->addPolicy(policy);
    }

    return true;
}

bool Manager::addPolicy(fpi::SnapshotPolicy& policy) {
    return snapScheduler->addPolicy(policy);
}

bool Manager::removePolicy(int64_t id) {
    return snapScheduler->removePolicy(id);
}

}  // namespace snapshot
}  // namespace fds
