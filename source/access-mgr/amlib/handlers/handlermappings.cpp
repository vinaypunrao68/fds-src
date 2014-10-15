/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <handlermappings.h>
#include <responsehandler.h>
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

void fn_StartBlobTxHandler(FDSN_Status status,
                           const ErrorDetails *errorDetails,
                           BlobTxId blobTxId,
                           void *callbackData) {
    StartBlobTxResponseHandler* handler =
            reinterpret_cast<StartBlobTxResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;
    handler->blobTxId = blobTxId;

    handler->ready();
}

void fn_CommitBlobTxHandler(FDSN_Status status,
                           const ErrorDetails *errorDetails,
                           BlobTxId blobTxId,
                           void *callbackData) {
    CommitBlobTxResponseHandler* handler =
            reinterpret_cast<CommitBlobTxResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;
    handler->blobTxId = blobTxId;

    handler->ready();
}

void fn_AbortBlobTxHandler(FDSN_Status status,
                           const ErrorDetails *errorDetails,
                           BlobTxId blobTxId,
                           void *callbackData) {
    AbortBlobTxResponseHandler* handler =
            reinterpret_cast<AbortBlobTxResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errorDetails;
    handler->blobTxId = blobTxId;

    handler->ready();
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
    // PutObjectResponseHandler* handler= reinterpret_cast<PutObjectResponseHandler*>(callbackData); //NOLINT
    // handler->status = status;
    // handler->errorDetails = errorDetails;

    // handler->ready();
    return 0;
}

}  // namespace fds
