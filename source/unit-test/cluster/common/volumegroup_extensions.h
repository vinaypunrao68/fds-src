/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef _VOLUMEGROUP_EXTENSIONS_H_
#define _VOLUMEGROUP_EXTENSIONS_H_

#include <sstream>
#include <fdsp/volumegroup_types.h>
#include <fds_defines.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {
using StringPtr = SHPTR<std::string>;

struct VolumeGroupConstants {
    static const int64_t            OPSTARTID = 0;
    static const int64_t            COMMITSTARTID = 0;
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

std::string logString(const fpi::VolumeIoHdr &hdr);
std::string logString(const fpi::StartTxMsg& msg);
std::string logString(const fpi::AddToVolumeGroupCtrlMsg& msg);
std::string logString(const fpi::PullCommitLogEntriesMsg &msg);
std::string logString(const fpi::PullActiveTxsMsg& msg);

}  // namespace fds

#endif
