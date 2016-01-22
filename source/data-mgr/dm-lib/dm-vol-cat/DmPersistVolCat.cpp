/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

// Standard includes.
#include <cstdlib>
#include <limits>
#include <string>

// System includes.
#include <linux/limits.h>

// Internal includes.
#include "dm-vol-cat/DmPersistVolCat.h"
#include "leveldb/db.h"
#include "net/SvcMgr.h"
#include "fds_process.h"
#include "fds_module.h"
#include "fds_resource.h"

namespace fds {

extern const JournalTimestampKey OP_TIMESTAMP_KEY { };
extern const leveldb::Slice OP_TIMESTAMP_REC { static_cast<leveldb::Slice>(OP_TIMESTAMP_KEY) };

Error DmPersistVolCat::syncCatalog(const NodeUuid & dmUuid) {
    throw new Exception("function deprecated.. DO NOT USE");
    return ERR_OK;
}

}  // namespace fds
