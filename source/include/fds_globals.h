/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_GLOBALS_H_
#define SOURCE_INCLUDE_FDS_GLOBALS_H_

/**
 * Global fds references
 */
namespace fds {
    class FdsProcess;
    class fds_log;

    /* Global process */
    extern FdsProcess *g_fdsprocess;
    /* Global process wide log */
    extern fds_log *g_fdslog;
    /* Global build identifier string */
    extern const char* buildStrTmpl;
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_GLOBALS_H_
