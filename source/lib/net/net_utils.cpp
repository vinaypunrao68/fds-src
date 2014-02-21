/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <sys/unistd.h>

#include <string>
#include <net/net_utils.h>

namespace fds {
namespace net {
std::string get_my_hostname()
{
    char name[255];
    if (gethostname(name, 255) == 0) {
        return std::string(name);
    }
    return std::string();
}
}  // namespace net
}  // namespace fds
