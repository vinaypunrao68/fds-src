/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#include <string>
#include <fdsp_utils.h>
#include <fdsp/am_types_types.h>
#include <fdsp/dm_api_types.h>
#include <fdsp/sm_api_types.h>
#include <fds_resource.h>
#include <net/SvcMgr.h>
#include <net/SvcRequest.h>

#include <boost/algorithm/string/replace.hpp>

using boost::replace_all;

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

FDS_ProtocolInterface::FDS_ObjectIdType strToObjectIdType(const std::string & rhs) {
    fds_verify(rhs.compare(0, 2, "0x") == 0);  // Require 0x prefix
    fds_verify(rhs.size() == (40 + 2));  // Account for 0x

    FDS_ProtocolInterface::FDS_ObjectIdType objId;
    char a, b;
    uint j = 0;
    // Start the offset at 2 to account of 0x
    for (uint i = 2; i < rhs.length(); i += 2, j++) {
        a = rhs[i];   if (a > 57) a -= 87; else a -= 48; // NOLINT
        b = rhs[i+1]; if (b > 57) b -= 87; else b -= 48; // NOLINT
        objId.digest += (a << 4 & 0xF0) + b;
    }

    return objId;
}

FDS_ProtocolInterface::SvcUuid&
assign(FDS_ProtocolInterface::SvcUuid& lhs, const ResourceUUID& rhs)
{
    lhs.svc_uuid = rhs.uuid_get_val();
    return lhs;
}

FDS_ProtocolInterface::FDSP_Uuid&
assign(FDS_ProtocolInterface::FDSP_Uuid& lhs, const fpi::SvcID& rhs)
{
    lhs.uuid = rhs.svc_uuid.svc_uuid;
    return lhs;
}

void swapAsyncHdr(boost::shared_ptr<fpi::AsyncHdr> &header)
{
    auto temp = header->msg_src_uuid;
    header->msg_src_uuid = header->msg_dst_uuid;
    header->msg_dst_uuid = temp;
}

std::string logString(const FDS_ProtocolInterface::AsyncHdr &header)
{
    std::ostringstream oss;
    oss << " Req Id: " << static_cast<SvcRequestId>(header.msg_src_id)
        << " Type: " << fpi::_FDSPMsgTypeId_VALUES_TO_NAMES.at(header.msg_type_id)
        << std::hex
        << " From: " << SvcMgr::mapToSvcUuidAndName(header.msg_src_uuid)
        << " To: " << SvcMgr::mapToSvcUuidAndName(header.msg_dst_uuid)
        << std::dec
        << " DLT version: " << header.dlt_version
        << " error: " << header.msg_code;
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
    oss << " PutObjectMsg for object " << ObjectID(putObj.data_obj_id.digest)
	<< " Volume UUID " << std::hex << putObj.volume_id << std::dec
	<< " Object length " << putObj.data_obj_len;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::OpenVolumeMsg &openVol)
{
    std::ostringstream oss;
    oss << " OpenVolumeMsg Vol Id: " << openVol.volume_id;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CloseVolumeMsg &closeVol)
{
    std::ostringstream oss;
    oss << " CloseVolumeMsg Vol Id: " << closeVol.volume_id;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::ReloadVolumeMsg & vol) {
    std::ostringstream oss;
    oss << " ReloadVolumeMsg Vol Id: " << vol.volume_id;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CtrlNotifyDMStartMigrationMsg & vol) {
    std::ostringstream oss;
    oss << " CtrlNotifyDMStartMigrationMsg Vol Id: ";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CtrlNotifyInitialBlobFilterSetMsg &msg)
{
	std::ostringstream oss;
	oss << " CtrlNotifyInitialBlobFilterSetMsg Vol Id: " << msg.volumeId;
	return oss.str();
}

std::string logString(const fpi::CtrlNotifyDeltaBlobDescMsg &msg)
{
	std::ostringstream oss;
	oss << " CtrlNotifyDeltaBlobDescMsg volume=" << msg.volume_id
        << " seq_id=" << msg.msg_seq_id
        << " last_msg_seq_id=" << msg.last_msg_seq_id
        << " list_size=" << msg.blob_desc_list.size();
	return oss.str();
}

std::string logString(const fpi::CtrlNotifyDeltaBlobsMsg &msg)
{
	std::ostringstream oss;
	oss << " CtrlNotifyDeltaBlobsMsg volume=" << msg.volume_id
        << " seq_id=" << msg.msg_seq_id
        << " last_msg_seq_id=" << msg.last_msg_seq_id
        << " list_size=" << msg.blob_obj_list.size();
	return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CtrlObjectMetaDataPropagate& msg)
{
    std::ostringstream oss;
    oss << " CtrlObjectMetaDataPropagate for object " << ObjectID(msg.objectID.digest)
	    << " reconcile " << msg.objectReconcileFlag
        << " refcnt " << msg.objectRefCnt
        << " objectCompressType " << msg.objectCompressType
        << " objectCompressLen " << msg.objectCompressLen
        << " objectBlkLen " << msg.objectBlkLen
        << " objectSize " << msg.objectSize
        << " objectFlags " << (fds_uint16_t)msg.objectFlags
        << " objectExpireTime" << msg.objectExpireTime << std::endl;
    for (auto volAssoc : msg.objectVolumeAssoc) {
        oss << "CtrlObjectMetaDataPropagate vol assoc "
            << std::hex << volAssoc.volumeAssoc << std::dec
            << " refcnt " << volAssoc.volumeRefCnt;
    }
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CtrlObjectMetaDataSync& msg)
{
    std::ostringstream oss;
    oss << " CtrlObjectMetaDataSync for object " << ObjectID(msg.objectID.digest)
        << " refcnt " << msg.objRefCnt << std::endl;

    for (auto volAssoc : msg.objVolAssoc) {
        oss << "CtrlObjectMetaDataPropagate vol assoc "
            << std::hex << volAssoc.volumeAssoc << std::dec
            << " refcnt " << volAssoc.volumeRefCnt;
    }
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
    oss << " UpdateCatalogRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::UpdateCatalogOnceMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogOnceMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::UpdateCatalogOnceRspMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogOnceRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::StartBlobTxMsg& stBlobTx)
{
    std::ostringstream oss;
    oss << " StartBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CommitBlobTxMsg& commitBlbTx)
{
    std::ostringstream oss;
    oss << " CommitBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::SetBlobMetaDataMsg& setMDMsg)
{
    std::ostringstream oss;
    oss << " SetBlobMetaDataMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::SetBlobMetaDataRspMsg& setMDRspMsg)
{
    std::ostringstream oss;
    oss << " SetBlobMetaDataRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::DeleteObjectMsg& delMsg)
{
    std::ostringstream oss;
    oss << " DeleteObjectMsg " << ObjectID(delMsg.objId.digest)
	<< " Volume UUID " << std::hex << delMsg.volId << std::dec;
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::DeleteObjectRspMsg& delRspMsg)
{
    std::ostringstream oss;
    oss << " DeleteObjectRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::AddObjectRefMsg& copyMsg)
{
    std::ostringstream oss;
    oss << " AddObjectRefMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::AddObjectRefRspMsg& copyRspMsg)
{
    std::ostringstream oss;
    oss << " AddObjectRefRspMsg";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::AbortBlobTxMsg& abortBlbTx)
{
    std::ostringstream oss;
    // FIXME(DAC): This does nothing.
    oss << " AbortBlobTxMs";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::GetBlobMetaDataMsg& message)
{
    return "GetBlobMetaDataMsg";
}

std::string logString(const FDS_ProtocolInterface::RenameBlobMsg& message)
{
    return "RenameBlobMsg";
}

std::string logString(const FDS_ProtocolInterface::RenameBlobRespMsg& message)
{
    return "RenameBlobRespMsg";
}

std::string logString(const FDS_ProtocolInterface::SetVolumeMetadataMsg& msg) {
    return "SetVolumeMetadataMsg";
}

std::string logString(const FDS_ProtocolInterface::GetVolumeMetadataMsgRsp& msg) {
    return "GetVolumeMetadataMsgRsp";
}

std::string logString(const FDS_ProtocolInterface::StatVolumeMsg& msg) {
    return "StatVolumeMsg";
}

std::string logString(const FDS_ProtocolInterface::GetBucketMsg& msg) {
    std::ostringstream oss;
    oss << " GetBucketMsg(volume_id: " << msg.volume_id
            << ", count: " << msg.count
            << ", startPos: " << msg.startPos
            << ", pattern: " << msg.pattern << ")";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::GetBucketRspMsg& msg) {
    std::ostringstream oss;
    oss << " GetBucketRspMsg(count: " << msg.blob_descr_list.size() << ")";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::DeleteBlobMsg& msg) {
    std::ostringstream oss;
    oss << " DeleteBlobMsg ";
    return oss.str();
}

std::string logString(const FDS_ProtocolInterface::CtrlNotifyFinishVolResyncMsg& msg) {
    std::ostringstream oss;
    oss << " Finish volume Resync for volume ID: " << msg.volume_id
    		<< ", DMT Version: " << msg.DMT_Version
			<< ", commit log sequence number: " << msg.commit_log_seq_num;
    return oss.str();
}

std::string quoteString(std::string const& text,
                        std::string const& delimiter,
                        std::string const& escape) {
    std::string retval(text);
    replace_all(retval, escape, escape + escape);
    replace_all(retval, delimiter, escape + delimiter);
    return delimiter + retval + delimiter;
}

}  // namespace fds
