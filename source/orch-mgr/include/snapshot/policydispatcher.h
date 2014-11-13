/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_POLICYDISPATCHER_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_POLICYDISPATCHER_H_

#include <thrift/concurrency/Monitor.h>
#include <snapshot/schedulertask.h>
#include <util/Log.h>
#include <queue>
#include <thread>
#include <vector>
namespace fds {
class OrchMgr;
namespace snapshot {

class PolicyDispatcher : public TaskProcessor, HasLogger {
  public:
    explicit PolicyDispatcher(OrchMgr* om);
    bool process(const Task& task);
    uint process(const std::vector<Task *>& vecTasks);

    void run();
  protected:
    OrchMgr *om;
    apache::thrift::concurrency::Monitor monitor;
    std::queue<uint64_t> policyq;
    std::thread runner;
};

}  // namespace snapshot
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_POLICYDISPATCHER_H_
