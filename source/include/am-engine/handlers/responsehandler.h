/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
#define SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_

#include <concurrency/taskstatus.h>
#include <string>
#include <xdi/am_shim_types.h>

namespace fds {
    /**
     * HandlerType:
     *    - defines how the cb is executed when call() is called!!
     *    WAITEDFOR : Someone else will wait for this call to complete & then process
     *    IMMEDIATE : will be processed at the time of call
     *    QUEUED    : this data will be queued for later processing
     */
    enum class HandlerType { WAITEDFOR, IMMEDIATE, QUEUED };

    struct ResponseHandler : virtual native::Callback {
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
        std::string name;
        SimpleResponseHandler();
        SimpleResponseHandler(const std::string& name);

        virtual void process();
        virtual ~SimpleResponseHandler();
    };

    struct PutObjectResponseHandler : ResponseHandler {
        void *reqContext = NULL;
        fds_uint64_t bufferSize = 0;
        fds_off_t offset = 0;
        char *buffer;

        virtual void process();
        virtual ~PutObjectResponseHandler();
    };

    struct GetObjectResponseHandler : ResponseHandler {
        BucketContextPtr bucket_ctx;
        void *reqContext = NULL;
        fds_uint64_t bufferSize = 0;
        fds_off_t offset = 0;
        const char *buffer = NULL;
        fds_uint64_t blobSize = 0;
        const std::string* blobEtag = NULL;
        fds_uint64_t reqId = 0;

        virtual void process();
        virtual ~GetObjectResponseHandler();
    };

    struct ListBucketResponseHandler : ResponseHandler {
        int isTruncated = 0;
        const char *nextMarker = NULL;
        int contentsCount = 0;
        const ListBucketContents *contents =NULL;
        int commonPrefixesCount = 0;
        const char **commonPrefixes = NULL;

        virtual void process();
        virtual ~ListBucketResponseHandler();
    };

    struct BucketStatsResponseHandler : ResponseHandler {
        BucketStatsResponseHandler(xdi::VolumeDescriptor& volumeDescriptor);

        xdi::VolumeDescriptor& volumeDescriptor;
        const std::string* timestamp = NULL;
        int content_count = 0;
        const BucketStatsContent* contents = NULL;
        void *req_context = NULL;

        virtual void process();
        virtual ~BucketStatsResponseHandler();
    };

    /** Newer callback schemes **/

    struct StatBlobResponseHandler : virtual ResponseHandler, virtual native::StatBlobCallback {
        
    };

}  // namespace fds
#endif  // SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
