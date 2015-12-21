/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_VOLUMEGROUP_EXTENSIONS_H_
#define SOURCE_INCLUDE_NET_VOLUMEGROUP_EXTENSIONS_H_

#include <sstream>
#include <fdsp/volumegroup_types.h>
#include <fds_defines.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {
using StringPtr = SHPTR<std::string>;

struct VolumeGroupConstants {
    static const int64_t            OPSTARTID = 0;
    static const int64_t            COMMITSTARTID = 0;
    static const int32_t            VERSION_INVALID = 0;
    static const int32_t            VERSION_SKIPCHECK = 1;
    static const int32_t            VERSION_START = 2;
};

template <class MsgT>
fpi::VolumeIoHdr& getVolumeIoHdrRef(MsgT& msg)
{
    return msg.volumeIoHdr;
}

template <class MsgT>
const fpi::VolumeIoHdr& getVolumeIoHdrRef(const MsgT& msg)
{
    return msg.volumeIoHdr;
}

template <class MsgT>
int64_t getVolIdFromSvcMsg(const MsgT& msg)
{
    return msg.groupId;
}

template<class MsgT>
int64_t getVolIdFromSvcMsgWithIoHdr(const MsgT &msg)
{
    return getVolumeIoHdrRef<MsgT>(msg).groupId;
}

std::string logString(const fpi::VolumeIoHdr &hdr);
std::string logString(const fpi::StartTxMsg& msg);
std::string logString(const fpi::UpdateTxMsg& msg);
std::string logString(const fpi::CommitTxMsg& msg);
std::string logString(const fpi::AddToVolumeGroupCtrlMsg& msg);
std::string logString(const fpi::PullCommitLogEntriesMsg &msg);
std::string logString(const fpi::VolumeGroupInfo &group);
std::string logString(const fpi::AddToVolumeGroupRespCtrlMsg& msg);

}  // namespace fds

#endif          // SOURCE_INCLUDE_NET_VOLUMEGROUP_EXTENSIONS_H_
