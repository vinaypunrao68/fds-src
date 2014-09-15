/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
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
    uint64_t deleteTime  = snapshot.creationTimestamp/1000 + snapshot.retentionTimeSeconds;
    auto handleptr = handleMap.find(snapshot.volumeId);
    if (handleptr != handleMap.end()) {
        // volume already exists
        LOGDEBUG << "volume already exists : " << snapshot.volumeId;
        DeleteTask* task = handleptr->second->node_->value;
        if (task->runAtTime > deleteTime) {
            LOGDEBUG << "updating runAt for vol:" << snapshot.volumeId
                     << " @ " << deleteTime;
            task->runAtTime = deleteTime;
            pq.erase(*(handleptr->second));
            auto handle = pq.push(task);
            handleMap[snapshot.volumeId] = &handle;
        }
    } else {
        DeleteTask* task = new DeleteTask();
        task->volumeId = snapshot.volumeId;
        auto handle = pq.push(task);
        handleMap[snapshot.volumeId] = &handle;
        LOGDEBUG << "new volume:" << snapshot.volumeId << " @ " << deleteTime;
        fModified = true;
    }

    if (fModified) {
        ping();
    }
    return fModified;
}

bool DeleteScheduler::removeVolume(fds_volid_t volumeId) {
    atc::Synchronized s(monitor);
    auto handleptr = handleMap.find(volumeId);
    if (handleptr != handleMap.end()) {
        pq.erase(*(handleptr->second));
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
    LOGDEBUG << " --- delete scheduler queue ---";
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
        while (!fShutdown && pq.empty()) {
            LOGDEBUG << "delete q empty .. waiting.";
            monitor.waitForever();
        }

        while (!fShutdown && !pq.empty()) {
            DeleteTask* task;
            uint64_t currentTime = fds::util::getTimeStampSeconds();
            task = pq.top();
            LOGDEBUG << "curTime:" << currentTime << " next:" << task->runAtTime;
            dump();
            if (task->runAtTime > currentTime) {
                // there is no task to be executed at the time
                LOGDEBUG << "going into wait ...";
                monitor.waitForTimeRelative((task->runAtTime - currentTime)*1000);  // ms
            } else {
                // to be executed now ..
                LOGDEBUG << "processing volumeid:" << task->volumeId;
                uint64_t nextTime = taskProcessor->process(*task);

                // now check the next time & reschedule the task
                auto handleptr = handleMap.find(task->volumeId);
                if (handleptr == handleMap.end()) {
                    LOGERROR << "major error . volumeid is missing from the map : "
                             << task->volumeId;
                }

                if (nextTime > currentTime) {
                    task->runAtTime = nextTime;
                    LOGDEBUG << "rescheduling volume:" << task->volumeId
                             << " @ " << task->runAtTime;
                    pq.pop();
                    auto handle = pq.push(task);
                    handleMap[task->volumeId] = &handle;
                    // pq.update(*(handleptr->second), task);
                } else {
                    LOGWARN << "no more recurrence of this volume: " << task->volumeId;
                    pq.pop();
                    handleMap.erase(handleptr);
                }
            }
        }
    }  // !fShutdown
}

}  // namespace snapshot
}  // namespace fds
