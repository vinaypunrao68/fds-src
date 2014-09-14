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
std::string SmIoDeleteObjectReq::log_string()
{
    std::stringstream ret;
    ret << " SmIoDeleteObjectReq";
    return ret.str();
}

std::string SmIoAddObjRefReq::log_string()
{
    std::stringstream ret;
    ret << " SmIoAddObjRefReq";
    return ret.str();
}
}  // namespace fds
