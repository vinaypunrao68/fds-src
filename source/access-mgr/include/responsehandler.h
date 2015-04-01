/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_

#include <concurrency/taskstatus.h>
#include <string>
#include <vector>

#include <fds_defines.h>
#include <fds_error.h>
#include <fds_typedefs.h>
#include <fdsp/xdi_types.h>
#include <fdsp/config_types_types.h>
#include <blob/BlobTypes.h>

namespace fds {
/**
 * HandlerType:
 *    - defines how the cb is executed when call() is called!!
 *    WAITEDFOR : Someone else will wait for this call to complete & then process
 *    IMMEDIATE : will be processed at the time of call
 *    QUEUED    : this data will be queued for later processing
 */
enum class HandlerType { WAITEDFOR, IMMEDIATE, QUEUED };

/**
 * After filling the callback object , the call() method should be called.
 * Whether the call() method executes immediately / later / in a separate thread
 * is implementation dependent
 */

struct Callback {
    FDSN_Status status = FDSN_StatusErrorUnknown;
    Error error = ERR_MAX;

    void operator()(FDSN_Status status = FDSN_StatusErrorUnknown);
    void call(FDSN_Status status);
    void call(Error err);
    bool isStatusSet();
    bool isErrorSet();

    virtual void call() = 0;
    virtual ~Callback() = default;
};
typedef boost::shared_ptr<Callback> CallbackPtr;

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

struct SimpleResponseHandler : ResponseHandler {
    typedef boost::shared_ptr<SimpleResponseHandler> ptr;
    std::string name;
    SimpleResponseHandler();
    explicit SimpleResponseHandler(const std::string& name);

    virtual void process();
    virtual ~SimpleResponseHandler();
};

/**
 * These are the structures that AmDataApi uses to respond with, with
 * the Dispatcher and Processor setting them, and the DataApi using them to
 * callback the connector interfaces.
 */

struct StatBlobCallback {
    typedef boost::shared_ptr<StatBlobCallback> ptr;
    /// The blob descriptor to fill in
    boost::shared_ptr<BlobDescriptor> blobDesc;
};

struct StatBlobResponseHandler : ResponseHandler , StatBlobCallback {
    explicit StatBlobResponseHandler(fpi::BlobDescriptor &retVal);
    typedef boost::shared_ptr<StatBlobResponseHandler> ptr;

    fpi::BlobDescriptor &retBlobDesc;

    virtual void process();
    virtual ~StatBlobResponseHandler();
};

struct AttachVolumeResponseHandler : ResponseHandler {
    AttachVolumeResponseHandler();
    typedef boost::shared_ptr<AttachVolumeResponseHandler> ptr;

    virtual void process();
    virtual ~AttachVolumeResponseHandler();
};

struct StartBlobTxCallback {
    typedef boost::shared_ptr<StartBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct StartBlobTxResponseHandler : ResponseHandler, StartBlobTxCallback {
    explicit StartBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<StartBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~StartBlobTxResponseHandler();
};

struct AbortBlobTxCallback {
    typedef boost::shared_ptr<AbortBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct AbortBlobTxResponseHandler : ResponseHandler, AbortBlobTxCallback {
    explicit AbortBlobTxResponseHandler(apis::TxDescriptor &retVal);
    typedef boost::shared_ptr<AbortBlobTxResponseHandler> ptr;

    apis::TxDescriptor &retTxDesc;

    virtual void process();
    virtual ~AbortBlobTxResponseHandler();
};

struct UpdateBlobCallback {
    typedef boost::shared_ptr<UpdateBlobCallback> ptr;
};

struct UpdateBlobResponseHandler : ResponseHandler, UpdateBlobCallback {
    typedef boost::shared_ptr<UpdateBlobResponseHandler> ptr;

    virtual void process();
    virtual ~UpdateBlobResponseHandler();
};

struct GetObjectCallback {
    typedef boost::shared_ptr<GetObjectCallback> ptr;
    boost::shared_ptr<std::string> returnBuffer;
    fds_uint32_t returnSize;
};

struct GetObjectResponseHandler : ResponseHandler, GetObjectCallback {
    GetObjectResponseHandler();

    typedef boost::shared_ptr<GetObjectResponseHandler> ptr;

    virtual void process();
    virtual ~GetObjectResponseHandler();
};

struct GetBucketCallback {
    TYPE_SHAREDPTR(GetBucketCallback);
    int isTruncated = 0;
    const char *nextMarker = NULL;
    int contentsCount = 0;
    int commonPrefixesCount = 0;
    const char **commonPrefixes = NULL;

    boost::shared_ptr<std::vector<fpi::BlobDescriptor>> vecBlobs;
};

struct ListBucketResponseHandler : ResponseHandler, GetBucketCallback {
    explicit ListBucketResponseHandler(std::vector<fpi::BlobDescriptor> & vecBlobs);
    TYPE_SHAREDPTR(ListBucketResponseHandler);
    std::vector<fpi::BlobDescriptor> & retVecBlobs;
    virtual void process();
    virtual ~ListBucketResponseHandler();
};

struct BucketStatsResponseHandler : ResponseHandler {
    explicit BucketStatsResponseHandler(apis::VolumeDescriptor& volumeDescriptor);

    apis::VolumeDescriptor& volumeDescriptor;
    const std::string* timestamp = NULL;
    int content_count = 0;
    void *req_context = NULL;

    virtual void process();
    virtual ~BucketStatsResponseHandler();
};

struct StatVolumeCallback {
    TYPE_SHAREDPTR(StatVolumeCallback);
    fpi::VolumeStatus volStat;
};

struct StatVolumeResponseHandler : ResponseHandler, StatVolumeCallback {
    TYPE_SHAREDPTR(StatVolumeResponseHandler);
    apis::VolumeStatus& volumeStatus;
    explicit StatVolumeResponseHandler(apis::VolumeStatus& volumeStatus);
    virtual void process();
};

struct SetVolumeMetadataCallback {};
struct GetVolumeMetadataCallback {
    /// The metadata to be filled in
    boost::shared_ptr<std::map<std::string, std::string>> metadata;
};

struct AttachCallback {};
struct DeleteBlobCallback {};
struct UpdateMetadataCallback {};

struct GetObjectWithMetadataCallback :  public GetObjectCallback,
                                        public StatBlobCallback
{
};

struct CommitBlobTxCallback {
    typedef boost::shared_ptr<CommitBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_RESPONSEHANDLER_H_
