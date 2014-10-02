/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETEDISPATCHER_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETEDISPATCHER_H_

#include <queue>
#include <thread>

#include <thrift/concurrency/Monitor.h>
#include <snapshot/deleteschedulertask.h>

#include <util/Log.h>
#include <fdsp/snapshot_types.h>
#include <fds_types.h>
namespace fds {
class OrchMgr;
namespace snapshot {

class DeleteDispatcher : public DeleteTaskProcessor, HasLogger {
  public:
    explicit DeleteDispatcher(OrchMgr* om);
    uint64_t process(const DeleteTask& task);

    void run();
  protected:
    OrchMgr *om;
    apache::thrift::concurrency::Monitor monitor;
    std::queue<fpi::Snapshot> snapshotQ;
    std::thread runner;
};

}  // namespace snapshot
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETEDISPATCHER_H_
