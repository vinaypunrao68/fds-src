/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/handlermappings.h>
#include <am-engine/handlers/responsehandler.h>
#include <string>
#include <util/Log.h>

namespace fds {

void fn_ResponseHandler(FDSN_Status status,
                        const ErrorDetails *errorDetails,
                        void *callbackData) {
    SimpleResponseHandler* handler= reinterpret_cast<SimpleResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;
    handler->ready();
}

void fn_StatBlobHandler(FDSN_Status status,
                        const ErrorDetails *errorDetails,
                        BlobDescriptor blobDesc,
                        void *callbackData) {
    StatBlobResponseHandler* handler= reinterpret_cast<StatBlobResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;
    handler->blobDesc = blobDesc;

    handler->ready();
}

FDSN_Status fn_GetObjectHandler(BucketContextPtr bucket_ctx,
                                void *reqContext,
                                fds_uint64_t bufferSize,
                                fds_off_t offset,
                                const char *buffer,
                                fds_uint64_t blobSize,
                                const std::string &blobEtag,
                                void *callbackData,
                                FDSN_Status status,
                                ErrorDetails *errorDetails) {
    GetObjectResponseHandler* handler= reinterpret_cast<GetObjectResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;

    handler->bucket_ctx = bucket_ctx;
    handler->reqContext = reqContext;
    handler->bufferSize = bufferSize;
    handler->offset = offset;
    handler->buffer = buffer;
    handler->blobSize = blobSize;
    handler->blobEtag = blobEtag;

    handler->ready();
    return FDSN_StatusOK;
}

void fn_ListBucketHandler(int isTruncated,
                          const char *nextMarker,
                          int contentsCount,
                          const ListBucketContents *contents,
                          int commonPrefixesCount,
                          const char **commonPrefixes,
                          void *callbackData,
                          FDSN_Status status) {
    ListBucketResponseHandler* handler= reinterpret_cast<ListBucketResponseHandler*>(callbackData); //NOLINT
    handler->status = status;

    handler->isTruncated = isTruncated;
    handler->nextMarker = nextMarker;
    handler->contentsCount = contentsCount;
    handler->contents = contents;
    handler->commonPrefixesCount = commonPrefixesCount;
    handler->commonPrefixes = commonPrefixes;

    handler->ready();
}

void fn_BucketStatsHandler(const std::string& timestamp,
                           int content_count,
                           const BucketStatsContent* contents,
                           void *req_context,
                           void *callbackData,
                           FDSN_Status status,
                           ErrorDetails *errorDetails) {
    BucketStatsResponseHandler* handler= reinterpret_cast<BucketStatsResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;
    handler->timestamp = &timestamp;
    handler->content_count = content_count;
    handler->contents = contents;
    handler->req_context = req_context;

    handler->ready();
}

int fn_PutObjectHandler(void *reqContext,
                        fds_uint64_t bufferSize,
                        fds_off_t offset,
                        char *buffer,
                        void *callbackData,
                        FDSN_Status status,
                        ErrorDetails* errorDetails) {
    PutObjectResponseHandler* handler= reinterpret_cast<PutObjectResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;

    handler->ready();
    return 0;
}

}  // namespace fds
