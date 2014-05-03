/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_HANDLERS_HANDLERMAPPINGS_H_
#define SOURCE_INCLUDE_AM_ENGINE_HANDLERS_HANDLERMAPPINGS_H_

#include <native_api.h>
#include <string>

namespace fds {
void fn_ResponseHandler(FDSN_Status status,
                        const ErrorDetails *errorDetails,
                        void *callbackData);

void fn_StatBlobHandler(FDSN_Status status,
                        const ErrorDetails *errorDetails,
                        BlobDescriptor blobDesc,
                        void *callbackData);

FDSN_Status fn_GetObjectHandler(BucketContextPtr bucket_ctx,
                                void *reqContext,
                                fds_uint64_t bufferSize,
                                fds_off_t offset,
                                const char *buffer,
                                fds_uint64_t blobSize,
                                const std::string &blobEtag,
                                void *callbackData,
                                FDSN_Status status,
                                ErrorDetails *errDetails);

void fn_ListBucketHandler(int isTruncated,
                          const char *nextMarker,
                          int contentsCount,
                          const ListBucketContents *contents,
                          int commonPrefixesCount,
                          const char **commonPrefixes,
                          void *callbackData,
                          FDSN_Status status);

void fn_BucketStatsHandler(const std::string& timestamp,
                           int content_count,
                           const BucketStatsContent* contents,
                           void *req_context,
                           void *callbackData,
                           FDSN_Status status,
                           ErrorDetails *err_details);

int fn_PutObjectHandler(void *reqContext,
                        fds_uint64_t bufferSize,
                        fds_off_t offset,
                        char *buffer,
                        void *callbackData,
                        FDSN_Status status,
                        ErrorDetails* errDetails);
}  // namespace fds
#endif  // SOURCE_INCLUDE_AM_ENGINE_HANDLERS_HANDLERMAPPINGS_H_
