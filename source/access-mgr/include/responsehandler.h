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
#include <fdsp/dm_types_types.h>
#include <fdsp/xdi_types.h>
#include <fdsp/common_types.h>
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

struct VolumeDesc;

/**
 * After filling the callback object , the call() method should be called.
 * Whether the call() method executes immediately / later / in a separate thread
 * is implementation dependent
 */

struct Callback {
    fpi::ErrorCode error = fpi::OK;

    void call(fpi::ErrorCode err);

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

/* These are identical */
using RenameBlobCallback = StatBlobCallback;

struct StartBlobTxCallback {
    typedef boost::shared_ptr<StartBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct AbortBlobTxCallback {
    typedef boost::shared_ptr<AbortBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct UpdateBlobCallback {
    typedef boost::shared_ptr<UpdateBlobCallback> ptr;
};

struct GetObjectCallback {
    typedef boost::shared_ptr<GetObjectCallback> ptr;
    boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>> return_buffers;
    int return_size {0};
};

struct GetBucketCallback {
    TYPE_SHAREDPTR(GetBucketCallback);
    int isTruncated = 0;
    const char *nextMarker = NULL;
    int contentsCount = 0;
    int commonPrefixesCount = 0;
    const char **commonPrefixes = NULL;

    boost::shared_ptr<std::vector<fpi::BlobDescriptor>> vecBlobs;
    boost::shared_ptr<std::vector<std::string>> skippedPrefixes;
};

struct StatVolumeCallback {
    TYPE_SHAREDPTR(StatVolumeCallback);
    fpi::VolumeStatus volStat;
};

struct SetVolumeMetadataCallback {};
struct GetVolumeMetadataCallback {
    /// The metadata to be filled in
    boost::shared_ptr<std::map<std::string, std::string>> metadata;
};

struct AttachCallback {
    boost::shared_ptr<VolumeDesc> volDesc;
    boost::shared_ptr<fpi::VolumeAccessMode> mode;
};

struct DetachCallback {};
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
