/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETESCHEDULERTASK_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETESCHEDULERTASK_H_
#include <functional>
#include <string>
#include <ostream>
namespace fds {
namespace snapshot {

struct DeleteTask {
    /* volumeId of the volume whose snapshots should be deleted*/
    uint64_t volumeId;
    /* the latest time at which a snaphot belonging to the above volume
     is to be deleted*/
    uint64_t runAtTime;  // seconds

    bool operator < (const DeleteTask& task) const;
    friend std::ostream& operator<<(std::ostream& os, const DeleteTask& task);
};

struct DeleteTaskProcessor {
    virtual uint64_t process(const DeleteTask& task) = 0;
};

}  // namespace snapshot
}  // namespace fds


namespace std {
template<>
struct less<fds::snapshot::DeleteTask*> {
    bool operator()(const fds::snapshot::DeleteTask* lhs,
                    const fds::snapshot::DeleteTask* rhs) const {
        return *lhs < *rhs;
    }
};
}


#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETESCHEDULERTASK_H_
