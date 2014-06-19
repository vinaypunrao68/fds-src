/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <fdsp_utils.h>
#include <fdsp/fds_service_types.h>
#include <fds_resource.h>

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

FDS_ProtocolInterface::SvcUuid&
assign(FDS_ProtocolInterface::SvcUuid& lhs, const ResourceUUID& rhs)
{
    lhs.svc_uuid = rhs.uuid_get_val();
    return lhs;
}

std::string logString(const FDS_ProtocolInterface::AsyncHdr &header)
{
    std::ostringstream oss;
    oss << " Req Id: " << header.msg_src_id << " From: " << header.msg_src_uuid.svc_uuid
            << " To: " << header.msg_dst_uuid.svc_uuid << " error: " << header.msg_code;
    return oss.str();
}

std::ostringstream& logMsgCommon(std::ostringstream& oss,
                                 const std::string &type,
                                 const FDS_ProtocolInterface::AsyncHdr &header)
{
    oss << type << " Req Id: " << header.msg_src_id << " From: " << header.msg_src_uuid.svc_uuid;
    return oss;
}

std::string logString(const FDS_ProtocolInterface::GetObjectMsg &getObj)
{
    std::ostringstream oss;
    logMsgCommon(oss, "GetObjectMsg", getObj.hdr)
        << " Obj Id: " << ObjectID(getObj.data_obj_id.digest);
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::GetObjectResp &getObj)
{
    std::ostringstream oss;
    oss << " GetObjectResp ";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::QueryCatalogMsg& qryCat)
{
    std::ostringstream oss;
    logMsgCommon(oss, "QueryCatalogMsg", qryCat.hdr) << " volId: " << qryCat.volume_id
        << " blobName:  " << qryCat.blob_name;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::PutObjectMsg& putObj)
{
    std::ostringstream oss;
    logMsgCommon(oss, "PutObjectMsg", putObj.hdr);
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::PutObjectRspMsg& putObj)
{
    std::ostringstream oss;
    logMsgCommon(oss, "PutObjectRspMsg", putObj.hdr);
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::UpdateCatalogMsg& updCat)
{
    std::ostringstream oss;
    logMsgCommon(oss, "UpdateCatalogMsg", updCat.hdr);
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::UpdateCatalogRspMsg& updCat)
{
    std::ostringstream oss;
    logMsgCommon(oss, "UpdateCatalogRspMsg", updCat.hdr);
    return oss.str();
}
}  // namespace fds
