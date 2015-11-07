/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include "AmAsyncDataApi.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "fdsp/common_types.h"
#include "fds_process.h"
#include <fiu-control.h>
#include <util/fiu_util.h>

#include "requests/requests.h"
#include "AsyncResponseHandlers.h"
#include "AmProcessor.h"

namespace fds {

template<typename T, typename C>
CallbackPtr
create_async_handler(C&& c)
{ return boost::make_shared<AsyncResponseHandler<T, C>>(std::forward<C>(c)); }

AmAsyncDataApi::AmAsyncDataApi(processor_type processor,
                                  response_ptr response_api)
:   amProcessor(processor),
    responseApi(response_api)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    if (conf.get<fds_bool_t>("testing.uturn_amserv_all")) {
        fiu_enable("am.uturn.api", 1, NULL, 0);
        LOGNOTIFY << "Enabled AM API uturn";
    }
}

AmAsyncDataApi::AmAsyncDataApi(processor_type processor,
                                  response_api_type* response_api)
: AmAsyncDataApi(processor, response_ptr(response_api))
{ }

void AmAsyncDataApi::attachVolume(RequestHandle const& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName,
                                     shared_vol_mode_type& mode) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](AttachCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->attachVolumeResp(e, requestId, cb->volDesc, cb->mode);
    };

    auto callback = create_async_handler<AttachCallback>(std::move(closure));

    AmRequest *blobReq = new AttachVolumeReq(invalid_vol_id,
                                             *volumeName,
                                             *mode,
                                             callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::detachVolume(RequestHandle const& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](DetachCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->detachVolumeResp(e, requestId);
    };

    auto callback = create_async_handler<DetachCallback>(std::move(closure));

    AmRequest *blobReq = new DetachVolumeReq(invalid_vol_id, *volumeName, callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::volumeStatus(RequestHandle const& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](StatVolumeCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        typename response_api_type::shared_status_type volume_status;
        if (fpi::OK == e) {
            volume_status = boost::make_shared<apis::VolumeStatus>();
            volume_status->blobCount = cb->blob_count;
            volume_status->currentUsageInBytes = cb->current_usage_bytes;
        }
        p->volumeStatusResp(e, requestId, volume_status);
    };

    auto callback = create_async_handler<StatVolumeCallback>(std::move(closure));

    AmRequest *blobReq = new StatVolumeReq(invalid_vol_id,
                                           *volumeName,
                                           callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::volumeContents(RequestHandle const& requestId,
                                       shared_string_type& domainName,
                                       shared_string_type& volumeName,
                                       shared_int_type& count,
                                       shared_size_type& offset,
                                       shared_string_type& pattern,
                                       boost::shared_ptr<fpi::PatternSemantics>& patternSem,
                                       boost::shared_ptr<fpi::BlobListOrder>& orderBy,
                                       shared_bool_type& descending,
                                       shared_string_type& delimiter) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](GetBucketCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->volumeContentsResp(e, requestId, cb->vecBlobs, cb->skippedPrefixes);
    };

    auto callback = create_async_handler<GetBucketCallback>(std::move(closure));

    AmRequest *blobReq = new VolumeContentsReq(invalid_vol_id,
                                               *volumeName,
                                               *count,
                                               *offset,
                                               *pattern,
                                               *patternSem,
                                               *orderBy,
                                               *descending,
                                               *delimiter,
                                               callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::setVolumeMetadata(RequestHandle const& requestId,
                                          shared_string_type& domainName,
                                          shared_string_type& volumeName,
                                          shared_meta_type& metadata) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](SetVolumeMetadataCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->setVolumeMetadataResp(e, requestId);
    };

    auto callback = create_async_handler<SetVolumeMetadataCallback>(std::move(closure));

    AmRequest *blobReq = new SetVolumeMetadataReq(invalid_vol_id,
                                                  *volumeName,
                                                  metadata,
                                                  callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::getVolumeMetadata(RequestHandle const& requestId,
                                          shared_string_type& domainName,
                                          shared_string_type& volumeName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](GetVolumeMetadataCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->getVolumeMetadataResp(e, requestId, cb->metadata);
    };

    auto callback = create_async_handler<GetVolumeMetadataCallback>(std::move(closure));

    AmRequest *blobReq = new GetVolumeMetadataReq(invalid_vol_id,
                                                  *volumeName,
                                                  callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::statBlob(RequestHandle const& requestId,
                                 shared_string_type& domainName,
                                 shared_string_type& volumeName,
                                 shared_string_type& blobName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](StatBlobCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->statBlobResp(e, requestId, cb->blobDesc);
    };

    auto callback = create_async_handler<StatBlobCallback>(std::move(closure));

    AmRequest *blobReq = new StatBlobReq(invalid_vol_id,
                                         *volumeName,
                                         *blobName,
                                         callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::startBlobTx(RequestHandle const& requestId,
                                    shared_string_type& domainName,
                                    shared_string_type& volumeName,
                                    shared_string_type& blobName,
                                    shared_int_type& blobMode) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](StartBlobTxCallback* cb, fpi::ErrorCode const& e) mutable -> void {
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
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::commitBlobTx(RequestHandle const& requestId,
                                     shared_string_type& domainName,
                                     shared_string_type& volumeName,
                                     shared_string_type& blobName,
                                     shared_tx_ctx_type& txDesc) {
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](CommitBlobTxCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->commitBlobTxResp(e, requestId);
    };

    auto callback = create_async_handler<CommitBlobTxCallback>(std::move(closure));

    AmRequest *blobReq = new CommitBlobTxReq(invalid_vol_id,
                                             *volumeName,
                                             *blobName,
                                             blobTxDesc,
                                             callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::abortBlobTx(RequestHandle const& requestId,
                                    shared_string_type& domainName,
                                    shared_string_type& volumeName,
                                    shared_string_type& blobName,
                                    shared_tx_ctx_type& txDesc) {
    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](AbortBlobTxCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->abortBlobTxResp(e, requestId);
    };

    auto callback = create_async_handler<AbortBlobTxCallback>(std::move(closure));

    AmRequest *blobReq = new AbortBlobTxReq(invalid_vol_id,
                                            *volumeName,
                                            *blobName,
                                            blobTxDesc,
                                            callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::getBlob(RequestHandle const& requestId,
                                shared_string_type& domainName,
                                shared_string_type& volumeName,
                                shared_string_type& blobName,
                                shared_int_type& length,
                                shared_offset_type& objectOffset) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto closure = [p = responseApi, requestId](GetObjectCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->getBlobResp(e, requestId, cb->return_buffers, cb->return_size);
    };

    auto callback = create_async_handler<GetObjectCallback>(std::move(closure));

    AmRequest *blobReq= new GetBlobReq(invalid_vol_id,
                                       *volumeName,
                                       *blobName,
                                       callback,
                                       static_cast<fds_uint64_t>(objectOffset->value),
                                       *length);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::getBlobWithMeta(RequestHandle const& requestId,
                                        shared_string_type& domainName,
                                        shared_string_type& volumeName,
                                        shared_string_type& blobName,
                                        shared_int_type& length,
                                        shared_offset_type& objectOffset) {
    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto closure = [p = responseApi, requestId](GetObjectWithMetadataCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->getBlobWithMetaResp(e, requestId, cb->return_buffers, cb->return_size, cb->blobDesc);
    };

    auto callback = create_async_handler<GetObjectWithMetadataCallback>(std::move(closure));

    GetBlobReq *blobReq= new GetBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        callback,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length);
    blobReq->get_metadata = true;
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::renameBlob(RequestHandle const& requestId,
                                   shared_string_type& domainName,
                                   shared_string_type& volumeName,
                                   shared_string_type& sourceBlobName,
                                   shared_string_type& destinationBlobName) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](RenameBlobCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->renameBlobResp(e, requestId, cb->blobDesc);
    };

    auto callback = create_async_handler<RenameBlobCallback>(std::move(closure));

    auto blobReq = new RenameBlobReq(invalid_vol_id,
                                    *volumeName,
                                    *sourceBlobName,
                                    *destinationBlobName,
                                    callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::updateMetadata(RequestHandle const& requestId,
                                       shared_string_type& domainName,
                                       shared_string_type& volumeName,
                                       shared_string_type& blobName,
                                       shared_tx_ctx_type& txDesc,
                                       shared_meta_type& metadata) {
    // Closure for response call
    auto closure = [p = responseApi, requestId](UpdateMetadataCallback* cb, fpi::ErrorCode const& e) mutable -> void {
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
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::updateBlobOnce(RequestHandle const& requestId,
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
    auto closure = [p = responseApi, requestId](UpdateBlobCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->updateBlobResp(e, requestId);
    };

    // Quick check, if these don't match reject!
    if ((size_t)*length != bytes->size()) {
        LOGWARN << "Rejecting updateBlobOnce,"
                << " request specified length: " << *length
                << " actual length of payload was: " << bytes->size();
        return closure(nullptr, fpi::BAD_REQUEST);
    }

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
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::updateBlob(RequestHandle const& requestId,
                                   shared_string_type& domainName,
                                   shared_string_type& volumeName,
                                   shared_string_type& blobName,
                                   shared_tx_ctx_type& txDesc,
                                   shared_buffer_type& bytes,
                                   shared_int_type& length,
                                   shared_offset_type& objectOffset) {

    fds_verify(*length >= 0);
    fds_verify(objectOffset->value >= 0);

    // Closure for response call
    auto closure = [p = responseApi, requestId](UpdateBlobCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->updateBlobResp(e, requestId);
    };

    // Quick check, if these don't match reject!
    if ((size_t)*length != bytes->size()) {
        LOGWARN << "Rejecting updateBlob,"
                << " request specified length: " << *length
                << " actual length of payload was: " << bytes->size();
        return closure(nullptr, fpi::BAD_REQUEST);
    }

    auto callback = create_async_handler<UpdateBlobCallback>(std::move(closure));


    // Setup the transcation descriptor
    BlobTxId::ptr blobTxDesc(new BlobTxId(txDesc->txId));

    AmRequest *blobReq = new PutBlobReq(invalid_vol_id,
                                        *volumeName,
                                        *blobName,
                                        static_cast<fds_uint64_t>(objectOffset->value),
                                        *length,
                                        bytes,
                                        blobTxDesc,
                                        callback);
    amProcessor->enqueueRequest(blobReq);
}

void AmAsyncDataApi::deleteBlob(RequestHandle const& requestId,
                                   shared_string_type& domainName,
                                   shared_string_type& volumeName,
                                   shared_string_type& blobName,
                                   shared_tx_ctx_type& txDesc) {
    BlobTxId::ptr blobTxId(new BlobTxId(txDesc->txId));

    // Closure for response call
    auto closure = [p = responseApi, requestId](DeleteBlobCallback* cb, fpi::ErrorCode const& e) mutable -> void {
        p->deleteBlobResp(e, requestId);
    };

    auto callback = create_async_handler<DeleteBlobCallback>(std::move(closure));

    AmRequest *blobReq = new DeleteBlobReq(invalid_vol_id,
                                           *blobName,
                                           *volumeName,
                                           blobTxId,
                                           callback);
    amProcessor->enqueueRequest(blobReq);
}

}  // namespace fds
