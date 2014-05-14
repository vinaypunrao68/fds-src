/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <fdsp_utils.h>
#include <fdsp/fds_service_types.h>

namespace fds {

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const ObjectID& rhs)
{
    lhs.digest.assign(reinterpret_cast<const char*>(rhs.GetId()), rhs.GetLen());
    return lhs;
}

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const meta_obj_id_t& rhs)
{
    lhs.digest.assign(reinterpret_cast<const char*>(rhs.metaDigest),
            sizeof(rhs.metaDigest));
    return lhs;
}

std::string logString(const FDS_ProtocolInterface::AsyncHdr &header)
{
    std::ostringstream oss;
    oss << "Id: " << header.msg_src_id << "From: " << header.msg_src_uuid.svc_uuid
            << " To: " << header.msg_dst_uuid.svc_uuid << " error: " << header.msg_code;
    return oss.str();
}

}  // namespace fds
