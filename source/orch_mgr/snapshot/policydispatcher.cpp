/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <snapshot/policydispatcher.h>
#include <orchMgr.h>
#include <vector>
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

void PolicyDispatcher::run() {
    uint64_t policyId = 0;

    while (true) {
        // this block is needed because we need to hold the lock only for a short time
        {
            atc::Synchronized s(monitor);
            while (policyq.empty()) {
                monitor.waitForever();
            }
            policyId = policyq.front();
            policyq.pop();
        }

        std::vector<int64_t> vecVolumes;
        om->getConfigDB()->listVolumesForSnapshotPolicy(vecVolumes, policyId);

        LOGDEBUG << "snapshot requests will be dispatched for ["
                 << vecVolumes.size() << "] volumes";

        for (auto id : vecVolumes) {
            LOGDEBUG << "snapshot request for volumeid:" << id;
        }
    }
}

}  // namespace snapshot
}  // namespace fds

