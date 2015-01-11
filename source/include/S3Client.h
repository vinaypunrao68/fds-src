/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_S3CLIENT_H_
#define SOURCE_INCLUDE_S3CLIENT_H_

#include <string>
#include <fds_error.h>
#include <concurrency/taskstatus.h>

namespace fds {
struct CurlMemoryStruct {
    char *memory;
    size_t size;
};

struct S3Client
{
    S3Client(const std::string &host,
             const std::string &authEp,
             const std::string &admin,
             const std::string &passwd);
    ~S3Client();
    Error createBucket(const std::string &bucketName);
    Error putFile(const std::string &bucketName,
                     const std::string &objName,
                     const std::string &filePath);
    Error getFile(const std::string &bucketName,
                  const std::string &objName,
                  const std::string &filePath);

    static size_t putFileCb(void *ptr, size_t size, size_t nmemb, void *stream);
    static size_t getFileCb(void *ptr, size_t size, size_t nmemb, FILE *stream);
    static size_t curlGetMemoryCb(void *contents, size_t size, size_t nmemb, void *userp);

 protected:
    Error authenticate_();
    Error curlGet_(const std::string &url, std::string& retJson);

    std::string host_;
    bool isHttps_;
    std::string authEp_;
    std::string admin_;
    std::string passwd_;
    std::string accessKey_;
    std::string secretKey_;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_S3CLIENT_H_
