/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_

#include <concurrency/taskstatus.h>
#include <native/types.h>
#include <string>
#include <apis/apis_types.h>
#include <vector>
#include <AmAsyncResponseApi.h>

namespace fds {
/**
 * HandlerType:
 *    - defines how the cb is executed when call() is called!!
 *    WAITEDFOR : Someone else will wait for this call to complete & then process
 *    IMMEDIATE : will be processed at the time of call
 *    QUEUED    : this data will be queued for later processing
 */
enum class HandlerType { WAITEDFOR, IMMEDIATE, QUEUED };

struct ResponseHandler : public Callback {
    HandlerType type = HandlerType::WAITEDFOR;
    void call();
    void ready();
    void wait();
    bool waitFor(ulong timeout);
    virtual void process() = 0;

    virtual ~ResponseHandler();
  protected:
    concurrency::TaskStatus task;
};

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

struct SimpleResponseHandler : ResponseHandler {
    typedef boost::shared_ptr<SimpleResponseHandler> ptr;
    std::string name;
    SimpleResponseHandler();
    explicit SimpleResponseHandler(const std::string& name);

    virtual void process();
    virtual ~SimpleResponseHandler();
};

struct StatBlobResponseHandler : ResponseHandler , StatBlobCallback {
    explicit StatBlobResponseHandler(apis::BlobDescriptor &retVal);
    typedef boost::shared_ptr<StatBlobResponseHandler> ptr;

    apis::BlobDescriptor &retBlobDesc;

    virtual void process();
    virtual ~StatBlobResponseHandler();
};

struct AttachVolumeResponseHandler : ResponseHandler {
    AttachVolumeResponseHandler();
    typedef boost::shared_ptr<AttachVolumeResponseHandler> ptr;

    virtual void process();
    virtual ~AttachVolumeResponseHandler();
};

struct StartBlobTxResponseHandler : ResponseHandler, StartBlobTxCallback {
    explicit StartBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<StartBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~StartBlobTxResponseHandler();
};

struct CommitBlobTxResponseHandler : ResponseHandler, CommitBlobTxCallback {
    explicit CommitBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<CommitBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~CommitBlobTxResponseHandler();
};

struct AbortBlobTxResponseHandler : ResponseHandler, AbortBlobTxCallback {
    explicit AbortBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<AbortBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~AbortBlobTxResponseHandler();
};

struct UpdateBlobResponseHandler : ResponseHandler, UpdateBlobCallback {
    typedef boost::shared_ptr<UpdateBlobResponseHandler> ptr;

    virtual void process();
    virtual ~UpdateBlobResponseHandler();
};

struct GetObjectResponseHandler : ResponseHandler, GetObjectCallback {
    explicit GetObjectResponseHandler();

    typedef boost::shared_ptr<GetObjectResponseHandler> ptr;

    virtual void process();
    virtual ~GetObjectResponseHandler();
};

struct ListBucketResponseHandler : ResponseHandler, GetBucketCallback {
    explicit ListBucketResponseHandler(std::vector<apis::BlobDescriptor> & vecBlobs);
    TYPE_SHAREDPTR(ListBucketResponseHandler);
    std::vector<apis::BlobDescriptor> & retVecBlobs;
    virtual void process();
    virtual ~ListBucketResponseHandler();
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

struct BucketStatsResponseHandler : ResponseHandler {
    explicit BucketStatsResponseHandler(apis::VolumeDescriptor& volumeDescriptor);

    apis::VolumeDescriptor& volumeDescriptor;
    const std::string* timestamp = NULL;
    int content_count = 0;
    const BucketStatsContent* contents = NULL;
    void *req_context = NULL;

    virtual void process();
    virtual ~BucketStatsResponseHandler();
};

struct StatVolumeResponseHandler : ResponseHandler, GetVolumeMetaDataCallback {
    TYPE_SHAREDPTR(StatVolumeResponseHandler);
    apis::VolumeStatus& volumeStatus;
    explicit StatVolumeResponseHandler(apis::VolumeStatus& volumeStatus);
    virtual void process();
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_
