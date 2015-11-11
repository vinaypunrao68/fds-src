/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <volumegroup_extensions.h>

namespace fds {

std::string logString(const fpi::VolumeIoHdr &hdr)
{
    std::stringstream ss;
    ss << " volume id: " << hdr.groupId
        << " version id: " << hdr.version;
    return ss.str();
}

std::string logString(const fpi::StartTxMsg& msg) {
    return logString(getVolumeIoHdrRef(msg));
}

}  // namespace fds
