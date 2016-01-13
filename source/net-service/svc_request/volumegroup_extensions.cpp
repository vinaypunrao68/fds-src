/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <net/volumegroup_extensions.h>

namespace fds {

const int64_t VolumeGroupConstants::OPSTARTID;
const int64_t VolumeGroupConstants::COMMITSTARTID;
const int32_t VolumeGroupConstants::VERSION_INVALID;
const int32_t VolumeGroupConstants::VERSION_SKIPCHECK;
const int32_t VolumeGroupConstants::VERSION_START;

std::string logString(const fpi::VolumeIoHdr &hdr)
{
    std::stringstream ss;
    ss << " volumeid: " << hdr.groupId
        << " versionid: " << hdr.version
        << " opid: " << hdr.opId
        << " commitid: " << hdr.commitId;
    return ss.str();
}

std::string logString(const fpi::StartTxMsg& msg)
{
    return logString(getVolumeIoHdrRef(msg));
}

std::string logString(const fpi::UpdateTxMsg& msg)
{
    return logString(getVolumeIoHdrRef(msg));
}

std::string logString(const fpi::CommitTxMsg& msg)
{
    return logString(getVolumeIoHdrRef(msg));
}

std::string logString(const fpi::AddToVolumeGroupCtrlMsg& msg) {
    std::stringstream ss;
    ss << " targetstate: " << msg.targetState
        << " groupid: " << msg.groupId
        << " replicaversion: " << msg.replicaVersion
        << " svcUuid: " << msg.svcUuid.svc_uuid
        << " lastopid: " << msg.lastOpId
        << " lastCommitId: " << msg.lastCommitId;
    return ss.str();
}

std::string logString(const fpi::PullCommitLogEntriesMsg &msg)
{
    std::stringstream ss;
    ss << " PullCommitLogEntriesMsg groupId: " << msg.groupId
        << " startCommitId: " << msg.startCommitId
        << " endCommitId: " << msg.endCommitId;
    return ss.str();
}

std::string logString(const fpi::VolumeGroupInfo &group)
{
    std::stringstream ss;
    ss << " groupId: " << group.groupId
        << " version: " << group.version
        << " lastOpId: " << group.lastOpId
        << " lastCommitId: " << group.lastCommitId
        << " functional size: " << group.functionalReplicas.size()
        << " nonfunctionalReplicas size: " << group.nonfunctionalReplicas.size();
    return ss.str();
}

std::string logString(const fpi::AddToVolumeGroupRespCtrlMsg& msg)
{
    std::stringstream ss;
    ss << "AddToVolumeGroupRespCtrlMsg groupid: " << fds::logString(msg.group);
    return ss.str();
}
}  // namespace fds
