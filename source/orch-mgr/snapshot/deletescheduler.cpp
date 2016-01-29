/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <orchMgr.h>
#include <snapshot/deletescheduler.h>
#include <util/timeutils.h>
namespace atc = apache::thrift::concurrency;
namespace fds { namespace snapshot {

DeleteScheduler::DeleteScheduler(OrchMgr* om, DeleteTaskProcessor* processor) {
    this->om = om;
    this->taskProcessor = processor;
    runner = new std::thread(&DeleteScheduler::run, this);
    LOGDEBUG << "scheduler instantiated";
}

DeleteScheduler::~DeleteScheduler() {
    shutdown();
    runner->join();
    delete runner;
}

// will also update / modify
bool DeleteScheduler::addSnapshot(const fpi::Snapshot& snapshot) {
    if (snapshot.retentionTimeSeconds <= 0) {
        LOGDEBUG << "nothing to do .. snapshot will be retained forever.";
        return true;
    }

    atc::Synchronized s(monitor);
    // check if the volume is already here
    bool fModified = false;
    uint64_t deleteTime  = snapshot.creationTimestamp + snapshot.retentionTimeSeconds;
    auto snapVolId = fds_volid_t(snapshot.volumeId);
    auto handleptr = handleMap.find(snapVolId);
    if (handleptr != handleMap.end()) {
        // volume already exists
        DeleteTask* task = *handleptr->second;
        if (task->runAtTime > deleteTime) {
            LOGDEBUG << "updating runAt for vol:" << snapVolId
                     << " @ " << fds::util::getLocalTimeString(deleteTime);
            task->runAtTime = deleteTime;
            pq.update(handleptr->second);
        }
    } else {
        DeleteTask* task = new DeleteTask();
        task->volumeId = snapVolId;
        task->runAtTime = deleteTime;
        handleMap[snapVolId] = pq.push(task);
        LOGDEBUG << "new volumeinfo :" << snapVolId
                 << " @ " << fds::util::getLocalTimeString(deleteTime);
        fModified = true;
    }

    if (fModified) {
        ping();
    }
    return fModified;
}

bool DeleteScheduler::removeVolume(fds_volid_t volumeId) {
    atc::Synchronized s(monitor);
    LOGDEBUG << "remove volume command rcvd..";
    auto handleptr = handleMap.find(volumeId);
    if (handleptr != handleMap.end()) {
        pq.erase(handleptr->second);
        handleMap.erase(handleptr);
        ping();
        LOGDEBUG << "removed volume from scheduler, id:" << volumeId;
        return true;
    } else {
        LOGWARN << "unable to locate volume in scheduler, vol:" << volumeId;
        return false;
    }
}

void DeleteScheduler::dump() {
    LOGDEBUG << " --- snapshot delete scheduler queue ---";
    for (PriorityQueue::ordered_iterator it = pq.ordered_begin(); it != pq.ordered_end(); ++it) {
        LOGDEBUG << *(*it) << ' ';
    }
}
void DeleteScheduler::shutdown() {
    atc::Synchronized s(monitor);
    fShutdown = true;
    monitor.notifyAll();
}

void DeleteScheduler::ping() {
    monitor.notifyAll();
}

void DeleteScheduler::run() {
    atc::Synchronized s(monitor);
    LOGNORMAL << "snapshot delete scheduler started";
    while (!fShutdown) {
        om->counters->numSnapshotsInDeleteQueue.set(pq.size());
        while (!fShutdown && pq.empty()) {
            LOGDEBUG << "delete q empty .. waiting.";
            monitor.waitForever();
        }

        while (!fShutdown && !pq.empty()) {
            om->counters->numSnapshotsInDeleteQueue.set(pq.size());
            DeleteTask* task;
            uint64_t currentTime = fds::util::getTimeStampSeconds();
            task = pq.top();
            LOGDEBUG << "curTime:" << fds::util::getLocalTimeString(currentTime)
                     << " next:" << fds::util::getLocalTimeString(task->runAtTime);
            dump();
            if (task->runAtTime > currentTime) {
                // there is no task to be executed at the time
                LOGDEBUG << "going into wait ...";
                monitor.waitForTimeRelative((task->runAtTime - currentTime)*1000);  // ms
            } else {
                // to be executed now
                uint64_t nextTime = taskProcessor->process(*task);
                LOGDEBUG << "processed volumeid:"
                         << task->volumeId
                         << " next @ " << fds::util::getLocalTimeString(nextTime)
                         << ":" << nextTime;
                // now check the next time & reschedule the task
                auto handleptr = handleMap.find(task->volumeId);
                if (handleptr == handleMap.end()) {
                    LOGERROR << "major error . volumeid is missing from the map : "
                             << task->volumeId;
                }

                if (nextTime > currentTime) {
                    task->runAtTime = nextTime;
                    LOGDEBUG << "rescheduling volume:" << task->volumeId
                             << " @ " << fds::util::getLocalTimeString(task->runAtTime);
                    pq.update(handleptr->second);
                } else {
                    LOGWARN << "no more snapshots to be monitored for volume: " << task->volumeId;
                    pq.pop();
                    handleMap.erase(handleptr);
                }
            }
        }
    }  // !fShutdown
}

}  // namespace snapshot
}  // namespace fds
