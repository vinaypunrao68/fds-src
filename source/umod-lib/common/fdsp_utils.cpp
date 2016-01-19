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
/**
 * NOTE : use this declaration in your cpp along with includes outside fds namespace
 * like DECL_EXTERN_OUTPUT_FUNCS(ActiveObjectsMsg);
 */

using boss = boost::log::formatting_ostream;

#define DEFINE_OUTPUT_FUNCS(MSGTYPE)                                    \
    boss& operator<<(boss& out, const fpi::MSGTYPE &msg);               \
    boss& operator<<(boss& out, const fpi::MSGTYPE* msg) {              \
        if (msg) out << (*msg);                                         \
        else out << "[ " #MSGTYPE " msg is NULL ]";                     \
        return out;                                                     \
    }                                                                   \
    boss& operator<<(boss& out, const SHPTR<fpi::MSGTYPE> &msg) {       \
        if (msg.get()) out << (*msg);                                   \
        else out << "[ " #MSGTYPE " msg is NULL ]";                     \
        return out;                                                     \
    }                                                                   \
    std::string logString(const fpi::MSGTYPE &msg) {                    \
        boss oss;                                                       \
        std::string s; oss.attach(s);                                   \
        oss << msg;                                                     \
        return oss.str();                                               \
    }                                                                   \
    boss& operator<<(boss& out, const fpi::MSGTYPE &msg)

namespace fds {

fpi::FDS_ObjectIdType& assign(fpi::FDS_ObjectIdType& lhs, const ObjectID& rhs) {
    lhs.digest.assign(reinterpret_cast<const char*>(rhs.GetId()), rhs.GetLen());
    return lhs;
}

fpi::FDS_ObjectIdType& assign(fpi::FDS_ObjectIdType& lhs, const meta_obj_id_t& rhs) {
    lhs.digest.assign(reinterpret_cast<const char*>(rhs.metaDigest),
            sizeof(rhs.metaDigest));
    return lhs;
}

fpi::FDS_ObjectIdType strToObjectIdType(const std::string & rhs) {
    fds_verify(rhs.compare(0, 2, "0x") == 0);  // Require 0x prefix
    fds_verify(rhs.size() == (40 + 2));  // Account for 0x

    fpi::FDS_ObjectIdType objId;
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

fpi::SvcUuid& assign(fpi::SvcUuid& lhs, const ResourceUUID& rhs) {
    lhs.svc_uuid = rhs.uuid_get_val();
    return lhs;
}

fpi::FDSP_Uuid& assign(fpi::FDSP_Uuid& lhs, const fpi::SvcID& rhs) {
    lhs.uuid = rhs.svc_uuid.svc_uuid;
    return lhs;
}

void swapAsyncHdr(boost::shared_ptr<fpi::AsyncHdr> &header) {
    auto temp = header->msg_src_uuid;
    header->msg_src_uuid = header->msg_dst_uuid;
    header->msg_dst_uuid = temp;
}

DEFINE_OUTPUT_FUNCS(AsyncHdr) {
    out << "["
        << " reqid:" << static_cast<SvcRequestId>(msg.msg_src_id)
        << " type:" << ((fpi::_FDSPMsgTypeId_VALUES_TO_NAMES.find(msg.msg_type_id) != 
                         fpi::_FDSPMsgTypeId_VALUES_TO_NAMES.end())?
                        fpi::_FDSPMsgTypeId_VALUES_TO_NAMES.at(msg.msg_type_id):"-UNKNOWN-")
        << std::hex
        << " from:" << SvcMgr::mapToSvcUuidAndName(msg.msg_src_uuid)
        << " to:" << SvcMgr::mapToSvcUuidAndName(msg.msg_dst_uuid)
        << std::dec
        << " dltversion:" << msg.dlt_version
        << " error:" << static_cast<fds_errno_t>(msg.msg_code)
        << "]";
    return out;
}

std::string logString(const fpi::GetObjectMsg &getObj)
{
    std::ostringstream oss;
    oss << " GetObjectMsg Obj Id: " << ObjectID(getObj.data_obj_id.digest);
    return oss.str();
}

std::string logString(const fpi::GetObjectResp &getObj)
{
    std::ostringstream oss;
    oss << " GetObjectResp ";
    return oss.str();
}

std::string logString(const fpi::QueryCatalogMsg& qryCat)
{
    std::ostringstream oss;
    oss << " QueryCatalogMsg volId: " << qryCat.volume_id
        << " blobName:  " << qryCat.blob_name;
    return oss.str();
}

std::string logString(const fpi::PutObjectMsg& putObj)
{
    std::ostringstream oss;
    oss << " PutObjectMsg for object " << ObjectID(putObj.data_obj_id.digest)
	<< " Volume UUID " << std::hex << putObj.volume_id << std::dec
	<< " Object length " << putObj.data_obj_len;
    return oss.str();
}

std::string logString(const fpi::OpenVolumeMsg &openVol)
{
    std::ostringstream oss;
    oss << " OpenVolumeMsg Vol Id: " << openVol.volume_id;
    return oss.str();
}

std::string logString(const fpi::VolumeGroupInfoUpdateCtrlMsg& msg)
{
    std::ostringstream oss;
    oss << " VolumeGroupInfoUpdateCtrlMsg Vol Id: " << msg.group.groupId;
    return oss.str();
}

std::string logString(const fpi::CloseVolumeMsg &closeVol)
{
    std::ostringstream oss;
    oss << " CloseVolumeMsg Vol Id: " << closeVol.volume_id;
    return oss.str();
}

std::string logString(const fpi::ReloadVolumeMsg & vol) {
    std::ostringstream oss;
    oss << " ReloadVolumeMsg Vol Id: " << vol.volume_id;
    return oss.str();
}

std::string logString(const fpi::CtrlNotifyDMStartMigrationMsg & vol) {
    std::ostringstream oss;
    oss << " CtrlNotifyDMStartMigrationMsg.  DMT version:  "<< vol.DMT_version
        << " # of volumes: " << vol.migrations.size();
    return oss.str();
}

std::string logString(const fpi::CtrlNotifyFinishMigrationMsg &msg)
{
    std::ostringstream oss;
    oss << " CtrlNotifyFinishMigrationMsg volId:  "<< msg.volume_id;
    return oss.str();
}

std::string logString(const fpi::CtrlNotifyInitialBlobFilterSetMsg &msg)
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

std::string logString(const fpi::CtrlObjectMetaDataPropagate& msg)
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

std::string logString(const fpi::CtrlObjectMetaDataSync& msg)
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

std::string logString(const fpi::PutObjectRspMsg& putObj)
{
    std::ostringstream oss;
    oss << " PutObjectRspMsg";
    return oss.str();
}

std::string logString(const fpi::UpdateCatalogMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogMsg TxId:" <<  updCat.txId
        << " volume_id" << updCat.volume_id << " blob_name: "<< updCat.blob_name
        << " opid: " << updCat.opId;
    return oss.str();
}

std::string logString(const fpi::UpdateCatalogRspMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogRspMsg";
    return oss.str();
}

std::string logString(const fpi::UpdateCatalogOnceMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogOnceMsg TxId:" << updCat.txId
        << " volume_id" << updCat.volume_id << " blob_name: "<< updCat.blob_name
        << " opid: " << updCat.opId;
    return oss.str();
}

std::string logString(const fpi::UpdateCatalogOnceRspMsg& updCat)
{
    std::ostringstream oss;
    oss << " UpdateCatalogOnceRspMsg";
    return oss.str();
}

std::string logString(const fpi::StartBlobTxMsg& stBlobTx)
{
    std::ostringstream oss;
    oss << " StartBlobTxMs TxId:" << stBlobTx.txId
        << " volume_id" << stBlobTx.volume_id << " blob_name: "<< stBlobTx.blob_name
        << " opid: " << stBlobTx.opId;
    return oss.str();
}

std::string logString(const fpi::CommitBlobTxMsg& commitBlbTx)
{
    std::ostringstream oss;
    oss << " CommitBlobTxMs TxId:" << commitBlbTx.txId
        << " volume_id" << commitBlbTx.volume_id << " blob_name: "<< commitBlbTx.blob_name
        << " opid: " << commitBlbTx.opId;
    return oss.str();
}

std::string logString(const fpi::SetBlobMetaDataMsg& setMDMsg)
{
    std::ostringstream oss;
    oss << " SetBlobMetaDataMsg TxId:" << setMDMsg.txId
        << " volume_id" << setMDMsg.volume_id << " blob_name: "<< setMDMsg.blob_name
        << " opid: " << setMDMsg.opId;
    return oss.str();
}

std::string logString(const fpi::SetBlobMetaDataRspMsg& setMDRspMsg)
{
    std::ostringstream oss;
    oss << " SetBlobMetaDataRspMsg";
    return oss.str();
}

std::string logString(const fpi::DeleteObjectMsg& delMsg)
{
    std::ostringstream oss;
    oss << " DeleteObjectMsg " << ObjectID(delMsg.objId.digest)
	<< " Volume UUID " << std::hex << delMsg.volId << std::dec;
    return oss.str();
}

std::string logString(const fpi::DeleteObjectRspMsg& delRspMsg)
{
    std::ostringstream oss;
    oss << " DeleteObjectRspMsg";
    return oss.str();
}

std::string logString(const fpi::AddObjectRefMsg& copyMsg)
{
    std::ostringstream oss;
    oss << " AddObjectRefMsg";
    return oss.str();
}

std::string logString(const fpi::AddObjectRefRspMsg& copyRspMsg)
{
    std::ostringstream oss;
    oss << " AddObjectRefRspMsg";
    return oss.str();
}

std::string logString(const fpi::AbortBlobTxMsg& abortBlbTx)
{
    std::ostringstream oss;
    // FIXME(DAC): This does nothing.
    oss << " AbortBlobTxMs";
    oss << " AbortBlobTxMs TxId:" << abortBlbTx.txId
        << " volume_id" << abortBlbTx.volume_id << " blob_name: "<< abortBlbTx.blob_name
        << " opid: " << abortBlbTx.opId;
    return oss.str();
}

std::string logString(const fpi::GetBlobMetaDataMsg& message)
{
    return "GetBlobMetaDataMsg";
}

std::string logString(const fpi::RenameBlobMsg& message)
{
    std::ostringstream oss;
    oss << "RenameBlobMsg " << " opid: " << message.opId;
    return oss.str();
}

std::string logString(const fpi::RenameBlobRespMsg& message)
{
    return "RenameBlobRespMsg";
}

std::string logString(const fpi::SetVolumeMetadataMsg& msg) {
    return "SetVolumeMetadataMsg";
}

std::string logString(const fpi::GetVolumeMetadataMsgRsp& msg) {
    return "GetVolumeMetadataMsgRsp";
}

std::string logString(const fpi::StatVolumeMsg& msg) {
    return "StatVolumeMsg";
}

std::string logString(const fpi::GetBucketMsg& msg) {
    std::ostringstream oss;
    oss << " GetBucketMsg(volume_id: " << msg.volume_id
            << ", count: " << msg.count
            << ", startPos: " << msg.startPos
            << ", pattern: " << msg.pattern << ")";
    return oss.str();
}

std::string logString(const fpi::GetBucketRspMsg& msg) {
    std::ostringstream oss;
    oss << " GetBucketRspMsg(count: " << msg.blob_descr_list.size() << ")";
    return oss.str();
}

std::string logString(const fpi::DeleteBlobMsg& msg) {
    std::ostringstream oss;
    oss << " DeleteBlobMsg ";
    oss << " DeleteBlobMsg TxId:" << msg.txId
        << " volume_id" << msg.volume_id << " blob_name: "<< msg.blob_name
        << " opid: " << msg.opId;
    return oss.str();
}

std::string logString(const fpi::GetVolumeMetadataMsg& msg) {
    std::ostringstream oss;
    oss << " GetVolumeMetadataMsg "
        << " volume_id: " << msg.volumeId;
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

DEFINE_OUTPUT_FUNCS(ActiveObjectsMsg) {
    out << "["
        << " token:" << msg.token
        << " file:" << msg.filename        
        << " checksum:" << msg.checksum
        << " volumes:" << msg.volumeIds.size() <<":";
    
    for (const auto& volId : msg.volumeIds) {
        out << volId << ",";
    }
    out << "]";
    return out;
}

DEFINE_OUTPUT_FUNCS(SvcUuid) {
    out << SvcMgr::mapToSvcUuidAndName(msg);
    return out;
}

DEFINE_OUTPUT_FUNCS(GenericCommandMsg) {
    out << "["
        << " command:" << msg.command
        << " args:" << msg.arg
        << "]";
    return out;
}

DEFINE_OUTPUT_FUNCS(LoadFromArchiveMsg) {
    out << "["
        << " file:" << msg.filename
        << " vol:" << msg.volId
        << "]";
    return out;
}

size_t sizeOfData(fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg) {
    size_t totalSize = 0;
    auto blobDescList = msg->blob_desc_list;
    for (auto bdlIt : blobDescList) {
        totalSize += sizeof(bdlIt.vol_blob_name);
        totalSize += sizeof(bdlIt.vol_blob_desc);
    }
    return (totalSize);
}

size_t sizeOfData(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg) {
    size_t totalSize = 0;
    auto blobList = msg->blob_obj_list;
    for (auto blobObj : blobList) {
        auto blob_diff_list = blobObj.blob_diff_list;
        for (auto blobObjInfo : blob_diff_list) {
            totalSize += sizeof(blobObjInfo.data_obj_id.digest);
        }
    }

    return (totalSize);
}

}  // namespace fds
