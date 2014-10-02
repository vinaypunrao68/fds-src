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
    snapPolicyDispatcher = new PolicyDispatcher(om);
    snapScheduler = new Scheduler(om, snapPolicyDispatcher);
    deleteDispatcher = new DeleteDispatcher(om);
    deleteScheduler = new DeleteScheduler(om, deleteDispatcher);
}

bool Manager::loadFromConfigDB() {
    std::vector<fpi::SnapshotPolicy> vecSnapshotPolicies;
    om->getConfigDB()->listSnapshotPolicies(vecSnapshotPolicies);
    LOGNORMAL << "loaded [" << vecSnapshotPolicies.size() << "] policies";
    for (auto policy : vecSnapshotPolicies) {
        snapScheduler->addPolicy(policy);
    }
    std::vector<fds_volid_t> vecVolumeIds;
    om->getConfigDB()->getVolumeIds(vecVolumeIds);
    std::vector<fpi::Snapshot> vecSnapshots;

    LOGNORMAL << "processing snapshots for delete q from ["
              << vecVolumeIds.size()
              << "] volumes";

    for (const auto& volumeId : vecVolumeIds) {
        vecSnapshots.clear();
        om->getConfigDB()->listSnapshots(vecSnapshots, volumeId);
        LOGDEBUG << "processing [" << vecSnapshots.size()
                 << "] snapshots for vol:" << volumeId;
        for (const auto& snapshot : vecSnapshots) {
            deleteScheduler->addSnapshot(snapshot);
        }
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
