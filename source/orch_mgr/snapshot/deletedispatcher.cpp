/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>

#include <snapshot/deletedispatcher.h>
#include <OmResources.h>
#include <orchMgr.h>
#include <snapshot/svchandler.h>

#include <util/stringutils.h>
#include <util/timeutils.h>
#include <unistd.h>

namespace atc = apache::thrift::concurrency;
namespace fds { namespace snapshot {

DeleteDispatcher::DeleteDispatcher(OrchMgr* om) : om(om),
                                                  runner(&DeleteDispatcher::run, this) {
}

uint64_t DeleteDispatcher::process(const DeleteTask& task) {
    LOGDEBUG << "received message to process volume : " << task.volumeId;
    std::vector<fpi::Snapshot> vecSnapshots;
    uint64_t nextTime = 0, deleteTime, currentTime = fds::util::getTimeStampSeconds();
    bool fAdded = false;
    // get the snapshots for this volume
    om->getConfigDB()->listSnapshots(vecSnapshots, task.volumeId);

    // for each snapshot , check which needs to be deleted
    for (const auto& snapshot : vecSnapshots) {
        if (snapshot.retentionTimeSeconds <= 0 || snapshot.state != fpi::ResourceState::Active) {
            continue;
        }
        deleteTime = snapshot.retentionTimeSeconds + snapshot.creationTimestamp/1000;
        if (deleteTime <= currentTime) {
            LOGDEBUG << "snapshot needs to be deleted : " << snapshot.snapshotId;
            atc::Synchronized s(monitor);
            snapshotQ.push(snapshot);
            fAdded = true;
        } else {
            // set the next time at which snapshots should be deleted
            if (nextTime == 0 || deleteTime < nextTime) {
                nextTime = deleteTime;
            }
        }
    }
    if (fAdded) {
        atc::Synchronized s(monitor);
        monitor.notifyAll();
    }
    return nextTime;
}

void DeleteDispatcher::run() {
    fpi::Snapshot snapshot;

    while (true) {
        // this block is needed because we need to hold the lock only for a short time
        {
            atc::Synchronized s(monitor);
            while (snapshotQ.empty()) {
                monitor.waitForever();
            }
            snapshot = snapshotQ.front();
            snapshotQ.pop();
        }

        LOGDEBUG << "snapshot to be deleted : " << snapshot.snapshotName;
        snapshot.state = fpi::ResourceState::MarkedForDeletion;
        // mark the snapshot for deletion
        om->getConfigDB()->updateSnapshot(snapshot);

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        volContainer->om_delete_vol(snapshot.snapshotId);
    }
}

}  // namespace snapshot
}  // namespace fds

