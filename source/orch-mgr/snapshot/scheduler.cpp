/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <orchMgr.h>
#include <snapshot/scheduler.h>
#include <util/timeutils.h>
#include <vector>
namespace atc = apache::thrift::concurrency;
namespace fds { namespace snapshot {

Scheduler::Scheduler(OrchMgr* om, TaskProcessor* processor) {
    this->om = om;
    this->taskProcessor = processor;
    runner = new std::thread(&Scheduler::run, this);
    LOGDEBUG << "scheduler instantiated";
}

Scheduler::~Scheduler() {
    shutdown();
    runner->join();
    delete runner;
}

// will also update / modify
bool Scheduler::addPolicy(const fds::apis::SnapshotPolicy& policy) {
    LOGDEBUG << "about to add policy : " << policy.id;
    atc::Synchronized s(monitor);
    // check if the policy is already here
    bool fModified = false;
    auto handleptr = handleMap.find(policy.id);
    if (handleptr != handleMap.end()) {
        // policy already exists
        LOGDEBUG << "policy already exists : " << policy.id;
        Task* task = *handleptr->second;
        if (task->setRecurrence(policy.recurrenceRule)) {
            LOGDEBUG << "updating policy:" << policy.id << " : " <<policy.recurrenceRule;
            pq.update(handleptr->second);
            fModified = true;
        }
    } else {
        Task* task = new Task();
        task->policyId = policy.id;
        task->setRecurrence(policy.recurrenceRule);
        handleMap[policy.id] = pq.push(task);
        LOGDEBUG << "new policy:" << policy.id << " : " << policy.recurrenceRule;
        fModified = true;
    }

    if (fModified) {
        ping();
    }
    return fModified;
}

bool Scheduler::removePolicy(uint64_t policyId) {
    atc::Synchronized s(monitor);
    auto handleptr = handleMap.find(policyId);
    if (handleptr != handleMap.end()) {
        pq.erase(handleptr->second);
        handleMap.erase(handleptr);
        ping();
        LOGDEBUG << "removed policy from scheduler, id:" <<policyId;
        return true;
    } else {
        LOGWARN << "unable to remove policy from scheduler, id:" <<policyId;
        return false;
    }
}

void Scheduler::dump() {
    LOGDEBUG << " --- scheduler queue ---";
    for (PriorityQueue::ordered_iterator it = pq.ordered_begin(); it != pq.ordered_end(); ++it) {
        LOGDEBUG << *(*it) << ' ';
    }
}
void Scheduler::shutdown() {
    atc::Synchronized s(monitor);
    fShutdown = true;
    monitor.notifyAll();
}

void Scheduler::ping() {
    monitor.notifyAll();
}

void Scheduler::processPendingTasks() {
    if (vecTasks.empty()) return;
    LOGDEBUG << "time now:" << fds::util::getLocalTimeString(fds::util::getTimeStampSeconds());
    LOGDEBUG << "processing [" << vecTasks.size() << "] policies";
    taskProcessor->process(vecTasks);
    // now check the next time & reschedule the task
    for (auto task : vecTasks) {
        if (task->setNextRecurrence()) {
            LOGDEBUG << "rescheduling policy:" << task->policyId
                     << " @ " << fds::util::getLocalTimeString(task->runAtTime);
            handleMap[task->policyId] = pq.push(task);
        } else {
            LOGWARN << "no more recurrence of this policy: " << task->policyId;
        }
        om->counters->numSnapshotPoliciesScheduled.set(pq.size());
    }
    vecTasks.clear();
}

void Scheduler::run() {
    atc::Synchronized s(monitor);
    LOGNORMAL << "snapshot scheduler started";
    om->counters->numSnapshotPoliciesScheduled.set(pq.size());
    while (!fShutdown) {
        om->counters->numSnapshotPoliciesScheduled.set(pq.size());
        processPendingTasks();
        dump();
        while (!fShutdown && pq.empty()) {
            LOGDEBUG << "q empty .. waiting.";
            monitor.waitForever();
        }

        while (!fShutdown && !pq.empty()) {
            om->counters->numSnapshotPoliciesScheduled.set(pq.size());
            Task* task;
            uint64_t currTime = fds::util::getTimeStampSeconds();
            task = pq.top();
            // we have no more items to pick up
            // so process the tasks selected ..
            if (task->runAtTime > currTime && !vecTasks.empty()) {
                processPendingTasks();
                task = pq.top();
            }
            if (task->runAtTime > currTime) {
                // there is no task to be executed at the time
                dump();
                LOGDEBUG << "going into wait ...";
                monitor.waitForTimeRelative((task->runAtTime - currTime)*1000);  // ms
            } else {
                // to be executed now ..
                vecTasks.push_back(task);
                pq.pop();
                auto handleptr = handleMap.find(task->policyId);
                if (handleptr == handleMap.end()) {
                    LOGERROR << "major error . policyid is missing from the map : "
                             << task->policyId;
                } else {
                    handleMap.erase(handleptr);
                }
            }
        }
    }  // !fShutdown
}

}  // namespace snapshot
}  // namespace fds
