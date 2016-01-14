/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <OmResources.h>
#include <deletescheduler.h>
#include <util/timeutils.h>
namespace atc = apache::thrift::concurrency;
namespace fds {
using snapshot::DeleteTask;
DeleteScheduler::DeleteScheduler(OrchMgr* om) {
    this->om = om;
}

DeleteScheduler::~DeleteScheduler() {
    shutdown();
    runner->join();
    delete runner;
}

void DeleteScheduler::start() {
    this->runner = new std::thread(&DeleteScheduler::run, this);
    LOGDEBUG << "scheduler instantiated";
}

// will also update / modify
bool DeleteScheduler::scheduleVolume(fds_volid_t volumeId) {
    atc::Synchronized s(monitor);
    // check if the volume is already here
    bool fModified = false;
    const uint64_t TIMEDELAY = 300;  // 5*60;  // 5 mins
    uint64_t deleteTime  = fds::util::getTimeStampSeconds() + TIMEDELAY;
    auto handleptr = handleMap.find(volumeId);
    if (handleptr != handleMap.end()) {
        // volume already exists
        LOGWARN << "vol: " << volumeId
                << " already scheduled for delete";
    } else {
        DeleteTask* task = new DeleteTask();
        task->volumeId = volumeId;
        task->runAtTime = deleteTime;
        handleMap[volumeId] = pq.push(task);
        LOGDEBUG << "scheduling volume :" << volumeId
                 << " for delete @ " << fds::util::getLocalTimeString(deleteTime);
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
    LOGNORMAL << "delete scheduler started";
    while (!fShutdown) {
        while (!fShutdown && pq.empty()) {
            LOGDEBUG << "delete q empty .. waiting.";
            monitor.waitForever();
        }

        while (!fShutdown && !pq.empty()) {
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
                // trigger the state machine for task->volumeId

                OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
                VolumeContainer::pointer volContainer = local->om_vol_mgr();
                auto volId = task->volumeId.get();
                auto vol = VolumeInfo::vol_cast_ptr(volContainer->rs_get_resource(volId)); //NOLINT
                if (vol) {
                    LOGDEBUG << "resuming delete for vol:" << task->volumeId
                             << " name:" << vol->vol_get_name();
                    vol->vol_event(ResumeDelEvt(volId, vol.get()));
                } else {
                    LOGWARN << "unable to get volume info for vol:" << task->volumeId;
                }

                LOGDEBUG << "processed volumeid:" << task->volumeId;

                // now check the next time & reschedule the task
                auto handleptr = handleMap.find(task->volumeId);
                if (handleptr == handleMap.end()) {
                    LOGCRITICAL << "major error . volumeid is missing from the map : "
                                << task->volumeId;
                } else {
                    pq.pop();
                    handleMap.erase(handleptr);
                }
            }
        }
    }  // !fShutdown
}

}  // namespace fds
