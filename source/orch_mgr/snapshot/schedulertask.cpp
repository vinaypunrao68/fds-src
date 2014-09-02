/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <snapshot/schedulertask.h>
#include <memory>
#include <functional>
#include <string>
#include <libical/ical.h>
namespace fds { namespace snapshot {

bool Task::setRecurrence(std::string recurStr) {
    if (this->recurStr != recurStr) {
        this->recurStr = recurStr;
        recur = icalrecurrencetype_from_string(recurStr.c_str());

        // set the defaults as minute, second, hour
        // so that the event is triggered exactly at the hour/minute start
        // 00:00:00

        if (recur.by_second[0] == ICAL_RECURRENCE_ARRAY_MAX
            && recur.freq > ICAL_SECONDLY_RECURRENCE) {
            recur.by_second[1] = ICAL_RECURRENCE_ARRAY_MAX;
            recur.by_second[0] = 0;
        }

        if (recur.by_minute[0] == ICAL_RECURRENCE_ARRAY_MAX
            && recur.freq > ICAL_MINUTELY_RECURRENCE) {
            recur.by_minute[1] = ICAL_RECURRENCE_ARRAY_MAX;
            recur.by_minute[0] = 0;
        }

        if (recur.by_hour[0] == ICAL_RECURRENCE_ARRAY_MAX
            && recur.freq > ICAL_HOURLY_RECURRENCE) {
            recur.by_hour[1] = ICAL_RECURRENCE_ARRAY_MAX;
            recur.by_hour[0] = 0;
        }

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
        tt = icaltime_as_timet(next);
        if (runAtTime == (uint64_t)tt) {
            tt = 0;
            next = icalrecur_iterator_next(ritr);
            if (!icaltime_is_null_time(next)) {
                tt = icaltime_as_timet(next);
            }
        }
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
    time_t tt = (time_t)task.runAtTime;
    os << "[policy:" << task.policyId
       << " at:" << task.runAtTime
       << " : " << asctime(gmtime(&tt)) //NOLINT
       << "]";
    return os;
}

}  // namespace snapshot
}  // namespace fds
