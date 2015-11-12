/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef _VOLUMEGROUP_EXTENSIONS_H_
#define _VOLUMEGROUP_EXTENSIONS_H_

#include <sstream>
#include <fdsp/volumegroup_types.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {

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

}  // namespace fds

#endif
