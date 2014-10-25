/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SVCMSGFACTORY_H_
#define SOURCE_INCLUDE_SVCMSGFACTORY_H_

#include <boost/shared_ptr.hpp>
#include <fds_types.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class PutObjectMsg;
class QueryCatalogMsg;
class UpdateCatalogMsg;
class UpdateCatalogOnceMsg;
class StartBlobTxMsg;
class AbortBlobTxMsg;
class DeleteCatalogObjectMsg;
class GetVolumeBlobListMsg;
class SetBlobMetaDataMsg;
class GetBlobMetaDataMsg;
class GetDmStatsMsg;
class GetVolumeMetaDataMsg;
class DeleteBlobMsg;
class CommitBlobTxMsg;
class AbortBlobTxMsg;
class FDSP_BlobObjectInfo;
class GetObjectMsg;
class GetBucketMsg;
typedef boost::shared_ptr<PutObjectMsg> PutObjectMsgPtr;
typedef boost::shared_ptr<QueryCatalogMsg> QueryCatalogMsgPtr;
typedef boost::shared_ptr<UpdateCatalogMsg> UpdateCatalogMsgPtr;
typedef boost::shared_ptr<UpdateCatalogOnceMsg> UpdateCatalogOnceMsgPtr;
typedef boost::shared_ptr<StartBlobTxMsg> StartBlobTxMsgPtr;
typedef boost::shared_ptr<AbortBlobTxMsg> AbortBlobTxMsgPtr;
typedef boost::shared_ptr<DeleteCatalogObjectMsg> DeleteCatalogObjectMsgPtr;
typedef boost::shared_ptr<GetVolumeBlobListMsg> GetVolumeBlobListMsgPtr;
typedef boost::shared_ptr<SetBlobMetaDataMsg> SetBlobMetaDataMsgPtr;
typedef boost::shared_ptr<GetBlobMetaDataMsg> GetBlobMetaDataMsgPtr;
typedef boost::shared_ptr<GetDmStatsMsg> GetDmStatsMsgPtr;
typedef boost::shared_ptr<GetVolumeMetaDataMsg> GetVolumeMetaDataMsgPtr;
typedef boost::shared_ptr<DeleteBlobMsg> DeleteBlobMsgPtr;
typedef boost::shared_ptr<CommitBlobTxMsg> CommitBlobTxMsgPtr;
typedef boost::shared_ptr<AbortBlobTxMsg> AbortBlobTxMsgPtr;
typedef boost::shared_ptr<FDSP_BlobObjectInfo> FDSP_BlobObjectInfoPtr;
typedef boost::shared_ptr<GetObjectMsg> GetObjectMsgPtr;
typedef boost::shared_ptr<GetBucketMsg> GetBucketMsgPtr;
}

namespace fds {
/* Forward declarations */
class DataGenIf;
typedef boost::shared_ptr<DataGenIf> DataGenIfPtr;

void UpdateBlobInfo(FDS_ProtocolInterface::UpdateCatalogMsgPtr  updateCatMsg,
                          DataGenIfPtr dataGen, size_t blobSize);

template<typename T>
void
UpdateBlobInfoNoData(boost::shared_ptr<T> updateCatMsg, size_t objSize, size_t blobSize);
/*
void
UpdateBlobInfoNoData(FDS_ProtocolInterface::UpdateCatalogMsgPtr  updateCatMsg,
                            size_t objSize, size_t blobSize);
*/
namespace apis {
class VolumeSettings;
}

/**
* @brief Factory for generating service messages as well as useful fdsp/xdi objects
*/
struct SvcMsgFactory {
    static FDS_ProtocolInterface::PutObjectMsgPtr
        newPutObjectMsg(const uint64_t& volId, DataGenIfPtr dataGen);
    static FDS_ProtocolInterface::QueryCatalogMsgPtr
        newQueryCatalogMsg(const uint64_t& volId, DataGenIfPtr dataGen);
    static FDS_ProtocolInterface::QueryCatalogMsgPtr
        newQueryCatalogMsg(const uint64_t& volId, const std::string blobName, const uint64_t& blobOffset );
    static FDS_ProtocolInterface::UpdateCatalogMsgPtr
        newUpdateCatalogMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::UpdateCatalogOnceMsgPtr
        newUpdateCatalogOnceMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::StartBlobTxMsgPtr
        newStartBlobTxMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::CommitBlobTxMsgPtr
        newCommitBlobTxMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::AbortBlobTxMsgPtr
        newAbortBlobTxMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::DeleteCatalogObjectMsgPtr
        newDeleteCatalogObjectMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::SetBlobMetaDataMsgPtr
        newSetBlobMetaDataMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::GetBlobMetaDataMsgPtr
        newGetBlobMetaDataMsg(const uint64_t& volId, const std::string blobName);
    static FDS_ProtocolInterface::GetBucketMsgPtr
        newGetBucketMsg(const uint64_t& volId, const uint64_t& startPos);
    static FDS_ProtocolInterface::GetObjectMsgPtr
        newGetObjectMsg(const uint64_t& volId, const ObjectID& objId);

    static apis::VolumeSettings defaultS3VolSettings();
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_SVCMSGFACTORY_H_
