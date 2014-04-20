/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
#define SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_

#include <concurrency/taskstatus.h>
#include <native_api.h>
#include <string>
namespace fds {

    struct ResponseHandler {
        virtual void process() = 0;
        virtual void ready();
        virtual void wait();
        virtual bool waitFor(ulong timeout);

      protected:
        concurrency::TaskStatus task;
    };

    struct SimpleResponseHandler : ResponseHandler {
        FDSN_Status status = FDSN_StatusErrorUnknown;
        const ErrorDetails *errorDetails = NULL;

        virtual void process();
    };

    struct PutObjectResponseHandler : SimpleResponseHandler {
        void *reqContext = NULL;
        fds_uint64_t bufferSize = 0;
        fds_off_t offset = 0;
        char *buffer;

        virtual void process();
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
    };

    struct ListBucketResponseHandler : SimpleResponseHandler {
        int isTruncated = 0;
        const char *nextMarker = NULL;
        int contentsCount = 0;
        const ListBucketContents *contents =NULL;
        int commonPrefixesCount = 0;
        const char **commonPrefixes = NULL;

        virtual void process();
    };

    struct BucketStatsResponseHandler : SimpleResponseHandler {
        const std::string* timestamp = NULL;
        int content_count = 0;
        const BucketStatsContent* contents = NULL;
        void *req_context = NULL;

        virtual void process();
    };
}  // namespace fds
#endif  // SOURCE_INCLUDE_AM_ENGINE_HANDLERS_RESPONSEHANDLER_H_
