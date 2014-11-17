/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_SCHEDULERTASK_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_SCHEDULERTASK_H_
#include <libical/ical.h>
#include <functional>
#include <string>
#include <ostream>
#include <vector>
namespace fds {
namespace snapshot {

struct Task {
    uint64_t policyId;
    uint64_t runAtTime;  // seconds
    struct icalrecurrencetype recur;
    std::string recurStr;

    bool setRecurrence(std::string recurStr);
    bool setNextRecurrence();
    uint64_t getNextRecurrance();
    bool operator < (const Task& task) const;
    friend std::ostream& operator<<(std::ostream& os, const Task& task);
};


struct TaskProcessor {
    virtual bool process(const Task& task) = 0;
    virtual uint process(const std::vector<Task*>& vecTasks) {
        uint count = 0;
        for (const auto& task : vecTasks) {
            if (process(*task)) ++count;
        }
        return count;
    }
};

}  // namespace snapshot
}  // namespace fds

namespace std {
template<>
struct less<fds::snapshot::Task*> {
    bool operator()(const fds::snapshot::Task* lhs, const fds::snapshot::Task* rhs) const {
        return *lhs < *rhs;
    }
};}


#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_SCHEDULERTASK_H_
