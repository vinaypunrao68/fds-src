/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <map>
#include <snapshot/policydispatcher.h>
#include <OmResources.h>
#include <orchMgr.h>
#include <snapshot/svchandler.h>

#include <util/stringutils.h>
#include <util/timeutils.h>
#include <unistd.h>

namespace atc = apache::thrift::concurrency;
namespace fds { namespace snapshot {

PolicyDispatcher::PolicyDispatcher(OrchMgr* om) : om(om),
                                                  runner(&PolicyDispatcher::run, this) {
}

bool PolicyDispatcher::process(const Task& task) {
    LOGDEBUG << "received message to process task : " << task.policyId;
    atc::Synchronized s(monitor);
    policyq.push(task.policyId);
    monitor.notifyAll();
    return true;
}

uint PolicyDispatcher::process(const std::vector<Task*>& vecTasks) {
    LOGDEBUG << "received message to process [" << vecTasks.size() << "] tasks";
    atc::Synchronized s(monitor);
    for (const auto& task : vecTasks) {
        policyq.push(task->policyId);
    }
    monitor.notifyAll();
    return vecTasks.size();
}

void PolicyDispatcher::run() {
    uint64_t policyId = 0;
    // this is for coalescing multiple daily/weekly/monthly into one
    std::map<int64_t, fpi::Snapshot> mapVolSnaps;
    while (true) {
        // check if we need to process any snaps
        bool fShouldProcess = false;

        {
            // check if we need to process the selected volumes
            atc::Synchronized s(monitor);
            if ((policyq.empty() && !mapVolSnaps.empty()) || mapVolSnaps.size() > 1000) {
                fShouldProcess = true;
            }
        }

        if (fShouldProcess) {
            OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
            VolumeContainer::pointer volContainer = local->om_vol_mgr();
            LOGDEBUG << "snapshot requests will be dispatched for ["
                 << mapVolSnaps.size() << "] volumes";

            for (const auto& kv : mapVolSnaps) {
                // send the snapshot create request
                // NOTE : we are not sending the snapshot create request anymore
                // it will be handled by addVolume ...
                // om->snapshotMgr.svcHandler.omSnapshotCreate(snapshot)
                const fpi::Snapshot& snapshot = kv.second;
                fds::Error err = volContainer->addSnapshot(snapshot);
                if ( !err.ok() ) {
                    LOGWARN << "snapshot add failed : " << err;
                    continue;
                }
                // add this snapshot to the retention manager ...
                om->snapshotMgr->deleteScheduler->addSnapshot(snapshot);
            }

            // clear the map
            mapVolSnaps.clear();
        }

        // this block is needed because we need to hold the lock only for a short time
        {
            atc::Synchronized s(monitor);
            while (policyq.empty()) {
                monitor.waitForever();
            }
            LOGDEBUG << "q-size: " << policyq.size();
            policyId = policyq.front();
            policyq.pop();
        }

        // find all the volumes that match and add it to map
        std::vector<int64_t> vecVolumes;
        om->getConfigDB()->listVolumesForSnapshotPolicy(vecVolumes, policyId);

        fds::apis::SnapshotPolicy policy;
        om->getConfigDB()->getSnapshotPolicy(policyId, policy);

        VolumeDesc volumeDesc("", fds_volid_t(1));
        for (auto volId_ : vecVolumes) {
            fds_volid_t volId(volId_);
            // check if we already have a snap scheduled with the volId
            auto iter = mapVolSnaps.find(volId.get());
            if (iter != mapVolSnaps.end() &&
                iter->second.retentionTimeSeconds > policy.retentionTimeSeconds) {
                // Now there is a snap already scheduled which has higher retention
                // so skip it ..
                LOGWARN << "skipping [" << policy.policyName
                        << "] snapshot for [vol:" << volId << "]"
                        << " overridden by : " << iter->second.snapshotName;
                continue;
            }

            if (!om->getConfigDB()->getVolume(volId, volumeDesc)) {
                LOGWARN << "unable to get infor for vol: " << volId;
                continue;
            }

            if (!volumeDesc.isStateActive()) {
                LOGWARN << "skipping non-active volume : " << volId
                        << " : " << fpi::_ResourceState_VALUES_TO_NAMES.find(volumeDesc.getState())->second; //NOLINT
            }

            // create the structure
            fpi::Snapshot snapshot;
            snapshot.snapshotName = util::strlower(
                util::strformat("snap.%s.%s.%ld",
                                volumeDesc.name.c_str(),
                                policy.policyName.c_str(),
                                util::getTimeStampSeconds()));
            snapshot.volumeId = volId.get();
            auto snapshotId = om->getConfigDB()->getNewVolumeId();
            if (invalid_vol_id == snapshotId) {
                LOGWARN << "unable to generate a new snapshot id";
                continue;
            }
            snapshot.snapshotId = snapshotId.get();

            snapshot.snapshotPolicyId = policyId;
            snapshot.creationTimestamp = util::getTimeStampSeconds();
            snapshot.retentionTimeSeconds = policy.retentionTimeSeconds;
            // activate snap right away.
            snapshot.state = fpi::ResourceState::Loading;
            LOGDEBUG << "snapshot request for volumeid:" << volId
                     << " name:" << snapshot.snapshotName;

            // Add it to the map
            mapVolSnaps[volId.get()] = snapshot;
        }
    }
}

}  // namespace snapshot
}  // namespace fds

