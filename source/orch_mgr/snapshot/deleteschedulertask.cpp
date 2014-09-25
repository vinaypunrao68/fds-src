/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <snapshot/deleteschedulertask.h>
#include <functional>
#include <string>
#include <util/timeutils.h>
namespace fds { namespace snapshot {

bool DeleteTask::operator < (const DeleteTask& task) const {
    // this is because we need a min-heap ..
    // all boost & std heaps are max-heaps
    return runAtTime > task.runAtTime;
}

std::ostream& operator<<(std::ostream& os, const DeleteTask& task) {
    time_t tt = (time_t)task.runAtTime;
    os << "[volume:" << task.volumeId
       << " at:" << task.runAtTime
       << " : " << fds::util::getLocalTimeString(task.runAtTime)
       << "]";
    return os;
}

}  // namespace snapshot
}  // namespace fds
