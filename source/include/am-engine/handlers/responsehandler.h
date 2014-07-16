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

/**
 * Response handler for native putObject() calls done
 * for block-specific requests.
 * A specific handler is needed because an additional
 * callback to UBD is needed to notify the kernel that
 * a request has been processed.
 */
struct PutObjectBlkResponseHandler : PutObjectResponseHandler {
    typedef boost::function<void(fds_int32_t)> blkCallback;
    blkCallback ubdCallback;

    template<typename F, typename A, typename B, typename C>
    PutObjectBlkResponseHandler(F f,
                                A a,
                                B b,
                                C c)
            : ubdCallback(boost::bind(f, a, b, c, _1)) {
        type = HandlerType::IMMEDIATE;
    }
    virtual void process();
    virtual ~PutObjectBlkResponseHandler();

    typedef boost::shared_ptr<PutObjectBlkResponseHandler> ptr;
};

struct GetObjectResponseHandler : ResponseHandler {
    BucketContextPtr bucket_ctx;
    void *reqContext = NULL;
    fds_uint64_t bufferSize = 0;
    fds_off_t offset = 0;
    const char *buffer = NULL;
    fds_uint64_t blobSize = 0;
    std::string blobEtag;

    virtual void process();
    virtual ~GetObjectResponseHandler();
};

/**
 * Response handler for native getObject() calls done
 * for block-specific requests.
 * A specific handler is needed because an additional
 * callback to UBD is needed to notify the kernel that
 * a request has been processed.
 */
struct GetObjectBlkResponseHandler : GetObjectResponseHandler {
    typedef boost::function<void(fds_int32_t)> blkCallback;
    blkCallback ubdCallback;

    template<typename F, typename A, typename B, typename C>
    GetObjectBlkResponseHandler(F f,
                                A a,
                                B b,
                                C c)
            : ubdCallback(boost::bind(f, a, b, c, _1)) {
        type = HandlerType::IMMEDIATE;
    }
    virtual void process();
    virtual ~GetObjectBlkResponseHandler();

    typedef boost::shared_ptr<GetObjectBlkResponseHandler> ptr;
};

struct ListBucketResponseHandler : ResponseHandler, GetBucketCallback {
    ListBucketResponseHandler(std::vector<apis::BlobDescriptor> & vecBlobs);

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
