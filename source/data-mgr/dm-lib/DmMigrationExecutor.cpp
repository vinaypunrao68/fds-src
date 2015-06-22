/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationExecutor.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DmIoReqHandler* DmReqHandle)
    : DmReqHandler(DmReqHandle)
{

}

DmMigrationExecutor::~DmMigrationExecutor()
{

}

}  // namespace fds
