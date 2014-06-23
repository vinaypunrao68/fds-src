/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <SmIo.h>

namespace fds {
std::string SmIoPutObjectReq::log_string()
{
    std::stringstream ret;
    ret << " SmIoPutObjectReq";
    return ret.str();
}
}  // namespace fds
