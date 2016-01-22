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
    LOGDEBUG << "received message to process snaps for volume : " << task.volumeId;
    std::vector<fpi::Snapshot> vecSnapshots;
    uint64_t nextTime = 0, deleteTime, currentTime = fds::util::getTimeStampSeconds();
    bool fAdded = false;
    // get the snapshots for this volume
    om->getConfigDB()->listSnapshots(vecSnapshots, task.volumeId);

    // for each snapshot , check which needs to be deleted
    for (const auto& snapshot : vecSnapshots) {
        deleteTime = snapshot.retentionTimeSeconds + snapshot.creationTimestamp/1000;
        if (snapshot.retentionTimeSeconds <= 0) {
            LOGDEBUG << "snapshot will be retained forever : " << snapshot.snapshotName;
            continue;
        }

        bool fNeedsTimeCheck = false;
        bool fNeedsDeleteCheck = false;
        switch (snapshot.state) {
            case fpi::ResourceState::Unknown:
            case fpi::ResourceState::Loading:
                LOGDEBUG << "snapshot not in a state for delete : " << snapshot.snapshotName;
                fNeedsTimeCheck = true;
                break;

            case fpi::ResourceState::MarkedForDeletion:
                LOGDEBUG << "snapshot already marked for delete : " << snapshot.snapshotName;
                break;
            case fpi::ResourceState::Deleted:
                LOGDEBUG << "snapshot already deleted : " << snapshot.snapshotName;
                break;
            case fpi::ResourceState::InError:
                LOGWARN << "snapshot in Error : " << snapshot.snapshotName;
                break;
            case fpi::ResourceState::Offline:
            case fpi::ResourceState::Created:
            case fpi::ResourceState::Active:
                fNeedsTimeCheck = true;
                fNeedsDeleteCheck= true;
                break;
            case fpi::ResourceState::Syncing:
                // Fall through
            default:
                fds_assert(!"Not handled");
        }

        if (fNeedsTimeCheck && (nextTime == 0 || deleteTime < nextTime)) {
            nextTime = deleteTime;
        }

        if (fNeedsDeleteCheck && (deleteTime <= currentTime)) {
            LOGDEBUG << "snapshot will be deleted : "
                     << snapshot.snapshotId << ":" << snapshot.snapshotName;
            atc::Synchronized s(monitor);
            snapshotQ.push(snapshot);
            fAdded = true;
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
        volContainer->om_delete_vol(fds_volid_t(snapshot.snapshotId));
    }
}

}  // namespace snapshot
}  // namespace fds

