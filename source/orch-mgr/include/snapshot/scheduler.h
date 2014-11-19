/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_SCHEDULER_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_SCHEDULER_H_
#include <boost/heap/fibonacci_heap.hpp>
#include <thrift/concurrency/Monitor.h>
#include <snapshot/schedulertask.h>
#include <apis/apis_types.h>
#include <fds_types.h>
#include <util/Log.h>
#include <map>
#include <thread>
#include <vector>

namespace fds {
class OrchMgr;
namespace snapshot {

class Scheduler : public HasLogger {
  public:
    explicit Scheduler(OrchMgr* om, TaskProcessor* processor);
    ~Scheduler();
    // will also update / modify
    bool addPolicy(const fpi::SnapshotPolicy& policy);
    bool removePolicy(uint64_t policyId);
    void dump();
    void shutdown();

  protected:
    typedef boost::heap::fibonacci_heap<Task*> PriorityQueue;

    // to process the actual tasks
    TaskProcessor* taskProcessor;

    // list of tasks
    PriorityQueue pq;

    // map of policyid to internal map handle
    std::map<uint64_t, PriorityQueue::handle_type> handleMap;

    // monitor to sleep & notify
    apache::thrift::concurrency::Monitor monitor;
    std::vector<Task*> vecTasks;
    void processPendingTasks();
    // to shutdown
    bool fShutdown = false;
    OrchMgr* om;
    std::thread* runner;

    // run the scheduler
    void run();

  private:
    // signal a change
    void ping();
};
}  // namespace snapshot
}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_SCHEDULER_H_
