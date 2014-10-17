/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <apis/ConfigurationService.h>
#include <DataGen.hpp>
#include <SvcMsgFactory.h>

namespace fds {

fpi::PutObjectMsgPtr
SvcMsgFactory::newPutObjectMsg(const uint64_t& volId, DataGenIfPtr dataGen)
{
    auto data = dataGen->nextItem();
    auto objId = ObjIdGen::genObjectId(data->c_str(), data->length());
    fpi::PutObjectMsgPtr putObjMsg(new fpi::PutObjectMsg);
    putObjMsg->volume_id = volId;
    putObjMsg->origin_timestamp = fds::util::getTimeStampMillis();
    putObjMsg->data_obj = *data;
    putObjMsg->data_obj_len = data->length();
    fds::assign(putObjMsg->data_obj_id, objId);
    return putObjMsg;
}


void
UpdateBlobInfo(FDS_ProtocolInterface::UpdateCatalogMsgPtr  updateCatMsg,
                          DataGenIfPtr dataGen, size_t blobSize)
{
        uint64_t blobOffset = 0;
        while (dataGen->hasNext()) {
             auto data = dataGen->nextItem();
             auto objId = ObjIdGen::genObjectId(data->c_str(), data->length());
             FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
             updBlobInfo.offset   = blobOffset;
             updBlobInfo.size     = blobSize;
             updBlobInfo.blob_end = dataGen->hasNext() == false;
             updBlobInfo.data_obj_id.digest =
                   std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
             updateCatMsg->obj_list.push_back(updBlobInfo);
             blobOffset += data->length();
       }
}

apis::VolumeSettings SvcMsgFactory::defaultS3VolSettings()
{
    apis::VolumeSettings vs;
    vs.maxObjectSizeInBytes = 2 << 20;
    vs.volumeType = apis::OBJECT;
    return vs;
}

fpi::QueryCatalogMsgPtr
SvcMsgFactory::newQueryCatalogMsg(const uint64_t& volId,
             const std::string blobName, const uint64_t& blobOffset )
{
    fpi::QueryCatalogMsgPtr queryCatMsg(new fpi::QueryCatalogMsg);
    queryCatMsg->volume_id = volId;
    queryCatMsg->blob_name = blobName;
    queryCatMsg->blob_offset = blobOffset;
    queryCatMsg->blob_version = blob_version_invalid;
    return queryCatMsg;
}

fpi::UpdateCatalogMsgPtr
SvcMsgFactory::newUpdateCatalogMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::UpdateCatalogMsgPtr updCatMsg(new fpi::UpdateCatalogMsg);
    updCatMsg->volume_id = volId;
    updCatMsg->blob_name = blobName;
    updCatMsg->blob_version = blob_version_invalid;
    return updCatMsg;
}

fpi::UpdateCatalogOnceMsgPtr
SvcMsgFactory::newUpdateCatalogOnceMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::UpdateCatalogOnceMsgPtr updCatOnceMsg(new fpi::UpdateCatalogOnceMsg);
    updCatOnceMsg->volume_id = volId;
    updCatOnceMsg->blob_name = blobName;
    updCatOnceMsg->blob_version = blob_version_invalid;
    return updCatOnceMsg;
}

fpi::StartBlobTxMsgPtr
SvcMsgFactory::newStartBlobTxMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::StartBlobTxMsgPtr  startBlbTx(new fpi::StartBlobTxMsg);
    startBlbTx->volume_id = volId;
    startBlbTx->blob_name = blobName;
    startBlbTx->blob_version = blob_version_invalid;
    return startBlbTx;
}

fpi::CommitBlobTxMsgPtr
SvcMsgFactory::newCommitBlobTxMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::CommitBlobTxMsgPtr  commitBlbTx(new fpi::CommitBlobTxMsg);
    commitBlbTx->volume_id = volId;
    commitBlbTx->blob_name = blobName;
    commitBlbTx->blob_version = blob_version_invalid;
    return commitBlbTx;
}

fpi::AbortBlobTxMsgPtr
SvcMsgFactory::newAbortBlobTxMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::AbortBlobTxMsgPtr  abortBlbTx(new fpi::AbortBlobTxMsg);
    abortBlbTx->volume_id = volId;
    abortBlbTx->blob_name = blobName;
    abortBlbTx->blob_version = blob_version_invalid;
    return abortBlbTx;
}

fpi::DeleteCatalogObjectMsgPtr
SvcMsgFactory::newDeleteCatalogObjectMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::DeleteCatalogObjectMsgPtr  deleteBlbTx(new fpi::DeleteCatalogObjectMsg);
    deleteBlbTx->volume_id = volId;
    deleteBlbTx->blob_name = blobName;
    deleteBlbTx->blob_version = blob_version_invalid;
    return deleteBlbTx;
}

fpi::SetBlobMetaDataMsgPtr
SvcMsgFactory::newSetBlobMetaDataMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::SetBlobMetaDataMsgPtr  setBlobMeta(new fpi::SetBlobMetaDataMsg);
    setBlobMeta->volume_id = volId;
    setBlobMeta->blob_name = blobName;
    setBlobMeta->blob_version = blob_version_invalid;
    return setBlobMeta;
}

fpi::GetBlobMetaDataMsgPtr
SvcMsgFactory::newGetBlobMetaDataMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::GetBlobMetaDataMsgPtr  getBlobMeta(new fpi::GetBlobMetaDataMsg);
    getBlobMeta->volume_id = volId;
    getBlobMeta->blob_name = blobName;
    getBlobMeta->blob_version = blob_version_invalid;
    return getBlobMeta;
}

}  // namespace fds
