/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_S3CLIENT_H_
#define SOURCE_INCLUDE_S3CLIENT_H_

#include <libs3.h>
#include <string>
#include <concurrency/taskstatus.h>

namespace fds {
struct S3PutObjectCbData
{
    FILE *infile;
    uint64_t contentLength;
};

struct S3GetObjectCbData
{
    FILE *outfile;
};

struct S3TaskStatus : concurrency::TaskStatus
{
    S3TaskStatus();

    union {
        S3PutObjectCbData putCbData;
        S3GetObjectCbData getCbData;
    } cbData;
    S3Status status;
};


struct S3Client
{
    S3Client(const std::string &host,
             const std::string &accessKey,
             const std::string &secretKey);
    ~S3Client();
    S3Status createBucket(const std::string &bucketName);
    S3Status putFile(const std::string &bucketName,
                     const std::string &objName,
                     const std::string &filePath);
    S3Status getFile(const std::string &bucketName,
                     const std::string &objName,
                     const std::string &filePath);

    static S3Status responsePropertiesCallback_(const S3ResponseProperties *properties,
                                            void *callbackData);
    static void responseCompleteCallback_(S3Status status,
                                          const S3ErrorDetails *error,
                                          void *callbackData);
    static int putObjectDataCallback_(int bufferSize, char *buffer, void *callbackData);
    static S3Status getObjectDataCallback_(int bufferSize, const char *buffer,
                                           void *callbackData);

 protected:
    void initBucketContext_(const std::string &bucketName, S3BucketContext& bucketContext);

    std::string host_;
    std::string accessKey_;
    std::string secretKey_;
    S3ResponseHandler responseHandler_;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_S3CLIENT_H_
