/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_PROCESS_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_PROCESS_H_

#include <string>
#include <vector>

namespace fds
{
    namespace pm
    {
        pid_t fds_spawn(char *const argv[], int daemonize);

        pid_t fds_spawn_service(const std::string& prog, const std::string& fds_root, const std::vector<std::string>& args, bool daemonize);

        pid_t fds_spawn_service(const char *prog, const char *fds_root, const int argc, const char **extra_args, int daemonize);
    }  // namespace pm
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_PROCESS_H_
