/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <snapshot/schedulertask.h>
#include <memory>
#include <functional>
#include <string>

namespace fds { namespace snapshot {

bool Task::setRecurrence(std::string recurStr) {
    if (this->recurStr != recurStr) {
        this->recurStr = recurStr;
        recur = icalrecurrencetype_from_string(recurStr.c_str());
        setNextRecurrence();
        return true;
    }
    return false;
}

bool Task::setNextRecurrence() {
    runAtTime = getNextRecurrance();
    return runAtTime > 0 ? true : false;
}

uint64_t Task::getNextRecurrance() {
    time_t tt = 0;
    icalrecur_iterator* ritr;
    struct icaltimetype start, next;
    start = icaltime_from_timet_with_zone(time(NULL), 0, NULL);
    ritr = icalrecur_iterator_new(recur, start);
    next = icalrecur_iterator_next(ritr);
    if (!icaltime_is_null_time(next)) {
        tt = icaltime_as_timet(start);
    }
    icalrecur_iterator_free(ritr);
    return tt;
}

bool Task::operator < (const Task& task) const {
    // this is because we need a min-heap ..
    // all boost & std heaps are max-heaps
    return runAtTime > task.runAtTime;
}

std::ostream& operator<<(std::ostream& os, const Task& task) {
    os << "[policy:" << task.policyId <<" at:" << task.runAtTime << "]";
    return os;
}

}  // namespace snapshot
}  // namespace fds
