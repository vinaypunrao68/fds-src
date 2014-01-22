/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROCESS_GLOBALS_H_
#define SOURCE_INCLUDE_FDS_PROCESS_GLOBALS_H_

#include <fds_process.h>
#include <util/Log.h>

namespace fds {
    FdsProcess* g_fdsprocess = NULL;
    fds_log* g_fdslog = new fds_log("test.log");
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROCESS_GLOBALS_H_
