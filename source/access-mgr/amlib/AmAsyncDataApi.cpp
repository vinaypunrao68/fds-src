/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "fds_process.h"
#include <fiu-control.h>
#include <util/fiu_util.h>

#include "AmAsyncDataApi.h"
#include "StorHvCtrl.h"
#include "requests/requests.h"
#include "AsyncResponseHandlers.h"

namespace fds {

boost::shared_ptr<apis::BlobDescriptor>
transform_descriptor(boost::shared_ptr<BlobDescriptor> descriptor) {
    auto retBlobDesc = boost::make_shared<apis::BlobDescriptor>();
    retBlobDesc->name = descriptor->getBlobName();
    retBlobDesc->byteCount = descriptor->getBlobSize();

    for (const_kv_iterator it = descriptor->kvMetaBegin();
         it != descriptor->kvMetaEnd();
         ++it) {
        retBlobDesc->metadata[it->first] = it->second;
    }
    return retBlobDesc;
}

template<typename H>
AmAsyncDataApi<H>::AmAsyncDataApi(response_ptr response_api)
    : responseApi(response_api)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    if (conf.get<fds_bool_t>("testing.uturn_amserv_all")) {
        fiu_enable("am.uturn.api", 1, NULL, 0);
        LOGNOTIFY << "Enabled AM API uturn";
    }
}

template<typename H>
AmAsyncDataApi<H>::AmAsyncDataApi(response_api_type* response_api)
    : AmAsyncDataApi(response_ptr(response_api))
{ }

template<typename H>
void AmAsyncDataApi<H>::attachVolume(boost::shared_ptr<H>& requestId,
                             boost::shared_ptr<std::string>& domainName,
                             boost::shared_ptr<std::string>& volumeName) {
    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<AttachCallback>>(
        [this, requestId](AttachCallback* cb, Error const& e) mutable -> void {
            responseApi->attachVolumeResp(e, requestId);
        });
    storHvisor->enqueueAttachReq(*volumeName,
                                 SHARED_DYN_CAST(Callback, callback));
}

template<typename H>
void AmAsyncDataApi<H>::volumeStatus(boost::shared_ptr<H>& requestId,
                             boost::shared_ptr<std::string>& domainName,
                             boost::shared_ptr<std::string>& volumeName) {
    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<GetVolumeMetaDataCallback>>(
        [this, requestId](GetVolumeMetaDataCallback* cb, Error const& e) mutable -> void {
            boost::shared_ptr<apis::VolumeStatus> volume_status;
            if (e.ok()) {
                volume_status = boost::make_shared<apis::VolumeStatus>();
                volume_status->blobCount = cb->volumeMetaData.blobCount;
                volume_status->currentUsageInBytes = cb->volumeMetaData.size;
            }
            responseApi->volumeStatusResp(e, requestId, volume_status);
        });
    AmRequest *blobReq = new GetVolumeMetaDataReq(invalid_vol_id,
                                                   *volumeName,
                                                   SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::volumeContents(boost::shared_ptr<H>& requestId,
                               boost::shared_ptr<std::string>& domainName,
                               boost::shared_ptr<std::string>& volumeName,
                               boost::shared_ptr<int32_t>& count,
                               boost::shared_ptr<int64_t>& offset) {
    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<GetBucketCallback>>(
        [this, requestId](GetBucketCallback* cb, Error const& e) mutable -> void {
            responseApi->volumeContentsResp(e, requestId, cb->vecBlobs);
        });
    AmRequest *blobReq = new VolumeContentsReq(invalid_vol_id,
                                               *volumeName,
                                               *count,
                                               SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::statBlob(boost::shared_ptr<H>& requestId,
                         boost::shared_ptr<std::string>& domainName,
                         boost::shared_ptr<std::string>& volumeName,
                         boost::shared_ptr<std::string>& blobName) {
    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<StatBlobCallback>>(
            [this, requestId](StatBlobCallback* cb, Error const& e) mutable -> void {
            // TODO(bszmyd): Tue 16 Dec 2014 08:06:47 PM MST
            // Unfortunately we have to transform meta-data received
            // from the DataManager. We should fix that.
            boost::shared_ptr<apis::BlobDescriptor> retBlobDesc = e.ok() ?
                transform_descriptor(cb->blobDesc) : nullptr;
            responseApi->statBlobResp(e, requestId, retBlobDesc);
        });
    AmRequest *blobReq = new StatBlobReq(invalid_vol_id,
                                         *volumeName,
                                         *blobName,
                                         SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::startBlobTx(boost::shared_ptr<H>& requestId,
                            boost::shared_ptr<std::string>& domainName,
                            boost::shared_ptr<std::string>& volumeName,
                            boost::shared_ptr<std::string>& blobName,
                            boost::shared_ptr<fds_int32_t>& blobMode) {
    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<StartBlobTxCallback>>(
        [this, requestId](StartBlobTxCallback* cb, Error const& e) mutable -> void {
            auto txDesc = boost::make_shared<apis::TxDescriptor>();
            txDesc->txId = cb->blobTxId.getValue();
            responseApi->startBlobTxResp(e, requestId, txDesc);
        });
    AmRequest *blobReq = new StartBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             *blobMode,
                                             SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::commitBlobTx(boost::shared_ptr<H>& requestId,
                             boost::shared_ptr<std::string>& domainName,
                             boost::shared_ptr<std::string>& volumeName,
                             boost::shared_ptr<std::string>& blobName,
                             boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<CommitBlobTxCallback>>(
        [this, requestId](CommitBlobTxCallback* cb, Error const& e) mutable -> void {
            responseApi->commitBlobTxResp(e, requestId);
        });
    AmRequest *blobReq = new CommitBlobTxReq(invalid_vol_id,
                                              *volumeName,
                                              *blobName,
                                              blobTxDesc,
                                              SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::abortBlobTx(boost::shared_ptr<H>& requestId,
                            boost::shared_ptr<std::string>& domainName,
                            boost::shared_ptr<std::string>& volumeName,
                            boost::shared_ptr<std::string>& blobName,
                            boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<AbortBlobTxCallback>>(
        [this, requestId](AbortBlobTxCallback* cb, Error const& e) mutable -> void {
            responseApi->abortBlobTxResp(e, requestId);
        });
    AmRequest *blobReq = new AbortBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             blobTxDesc,
                                             SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::getBlob(boost::shared_ptr<H>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<int32_t>& length,
                        boost::shared_ptr<apis::ObjectOffset>& objectOffset) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<GetObjectCallback>>(
        [this, requestId](GetObjectCallback* cb, Error const& e) mutable -> void {
            responseApi->getBlobResp(e, requestId, cb->returnBuffer, cb->returnSize);
        });
    AmRequest *blobReq= new GetBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        SHARED_DYN_CAST(Callback, callback),
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::getBlobWithMeta(boost::shared_ptr<H>& requestId,
                                boost::shared_ptr<std::string>& domainName,
                                boost::shared_ptr<std::string>& volumeName,
                                boost::shared_ptr<std::string>& blobName,
                                boost::shared_ptr<int32_t>& length,
                                boost::shared_ptr<apis::ObjectOffset>& objectOffset) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<GetObjectWithMetadataCallback>>(
        [this, requestId](GetObjectWithMetadataCallback* cb, Error const& e) mutable -> void {
            boost::shared_ptr<apis::BlobDescriptor> retBlobDesc = e.ok() ?
                transform_descriptor(cb->blobDesc) : nullptr;
            responseApi->getBlobWithMetaResp(e,
                                             requestId,
                                             cb->returnBuffer,
                                             cb->returnSize,
                                             retBlobDesc);
        });
    GetBlobReq *blobReq= new GetBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        SHARED_DYN_CAST(Callback, callback),
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length);
    blobReq->get_metadata = true;
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::updateMetadata(boost::shared_ptr<H>& requestId,
                               boost::shared_ptr<std::string>& domainName,
                               boost::shared_ptr<std::string>& volumeName,
                               boost::shared_ptr<std::string>& blobName,
                               boost::shared_ptr<apis::TxDescriptor>& txDesc,
                               boost::shared_ptr< std::map<std::string, std::string> >& metadata) {
    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<UpdateMetadataCallback>>(
        [this, requestId](UpdateMetadataCallback* cb, Error const& e) mutable -> void {
            responseApi->updateMetadataResp(e, requestId);
        });
    boost::shared_ptr<fpi::FDSP_MetaDataList> metaDataList(new fpi::FDSP_MetaDataList());
    LOGDEBUG << "received updateMetadata cmd";
    fpi::FDSP_MetaDataPair metaPair;
    for (auto const & meta : *metadata) {
        LOGDEBUG << meta.first << ":" << meta.second;
        metaPair.key = meta.first;
        metaPair.value = meta.second;
        metaDataList->push_back(metaPair);
    }
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    AmRequest *blobReq = new SetBlobMetaDataReq(invalid_vol_id,
                                                 *volumeName,
                                                 *blobName,
                                                 blobTxDesc,
                                                 metaDataList,
                                                 SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::updateBlobOnce(boost::shared_ptr<H>& requestId,
                               boost::shared_ptr<std::string>& domainName,
                               boost::shared_ptr<std::string>& volumeName,
                               boost::shared_ptr<std::string>& blobName,
                               boost::shared_ptr<fds_int32_t>& blobMode,
                               boost::shared_ptr<std::string>& bytes,
                               boost::shared_ptr<int32_t>& length,
                               boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                               boost::shared_ptr< std::map<std::string, std::string> >& metadata) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<UpdateBlobCallback>>(
        [this, requestId](UpdateBlobCallback* cb, Error const& e) mutable -> void {
            responseApi->updateBlobResp(e, requestId);
        });
    AmRequest *blobReq = new PutBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length,
                                        bytes,
                                        *blobMode,
                                        metadata,
                                        callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::updateBlob(boost::shared_ptr<H>& requestId,
                           boost::shared_ptr<std::string>& domainName,
                           boost::shared_ptr<std::string>& volumeName,
                           boost::shared_ptr<std::string>& blobName,
                           boost::shared_ptr<apis::TxDescriptor>& txDesc,
                           boost::shared_ptr<std::string>& bytes,
                           boost::shared_ptr<int32_t>& length,
                           boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                           boost::shared_ptr<bool>& isLast) {
    BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
        txDesc->txId));

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<UpdateBlobCallback>>(
        [this, requestId](UpdateBlobCallback* cb, Error const& e) mutable -> void {
            responseApi->updateBlobResp(e, requestId);
        });
    AmRequest *blobReq = new PutBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length,
                                        bytes,
                                        blobTxDesc,
                                        *isLast,
                                        &bucket_ctx,
                                        NULL,
                                        NULL,
                                        callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::deleteBlob(boost::shared_ptr<H>& requestId,
                           boost::shared_ptr<std::string>& domainName,
                           boost::shared_ptr<std::string>& volumeName,
                           boost::shared_ptr<std::string>& blobName,
                           boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    BlobTxId::ptr blobTxId(new BlobTxId(txDesc->txId));

    // Closure for response call
    auto callback = boost::make_shared<AsyncResponseHandler<DeleteBlobCallback>>(
        [this, requestId](DeleteBlobCallback* cb, Error const& e) mutable -> void {
            responseApi->deleteBlobResp(e, requestId);
        });
    AmRequest *blobReq = new DeleteBlobReq(invalid_vol_id,
                                           *blobName,
                                           *volumeName,
                                           blobTxId,
                                           SHARED_DYN_CAST(Callback, callback));
    storHvisor->enqueueBlobReq(blobReq);
}

// Explicit Instantiation of Templates
template class AmAsyncDataApi<apis::RequestId>;
template class AmAsyncDataApi<std::pair<fds_int64_t, fds_int32_t>>;
}  // namespace fds
