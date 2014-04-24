/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
#define SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_

#include <concurrency/taskstatus.h>
#include <native_api.h>
#include <string>
#include <xdi/am_shim_types.h>

namespace fds {

struct ResponseHandler {
    virtual void process() = 0;
    virtual void ready();
    virtual void wait();
    virtual bool waitFor(ulong timeout);

    virtual ~ResponseHandler();
  protected:
    concurrency::TaskStatus task;
};

struct SimpleResponseHandler : ResponseHandler {
    std::string name;
    SimpleResponseHandler();
    SimpleResponseHandler(const std::string& name);

    FDSN_Status status = FDSN_StatusErrorUnknown;
    const ErrorDetails *errorDetails = NULL;

    virtual void process();
    virtual ~SimpleResponseHandler();
};

struct PutObjectResponseHandler : SimpleResponseHandler {
    void *reqContext = NULL;
    fds_uint64_t bufferSize = 0;
    fds_off_t offset = 0;
    char *buffer;

    virtual void process();
    virtual ~PutObjectResponseHandler();
};

struct GetObjectResponseHandler : SimpleResponseHandler {
    BucketContextPtr bucket_ctx;
    void *reqContext = NULL;
    fds_uint64_t bufferSize = 0;
    fds_off_t offset = 0;
    const char *buffer = NULL;
    fds_uint64_t blobSize = 0;
    const std::string* blobEtag = NULL;

    virtual void process();
    virtual ~GetObjectResponseHandler();
};

struct ListBucketResponseHandler : SimpleResponseHandler {
    int isTruncated = 0;
    const char *nextMarker = NULL;
    int contentsCount = 0;
    const ListBucketContents *contents =NULL;
    int commonPrefixesCount = 0;
    const char **commonPrefixes = NULL;

    virtual void process();
    virtual ~ListBucketResponseHandler();
};

struct BucketStatsResponseHandler : SimpleResponseHandler {
    BucketStatsResponseHandler(xdi::VolumeDescriptor& volumeDescriptor);

    xdi::VolumeDescriptor& volumeDescriptor;
    const std::string* timestamp = NULL;
    int content_count = 0;
    const BucketStatsContent* contents = NULL;
    void *req_context = NULL;

    virtual void process();
    virtual ~BucketStatsResponseHandler();
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
