/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <volumegroup_extensions.h>

namespace fds {

std::string logString(const fpi::VolumeIoHdr &hdr)
{
    std::stringstream ss;
    ss << " volumeId: " << hdr.groupId
        << " versionId: " << hdr.version
        << " opId: " << hdr.opId
        << " commitId: " << hdr.commitId;
    return ss.str();
}

std::string logString(const fpi::StartTxMsg& msg) {
    return logString(getVolumeIoHdrRef(msg));
}

std::string logString(const fpi::AddToVolumeGroupCtrlMsg& msg) {
    std::stringstream ss;
    ss << " targetState: " << msg.targetState
        << " groupId: " << msg.groupId
        << " svcUuid: " << msg.svcUuid.svc_uuid
        << " lastOpId: " << msg.lastOpId
        << " lastCommitId: " << msg.lastCommitId;
    return ss.str();
}

}  // namespace fds
