/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMUTILS_H_
#define SOURCE_STOR_MGR_INCLUDE_SMUTILS_H_

#include <sys/statvfs.h>
#include "fds_error.h"
#include "fds_types.h"
#include <persistent-layer/dm_io.h>
#include <SmTypes.h>


namespace fds {

Error getDiskUsageInfo(DiskLocMap &diskLocMap,
                       const DiskId &diskId,
                       diskio::DiskStat &diskStat);
Error getDiskUsageInfo(std::string diskPath,
                       diskio::DiskStat &diskStat);

}  // namespace fds
#endif // SOURCE_STOR_MGR_INCLUDE_SMUTILS_H_
