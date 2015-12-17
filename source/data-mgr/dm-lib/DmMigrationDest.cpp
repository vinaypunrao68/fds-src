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
    processInitialBlobFilterSet(true);

    return err;
}

void
DmMigrationDest::staticMigrationStatusToSrc(NodeUuid srcNodeUuid,
                                            fds_volid_t volumeId,
                                            const Error &result)
{
    // TODO
}

} // namespace fds
