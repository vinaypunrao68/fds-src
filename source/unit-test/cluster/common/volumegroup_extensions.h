#include <fdsp/volumegroup_types.h>
namespace fds {
namespace fpi = FDS_ProtocolInterface;

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
}  // namespace fds
