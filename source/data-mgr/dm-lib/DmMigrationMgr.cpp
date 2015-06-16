/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationMgr.h>
#include <DmMigrationExecutor.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationMgr::DmMigrationMgr(DmIoReqHandler *DmReqHandle)
    : DmReqHandler(DmReqHandle),
      OmStartMigrCb(NULL)
{

}

DmMigrationMgr::~DmMigrationMgr()
{

}

}  // namespace fds
