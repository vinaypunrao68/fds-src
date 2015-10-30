/**
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORECOMMON_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORECOMMON_H_

/**
 * This header file is shared across Object Store universer.
 * It contains generic declaration which may be accessed in
 * Object Store and related entities like Object DataStore,
 * Object Metadata Store, Object PersistData Store, Object
 * Metadata Db  etc.
 */
#include <fds_types.h>
#include <persistent-layer/dm_io.h>
#include <functional>

namespace fds {
typedef std::function <void (fds_token_id smTokId,
                             diskio::DataTier tier,
                             const Error& error)> UpdateMediaTrackerFnObj;
typedef std::function <void (const fds_token_id& smTokId,
                             const diskio::DataTier& tier,
                             diskio::TokenStat &tokenStats)> EvaluateObjSetFn;
}
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORECOMMON_H_
