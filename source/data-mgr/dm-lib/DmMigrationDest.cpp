/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmMigrationDest.h>

namespace fds {

Error
DmMigrationDest::start()
{
    Error err(ERR_OK);
    LOGMIGRATE << "migrationid: " << migrationId
            << " starting Destination blob diff for volume: " << volumeUuid;

    // true - volumeGroupMode
    processInitialBlobFilterSet();

    return err;
}


void DmMigrationDest::routeAbortMigration() {
    abortMigration();
}
} // namespace fds
