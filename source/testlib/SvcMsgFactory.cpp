/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <fdsp/ConfigurationService.h>
#include <fdsp/dm_api_types.h>
#include <fdsp/sm_api_types.h>
#include <DataGen.hpp>
#include <boost/make_shared.hpp>
#include <SvcMsgFactory.h>

namespace fds {

template void UpdateBlobInfoNoData(fpi::UpdateCatalogMsgPtr updateCat, size_t objSize,
        size_t blobSize);
template void UpdateBlobInfoNoData(fpi::UpdateCatalogOnceMsgPtr updateCat, size_t objSize,
        size_t blobSize);

fpi::PutObjectMsgPtr
SvcMsgFactory::newPutObjectToSmMsg(const uint64_t& volId,
                                   const ObjectID& objId,
                                   boost::shared_ptr<std::string> objData)
{
    fpi::PutObjectMsgPtr putObjMsg(new fpi::PutObjectMsg);
    putObjMsg->volume_id = volId;
    putObjMsg->data_obj = *objData;
    putObjMsg->data_obj_len = objData->length();
    fds::assign(putObjMsg->data_obj_id, objId);
    return putObjMsg;
}

fpi::PutObjectMsgPtr
SvcMsgFactory::newPutObjectMsg(const uint64_t& volId, DataGenIfPtr dataGen)
{
    auto data = dataGen->nextItem();
    auto objId = ObjIdGen::genObjectId(data->c_str(), data->length());
    fpi::PutObjectMsgPtr putObjMsg(new fpi::PutObjectMsg);
    putObjMsg->volume_id = volId;
    putObjMsg->data_obj = *data;
    putObjMsg->data_obj_len = data->length();
    fds::assign(putObjMsg->data_obj_id, objId);
    return putObjMsg;
}

template<typename T>
void
UpdateBlobInfoNoData(boost::shared_ptr<T> updateCat, size_t objSize, size_t blobSize)
{
    for (fds_uint64_t blobOffset = 0; blobOffset < blobSize; blobOffset += objSize) {
            std::string data = updateCat->blob_name + std::to_string(blobOffset) +
                    std::to_string(util::getTimeStampNanos());

         ObjectID objId = ObjIdGen::genObjectId(data.c_str(), data.size());
         fds_uint64_t sz = ((blobOffset + objSize) < blobSize ? objSize
                    : (blobSize - blobOffset));
         FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
         updBlobInfo.offset   = blobOffset;
         updBlobInfo.size     = sz;
         updBlobInfo.data_obj_id.digest =
                   std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
         updateCat->obj_list.push_back(updBlobInfo);
        }
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
         updBlobInfo.data_obj_id.digest =
                   std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
         updateCatMsg->obj_list.push_back(updBlobInfo);
         blobOffset += data->length();
    }
}

fpi::GetObjectMsgPtr
SvcMsgFactory::newGetObjectMsg(const uint64_t& volId, const ObjectID& objId)
{
    fpi::GetObjectMsgPtr getObjMsg = boost::make_shared<fpi::GetObjectMsg>();
    getObjMsg->volume_id = volId;
    assign(getObjMsg->data_obj_id, objId);
    return getObjMsg;
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
    queryCatMsg->start_offset = blobOffset;
    queryCatMsg->end_offset = -1;
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

fpi::GetBucketMsgPtr
SvcMsgFactory::newGetBucketMsg(const uint64_t& volId, const uint64_t& start_pos)
{
    fpi::GetBucketMsgPtr  getBucket(new fpi::GetBucketMsg);
    getBucket->volume_id = volId;
    getBucket->startPos = start_pos;
    return getBucket;
}

fpi::GetDmStatsMsgPtr
SvcMsgFactory::newGetDmStatsMsg(const uint64_t& volId)
{
    fpi::GetDmStatsMsgPtr  getDmStats(new fpi::GetDmStatsMsg);
    getDmStats->volume_id = volId;
    return getDmStats;
}

fpi::DeleteBlobMsgPtr
SvcMsgFactory::newDeleteBlobMsg(const uint64_t& volId, const std::string blobName)
{
    fpi::DeleteBlobMsgPtr  delBlob(new fpi::DeleteBlobMsg);
    delBlob->volume_id = volId;
    delBlob->blob_name = blobName;
    delBlob->blob_version = blob_version_invalid;
    return delBlob;
}

}  // namespace fds
