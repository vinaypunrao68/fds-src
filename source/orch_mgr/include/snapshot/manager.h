/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_MANAGER_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_MANAGER_H_
#include <ostream>

#include <util/Log.h>
#include <snapshot/scheduler.h>
#include <snapshot/policydispatcher.h>
#include <snapshot/svchandler.h>

namespace fds {
class OrchMgr;
namespace snapshot {

class Manager : public HasLogger {
  public:
    explicit Manager(OrchMgr* om);
    void init();
    bool loadFromConfigDB();
    bool addPolicy(fpi::SnapshotPolicy& policy);
    bool removePolicy(int64_t id);
    OmSnapshotSvcHandler svcHandler;
  protected:
    OrchMgr* om;
    Scheduler* snapScheduler;
    PolicyDispatcher* snapPolicyDispatcher;
};

}  // namespace snapshot
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_MANAGER_H_
