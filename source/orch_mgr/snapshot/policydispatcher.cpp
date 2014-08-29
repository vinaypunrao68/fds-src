/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <snapshot/policydispatcher.h>
#include <orchMgr.h>
#include <vector>

namespace fds { namespace snapshot {

PolicyDispatcher::PolicyDispatcher(OrchMgr* om) : om(om) {
}

bool PolicyDispatcher::process(const Task& task) {
    LOGDEBUG << "received message to process task : " << task.policyId;
    std::vector<int64_t> vecVolumes;
    om->getConfigDB()->listVolumesForSnapshotPolicy(vecVolumes, task.policyId);

    LOGDEBUG << "snapshot requests will be dispatched for [" << vecVolumes.size() << "] volumes";

    for (auto id : vecVolumes) {
        LOGDEBUG << "snapshot request for volumeid:" << id;
    }

    return true;
}

}  // namespace snapshot
}  // namespace fds

