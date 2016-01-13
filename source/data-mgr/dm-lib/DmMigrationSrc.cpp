/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "../include/DmMigrationSrc.h"
#include <DataMgr.h>

namespace fds {

fds_bool_t
DmMigrationSrc::shouldForwardIO(fds_uint64_t dmtVersionIn) {
    return false;
}

void DmMigrationSrc::routeAbortMigration() {
    abortMigration();
}
}// namespace fds
