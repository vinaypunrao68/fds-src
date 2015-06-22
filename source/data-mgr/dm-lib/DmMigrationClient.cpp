/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationClient::DmMigrationClient(DmIoReqHandler* DmReqHandle)
    : DmReqHandler(DmReqHandle)
{
}

DmMigrationClient::~DmMigrationClient()
{
}


}  // namespace fds


