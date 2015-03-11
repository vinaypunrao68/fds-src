/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_IMPL_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_IMPL_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "fds_process.h"
#include <fiu-control.h>
#include <util/fiu_util.h>

#include "StorHvCtrl.h"
#include "requests/requests.h"
#include "AsyncResponseHandlers.h"

namespace fds {

extern boost::shared_ptr<fpi::BlobDescriptor>
    transform_descriptor(boost::shared_ptr<BlobDescriptor> descriptor);

template<typename T, typename C>
CallbackPtr
create_async_handler(C&& c)
{ return boost::make_shared<AsyncResponseHandler<T, C>>(std::forward<C>(c)); }

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
void AmAsyncDataApi<H>::attachVolume(H& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](AttachCallback* cb, Error const& e) mutable -> void {
        p->attachVolumeResp(e, requestId);
    };

    auto callback = create_async_handler<AttachCallback>(std::move(closure));

    storHvisor->enqueueAttachReq(*volumeName,
                                 callback);
}

template<typename H>
void AmAsyncDataApi<H>::volumeStatus(H& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](GetVolumeMetaDataCallback* cb, Error const& e) mutable -> void {
        typename response_api_type::shared_status_type volume_status;
        if (e.ok()) {
            volume_status = boost::make_shared<apis::VolumeStatus>();
            volume_status->blobCount = cb->volumeMetaData.blobCount;
            volume_status->currentUsageInBytes = cb->volumeMetaData.size;
        }
        p->volumeStatusResp(e, requestId, volume_status);
    };

    auto callback = create_async_handler<GetVolumeMetaDataCallback>(std::move(closure));

    AmRequest *blobReq = new GetVolumeMetaDataReq(invalid_vol_id,
                                                  *volumeName,
                                                  callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::volumeContents(H& requestId,
                                       shared_string_type& domainName,
                                       shared_string_type& volumeName,
                                       shared_int_type& count,
                                       shared_size_type& offset,
                                       shared_string_type& pattern,
                                       boost::shared_ptr<fpi::BlobListOrder>& orderBy,
                                       shared_bool_type& descending) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](GetBucketCallback* cb, Error const& e) mutable -> void {
        p->volumeContentsResp(e, requestId, cb->vecBlobs);
    };

    auto callback = create_async_handler<GetBucketCallback>(std::move(closure));

    AmRequest *blobReq = new VolumeContentsReq(invalid_vol_id,
                                               *volumeName,
                                               *count,
                                               *offset,
                                               *pattern,
                                               *orderBy,
                                               *descending,
                                               callback);
    storHvisor->enqueueBlobReq(blobReq);
}
template<typename H>
void AmAsyncDataApi<H>::statBlob(H& requestId,
                                 shared_string_type& domainName,
                                 shared_string_type& volumeName,
                                 shared_string_type& blobName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](StatBlobCallback* cb, Error const& e) mutable -> void {
        // TODO(bszmyd): Tue 16 Dec 2014 08:06:47 PM MST
        // Unfortunately we have to transform meta-data received
        // from the DataManager. We should fix that.
        typename response_api_type::shared_descriptor_type retBlobDesc = e.ok() ?
            transform_descriptor(cb->blobDesc) : nullptr;
        p->statBlobResp(e, requestId, retBlobDesc);
    };

    auto callback = create_async_handler<StatBlobCallback>(std::move(closure));

    AmRequest *blobReq = new StatBlobReq(invalid_vol_id,
                                         *volumeName,
                                         *blobName,
                                         callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::startBlobTx(H& requestId,
                                    shared_string_type& domainName,
                                    shared_string_type& volumeName,
                                    shared_string_type& blobName,
                                    shared_int_type& blobMode) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](StartBlobTxCallback* cb, Error const& e) mutable -> void {
        auto txDesc = boost::make_shared<apis::TxDescriptor>();
        txDesc->txId = cb->blobTxId.getValue();
        p->startBlobTxResp(e, requestId, txDesc);
    };

    auto callback = create_async_handler<StartBlobTxCallback>(std::move(closure));

    AmRequest *blobReq = new StartBlobTxReq(invalid_vol_id,
                                            *volumeName,
                                            *blobName,
                                            *blobMode,
                                            callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::commitBlobTx(H& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName,
                                     shared_string_type& blobName,
                                     shared_tx_ctx_type& txDesc) {
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](CommitBlobTxCallback* cb, Error const& e) mutable -> void {
        p->commitBlobTxResp(e, requestId);
    };

    auto callback = create_async_handler<CommitBlobTxCallback>(std::move(closure));

    AmRequest *blobReq = new CommitBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             blobTxDesc,
                                             callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::abortBlobTx(H& requestId,
                                    shared_string_type& domainName,
                                    shared_string_type& volumeName,
                                    shared_string_type& blobName,
                                    shared_tx_ctx_type& txDesc) {
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](AbortBlobTxCallback* cb, Error const& e) mutable -> void {
        p->abortBlobTxResp(e, requestId);
    };

    auto callback = create_async_handler<AbortBlobTxCallback>(std::move(closure));

    AmRequest *blobReq = new AbortBlobTxReq(invalid_vol_id,
                                            *volumeName,
                                            *blobName,
                                            blobTxDesc,
                                            callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::getBlob(H& requestId,
                                shared_string_type& domainName,
                                shared_string_type& volumeName,
                                shared_string_type& blobName,
                                shared_int_type& length,
                                shared_offset_type& objectOffset) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto closure = [p = responseApi, requestId](GetObjectCallback* cb, Error const& e) mutable -> void {
        p->getBlobResp(e, requestId, cb->returnBuffer, cb->returnSize);
    };

    auto callback = create_async_handler<GetObjectCallback>(std::move(closure));

    AmRequest *blobReq= new GetBlobReq(invalid_vol_id,
                                       *volumeName,
                                       *blobName,
                                       callback,
                                       static_cast<fds_uint64_t>(objectOffset->value),
                                       *length);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::getBlobWithMeta(H& requestId,
                                        shared_string_type& domainName,
                                        shared_string_type& volumeName,
                                        shared_string_type& blobName,
                                        shared_int_type& length,
                                        shared_offset_type& objectOffset) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto closure = [p = responseApi, requestId](GetObjectWithMetadataCallback* cb, Error const& e) mutable -> void {
        typename response_api_type::shared_descriptor_type retBlobDesc = e.ok() ?
            transform_descriptor(cb->blobDesc) : nullptr;
        p->getBlobWithMetaResp(e,
                                         requestId,
                                         cb->returnBuffer,
                                         cb->returnSize,
                                         retBlobDesc);
    };

    auto callback = create_async_handler<GetObjectWithMetadataCallback>(std::move(closure));

    GetBlobReq *blobReq= new GetBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        callback,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length);
    blobReq->get_metadata = true;
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::updateMetadata(H& requestId,
                                       shared_string_type& domainName,
                                       shared_string_type& volumeName,
                                       shared_string_type& blobName,
                                       shared_tx_ctx_type& txDesc,
                                       shared_meta_type& metadata) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](UpdateMetadataCallback* cb, Error const& e) mutable -> void {
        p->updateMetadataResp(e, requestId);
    };

    auto callback = create_async_handler<UpdateMetadataCallback>(std::move(closure));

    boost::shared_ptr<fpi::FDSP_MetaDataList> metaDataList(new fpi::FDSP_MetaDataList());
    LOGDEBUG << "received updateMetadata cmd";
    fpi::FDSP_MetaDataPair metaPair;
    for (auto const & meta : *metadata) {
        LOGDEBUG << meta.first << ":" << meta.second;
        metaPair.key = meta.first;
        metaPair.value = meta.second;
        metaDataList->push_back(metaPair);
    }
    // Setup the transaction descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

    AmRequest *blobReq = new SetBlobMetaDataReq(invalid_vol_id,
                                                *volumeName,
                                                *blobName,
                                                blobTxDesc,
                                                metaDataList,
                                                callback);
    storHvisor->enqueueBlobReq(blobReq);
}

template<typename H>
void AmAsyncDataApi<H>::updateBlobOnce(H& requestId,
                                       shared_string_type& domainName,
                                       shared_string_type& volumeName,
                                       shared_string_type& blobName,
                                       shared_int_type& blobMode,
                                       shared_buffer_type& bytes,
                                       shared_int_type& length,
                                       shared_offset_type& objectOffset,
                                       shared_meta_type& metadata) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto closure = [p = responseApi, requestId](UpdateBlobCallback* cb, Error const& e) mutable -> void {
        p->updateBlobResp(e, requestId);
    };

    auto callback = create_async_handler<UpdateBlobCallback>(std::move(closure));

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
void AmAsyncDataApi<H>::updateBlob(H& requestId,
                                   shared_string_type& domainName,
                                   shared_string_type& volumeName,
                                   shared_string_type& blobName,
                                   shared_tx_ctx_type& txDesc,
                                   shared_buffer_type& bytes,
                                   shared_int_type& length,
                                   shared_offset_type& objectOffset,
                                   shared_bool_type& isLast) {
    BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](UpdateBlobCallback* cb, Error const& e) mutable -> void {
        p->updateBlobResp(e, requestId);
    };

    auto callback = create_async_handler<UpdateBlobCallback>(std::move(closure));

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
void AmAsyncDataApi<H>::deleteBlob(H& requestId,
                                   shared_string_type& domainName,
                                   shared_string_type& volumeName,
                                   shared_string_type& blobName,
                                   shared_tx_ctx_type& txDesc) {
    BlobTxId::ptr blobTxId(new BlobTxId(txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](DeleteBlobCallback* cb, Error const& e) mutable -> void {
        p->deleteBlobResp(e, requestId);
    };

    auto callback = create_async_handler<DeleteBlobCallback>(std::move(closure));

    AmRequest *blobReq = new DeleteBlobReq(invalid_vol_id,
                                           *blobName,
                                           *volumeName,
                                           blobTxId,
                                           callback);
    storHvisor->enqueueBlobReq(blobReq);
}

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_IMPL_H_
