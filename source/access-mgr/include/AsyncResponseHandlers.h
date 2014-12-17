/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_ASYNCRESPONSEHANDLERS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_ASYNCRESPONSEHANDLERS_H_

#include "responsehandler.h"

namespace fds
{

template<typename T>
struct AsyncResponseHandler :   public ResponseHandler,
                                public T
{
    AsyncResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                         boost::shared_ptr<apis::RequestId>& _reqId)
        : respApi(_api), requestId(_reqId)
    { type = HandlerType::IMMEDIATE; }

    void process();

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;
};

using AbortBlobTxHandler = AsyncResponseHandler<AbortBlobTxCallback>;
template<> inline void AbortBlobTxHandler::process()
{ respApi->abortBlobTxResp(error, requestId); }

using AttachHandler = AsyncResponseHandler<AttachCallback>;
template<> inline void AttachHandler::process()
{ respApi->attachVolumeResp(error, requestId); }

using CommitBlobTxHandler = AsyncResponseHandler<CommitBlobTxCallback>;
template<> inline void CommitBlobTxHandler::process()
{ respApi->commitBlobTxResp(error, requestId); }

using DeleteBlobHandler = AsyncResponseHandler<DeleteBlobCallback>;
template<> inline void DeleteBlobHandler::process()
{ respApi->deleteBlobResp(error, requestId); }

using GetBlobHandler = AsyncResponseHandler<GetObjectCallback>;
template<> inline void GetBlobHandler::process()
{ respApi->getBlobResp(error, requestId, returnBuffer, returnSize); }

using GetBlobWithMetadataHandler = AsyncResponseHandler<GetObjectWithMetadataCallback>;
template<> void GetBlobWithMetadataHandler::process();

using StartBlobTxHandler = AsyncResponseHandler<StartBlobTxCallback>;
template<> inline void StartBlobTxHandler::process() {
    auto txDesc = boost::make_shared<apis::TxDescriptor>();
    txDesc->txId = blobTxId.getValue();
    respApi->startBlobTxResp(error, requestId, txDesc);
}

using StatBlobHandler = AsyncResponseHandler<StatBlobCallback>;
template<> void StatBlobHandler::process();

using UpdateBlobHandler = AsyncResponseHandler<UpdateBlobCallback>;
template<> inline void UpdateBlobHandler::process()
{ respApi->updateBlobResp(error, requestId); }

using UpdateMetadataHandler = AsyncResponseHandler<UpdateMetadataCallback>;
template<> inline void UpdateMetadataHandler::process()
{ respApi->updateMetadataResp(error, requestId); }

using VolumeContentsHandler = AsyncResponseHandler<GetBucketCallback>;
template<> inline void VolumeContentsHandler::process()
{ respApi->volumeContentsResp(error, requestId, vecBlobs); }

using VolumeStatusHandler = AsyncResponseHandler<GetVolumeMetaDataCallback>;
template<> void VolumeStatusHandler::process();

}  // namespace fds


#endif  // SOURCE_ACCESS_MGR_INCLUDE_ASYNCRESPONSEHANDLERS_H_
