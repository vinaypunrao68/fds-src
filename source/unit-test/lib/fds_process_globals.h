/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef _TEST_PROCESS_GLOBALS_H
#define _TEST_PROCESS_GLOBALS_H

#include <fds_process.h>
#include <util/Log.h>

namespace fds {
    FdsProcess* g_fdsprocess = NULL;
    fds_log* g_fdslog = new fds_log("test.log");
}
#endif
