/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_FDS_OSDEP_H_
#define SOURCE_PLATFORM_INCLUDE_FDS_OSDEP_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern pid_t fds_spawn(char *const argv[], int daemonize);
extern pid_t fds_spawn_service(const char *prog, const char *fds_root, int daemonize);

#ifdef __cplusplus
}
#endif
#endif  // SOURCE_PLATFORM_INCLUDE_FDS_OSDEP_H_
