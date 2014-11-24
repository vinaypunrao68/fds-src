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

struct ResponseHandler : virtual Callback {
    HandlerType type = HandlerType::WAITEDFOR;
    void call();
    void ready();
    void wait();
    bool waitFor(ulong timeout);

    virtual void process() {}

    // TODO(Greg): May be removed when sync interface is removed.
    virtual bool isAsyncHandler() {return true;}

    virtual ~ResponseHandler();
  protected:
    concurrency::TaskStatus task;
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

struct AsyncStatBlobResponseHandler : ResponseHandler , StatBlobCallback {
    AsyncStatBlobResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                 boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncStatBlobResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    boost::shared_ptr<apis::BlobDescriptor> retBlobDesc;

    virtual void process();
    virtual ~AsyncStatBlobResponseHandler();
};

struct AttachVolumeResponseHandler : ResponseHandler {
    AttachVolumeResponseHandler();
    typedef boost::shared_ptr<AttachVolumeResponseHandler> ptr;

    virtual void process();
    virtual ~AttachVolumeResponseHandler();
};

struct AsyncAttachVolumeResponseHandler : ResponseHandler {
    AsyncAttachVolumeResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                     boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncAttachVolumeResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncAttachVolumeResponseHandler();
};

struct StartBlobTxResponseHandler : ResponseHandler, StartBlobTxCallback {
    explicit StartBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<StartBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~StartBlobTxResponseHandler();
};

struct AsyncStartBlobTxResponseHandler
        : ResponseHandler, StartBlobTxCallback {
    AsyncStartBlobTxResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                    boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncStartBlobTxResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    /// Trans descriptor to be filled in on success
    boost::shared_ptr<apis::TxDescriptor> txDesc;

    virtual void process();
    virtual ~AsyncStartBlobTxResponseHandler();
};

struct CommitBlobTxResponseHandler : ResponseHandler, CommitBlobTxCallback {
    explicit CommitBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<CommitBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~CommitBlobTxResponseHandler();
};

struct AsyncCommitBlobTxResponseHandler
        : ResponseHandler, CommitBlobTxCallback {
    AsyncCommitBlobTxResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                    boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncCommitBlobTxResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    /// Trans descriptor to be filled in on success
    boost::shared_ptr<apis::TxDescriptor> txDesc;

    virtual void process();
    virtual ~AsyncCommitBlobTxResponseHandler();
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

struct AsyncUpdateBlobResponseHandler : ResponseHandler, UpdateBlobCallback {
    AsyncUpdateBlobResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                   boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncUpdateBlobResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncUpdateBlobResponseHandler();
};

struct AsyncUpdateBlobOnceResponseHandler : ResponseHandler, UpdateBlobCallback {
    AsyncUpdateBlobOnceResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                   boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncUpdateBlobOnceResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncUpdateBlobOnceResponseHandler();
};

struct AsyncUpdateMetadataResponseHandler : ResponseHandler {
    AsyncUpdateMetadataResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                       boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncUpdateMetadataResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncUpdateMetadataResponseHandler();
};

struct AsyncAbortBlobTxResponseHandler : ResponseHandler {
    AsyncAbortBlobTxResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                    boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncAbortBlobTxResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncAbortBlobTxResponseHandler();
};

struct AsyncDeleteBlobResponseHandler : ResponseHandler {
    AsyncDeleteBlobResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                   boost::shared_ptr<apis::RequestId>& _reqId);
    typedef boost::shared_ptr<AsyncDeleteBlobResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncDeleteBlobResponseHandler();
};

struct GetObjectResponseHandler : ResponseHandler, GetObjectCallback {
    explicit GetObjectResponseHandler(char *buf);

    typedef boost::shared_ptr<GetObjectResponseHandler> ptr;

    virtual void process();
    virtual ~GetObjectResponseHandler();

    virtual bool isAsyncHandler() {return false;}
};

struct AsyncGetObjectResponseHandler : ResponseHandler, GetObjectCallback {
    AsyncGetObjectResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                    boost::shared_ptr<apis::RequestId>& _reqId,
                                    boost::shared_ptr<int32_t>& length,
                                    char* buf = nullptr);
    typedef boost::shared_ptr<AsyncGetObjectResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    virtual void process();
    virtual ~AsyncGetObjectResponseHandler();
};

struct AsyncGetWithMetaResponseHandler : ResponseHandler, GetObjectCallback {
    AsyncGetWithMetaResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                    boost::shared_ptr<apis::RequestId>& _reqId,
                                    boost::shared_ptr<int32_t>& length,
                                    char* buf = nullptr);
    typedef boost::shared_ptr<AsyncGetWithMetaResponseHandler> ptr;

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;

    boost::shared_ptr<apis::BlobDescriptor> retBlobDesc;

    virtual void process();
    virtual ~AsyncGetWithMetaResponseHandler();
};

struct ListBucketResponseHandler : ResponseHandler, GetBucketCallback {
    explicit ListBucketResponseHandler(std::vector<apis::BlobDescriptor> & vecBlobs);
    TYPE_SHAREDPTR(ListBucketResponseHandler);
    std::vector<apis::BlobDescriptor> & retVecBlobs;
    virtual void process();
    virtual ~ListBucketResponseHandler();
};

struct AsyncListBucketResponseHandler : ResponseHandler, GetBucketCallback {
    AsyncListBucketResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                   boost::shared_ptr<apis::RequestId>& _reqId);
    TYPE_SHAREDPTR(AsyncListBucketResponseHandler);

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;
    boost::shared_ptr<std::vector<apis::BlobDescriptor>> vecBlobs;

    virtual void process();
    virtual ~AsyncListBucketResponseHandler();
};

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

struct AsyncStatVolumeResponseHandler : ResponseHandler, GetVolumeMetaDataCallback {
    AsyncStatVolumeResponseHandler(AmAsyncResponseApi::shared_ptr _api,
                                   boost::shared_ptr<apis::RequestId>& _reqId);
    TYPE_SHAREDPTR(AsyncStatVolumeResponseHandler);

    AmAsyncResponseApi::shared_ptr respApi;
    boost::shared_ptr<apis::RequestId> requestId;
    boost::shared_ptr<apis::VolumeStatus> volumeStatus;
    virtual void process();
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_
