/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_HANDLERMAPPINGS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_HANDLERMAPPINGS_H_

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

void
fn_StartBlobTxHandler(FDSN_Status status,
                      const ErrorDetails *errorDetails,
                      BlobTxId blobTxId,
                      void *callbackData);

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
#endif  // SOURCE_ACCESS_MGR_INCLUDE_HANDLERMAPPINGS_H_
