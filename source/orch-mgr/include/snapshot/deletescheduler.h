/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETESCHEDULER_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETESCHEDULER_H_
#include <boost/heap/fibonacci_heap.hpp>
#include <thrift/concurrency/Monitor.h>
#include <snapshot/deleteschedulertask.h>
#include <fdsp/common_types.h>
#include <fds_types.h>
#include <util/Log.h>
#include <map>
#include <thread>

namespace fds {
class OrchMgr;
namespace snapshot {

class DeleteScheduler : public HasLogger {
  public:
    explicit DeleteScheduler(OrchMgr* om, DeleteTaskProcessor* processor);
    ~DeleteScheduler();
    // will also update / modify
    bool addSnapshot(const fpi::Snapshot& snapshot);
    bool removeVolume(fds_volid_t volumeId);
    void dump();
    void shutdown();

    // signal a change
    void ping();

  protected:
    typedef boost::heap::fibonacci_heap<DeleteTask*> PriorityQueue;

    // to process the actual tasks
    DeleteTaskProcessor* taskProcessor;

    // list of tasks
    PriorityQueue pq;

    // map of policyid to internal map handle
    std::map<fds_volid_t, PriorityQueue::handle_type> handleMap;

    // monitor to sleep & notify
    apache::thrift::concurrency::Monitor monitor;

    // to shutdown
    bool fShutdown = false;
    OrchMgr* om;
    std::thread* runner;

    // run the scheduler
    void run();

  private:
};
}  // namespace snapshot
}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_DELETESCHEDULER_H_
