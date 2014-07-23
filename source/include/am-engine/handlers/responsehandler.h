/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
#define SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_

#include <concurrency/taskstatus.h>
#include <native/types.h>
#include <string>
#include <apis/apis_types.h>
#include <vector>
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

    virtual void process() {};

    virtual ~ResponseHandler();
  protected:
    concurrency::TaskStatus task;
};

struct SimpleResponseHandler : ResponseHandler {
    typedef boost::shared_ptr<SimpleResponseHandler> ptr;
    std::string name;
    SimpleResponseHandler();
    SimpleResponseHandler(const std::string& name);

    virtual void process();
    virtual ~SimpleResponseHandler();
};

struct StatBlobResponseHandler : ResponseHandler , StatBlobCallback {
    StatBlobResponseHandler(apis::BlobDescriptor &retVal);
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
    StartBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<StartBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~StartBlobTxResponseHandler();
};

struct CommitBlobTxResponseHandler : ResponseHandler, CommitBlobTxCallback {
    CommitBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<CommitBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~CommitBlobTxResponseHandler();
};

struct AbortBlobTxResponseHandler : ResponseHandler, AbortBlobTxCallback {
    AbortBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<AbortBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~AbortBlobTxResponseHandler();
};

struct PutObjectResponseHandler : ResponseHandler {
    void *reqContext = NULL;
    fds_uint64_t bufferSize = 0;
    fds_off_t offset = 0;
    char *buffer;

    virtual void process();
    virtual ~PutObjectResponseHandler();
};

struct GetObjectResponseHandler : ResponseHandler, GetObjectCallback {
    GetObjectResponseHandler(char *buf);

    typedef boost::shared_ptr<GetObjectResponseHandler> ptr;

    virtual void process();
    virtual ~GetObjectResponseHandler();
};

struct ListBucketResponseHandler : ResponseHandler, GetBucketCallback {
    ListBucketResponseHandler(std::vector<apis::BlobDescriptor> & vecBlobs);
    TYPE_SHAREDPTR(ListBucketResponseHandler);
    std::vector<apis::BlobDescriptor> & vecBlobs;
    virtual void process();
    virtual ~ListBucketResponseHandler();
};

struct BucketStatsResponseHandler : ResponseHandler {
    BucketStatsResponseHandler(apis::VolumeDescriptor& volumeDescriptor);

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
#endif  // SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
