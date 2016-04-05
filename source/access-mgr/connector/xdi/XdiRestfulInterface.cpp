/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

// System includes
#include <cpr/cpr.h>

// FDS includes
#include <util/Log.h>
#include "connector/xdi/XdiRestfulInterface.h"

namespace fds {

unsigned int XdiRestfulInterface::TIMEOUT = 10 * 60 * 1000;     // 10 minutes in millis

XdiRestfulInterface::XdiRestfulInterface(std::string const& host, unsigned int const port)
                                : _host(host), _port(port)
{

}

XdiRestfulInterface::~XdiRestfulInterface()
{

}

bool XdiRestfulInterface::flushVolume(std::string const& volume)
{
    auto r = cpr::Get(cpr::Url{"http://" + _host + ":" + std::to_string(_port) + "/flush/" + volume}, cpr::Timeout{TIMEOUT});

    bool ret = false;
    if (200 == r.status_code) {
        ret = true;
    } else if (cpr::ErrorCode::OPERATION_TIMEDOUT == r.error.code) {
        LOGERROR << "Flush timed out for volume: " << volume;
        ret = false;
    } else {
        LOGERROR << "Flush failed with status code: " << r.status_code;
        ret = false;
    }
    return ret;
}

} // namespace fds
