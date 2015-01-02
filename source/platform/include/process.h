/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PROCESS_H_
#define SOURCE_PLATFORM_INCLUDE_PROCESS_H_

namespace fds
{
    pid_t fds_spawn(char *const argv[], int daemonize);
    extern pid_t fds_spawn_service(const char *prog, const char *fds_root, const char **extra_args,
                               int daemonize);
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PROCESS_H_
