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
    oss << " Req Id: " << header.msg_src_id << " Type: " << header.msg_type_id
        << " From: " << header.msg_src_uuid.svc_uuid
        << " To: " << header.msg_dst_uuid.svc_uuid << " error: " << header.msg_code;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::GetObjectMsg &getObj)
{
    std::ostringstream oss;
    oss << " GetObjectMsg Obj Id: " << ObjectID(getObj.data_obj_id.digest);
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
    oss << " QueryCatalogMsg volId: " << qryCat.volume_id
        << " blobName:  " << qryCat.blob_name;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::PutObjectMsg& putObj)
{
    std::ostringstream oss;
    oss << " PutObjectMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::PutObjectRspMsg& putObj)
{
    std::ostringstream oss;
    oss << " PutObjectRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::UpdateCatalogMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::UpdateCatalogRspMsg& updCat)
{
    std::ostringstream oss;
    oss < " UpdateCatalogRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::StartBlobTxMsg& stBlobTx)
{
    std::ostringstream oss;
    oss < " StartBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::DeleteCatalogObjectMsg& delcatMsg)
{
    std::ostringstream oss;
    oss < " DeleteBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CommitBlobTxMsg& commitBlbTx)
{
    std::ostringstream oss;
    oss < " CommitBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::AbortBlobTxMsg& abortBlbTx)
{
    std::ostringstream oss;
    oss < " AbortBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::GetBlobMetaDataMsg& message)
{
    return "GetBlobMetaDataMsg";
}

std::string logString(const FDS_ProtocolInterface::SetBlobMetaDataMsg& message)
{
    return "SetBlobMetaDataMsg";
}

}  // namespace fds
