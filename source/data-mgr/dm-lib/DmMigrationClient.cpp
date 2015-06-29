/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationClient::DmMigrationClient(DataMgr& _dataMgr)
    : dataMgr(_dataMgr)
{
}

DmMigrationClient::~DmMigrationClient()
{
}


}  // namespace fds


