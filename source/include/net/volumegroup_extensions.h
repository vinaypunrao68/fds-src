/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_VOLUMEGROUP_EXTENSIONS_H_
#define SOURCE_INCLUDE_NET_VOLUMEGROUP_EXTENSIONS_H_

#include <sstream>
#include <fdsp/volumegroup_types.h>
#include <fds_defines.h>
#include <fds_error.h>

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

inline bool isVolumeGroupError(const Error &e)
{
    switch (e.GetErrno())
    {
        case ERR_OK:
            return false;
        case ERR_SVC_REQUEST_TIMEOUT:
        case ERR_SVC_REQUEST_INVOCATION:
        case ERR_SVC_REQUEST_FAILED:
        case ERR_IO_OPID_MISMATCH:
        case ERR_SYNC_INPROGRESS:
        case ERR_DM_VOL_NOT_ACTIVATED:
        case ERR_VOL_NOT_FOUND:
            return true;
        default:
            return false;
    }
    return false;

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
