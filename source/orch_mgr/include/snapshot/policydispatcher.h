/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_POLICYDISPATCHER_H_
#define SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_POLICYDISPATCHER_H_

#include <snapshot/schedulertask.h>
#include <util/Log.h>

namespace fds {
class OrchMgr;
namespace snapshot {

class PolicyDispatcher : public TaskProcessor, HasLogger {
  public:
    explicit PolicyDispatcher(OrchMgr* om);
    bool process(const Task& task);

    OrchMgr *om;
};

}  // namespace snapshot
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_SNAPSHOT_POLICYDISPATCHER_H_
