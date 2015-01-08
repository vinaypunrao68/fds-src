/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/stat.h>
#include <cstring>
#include <string>
#include <memory>
#include <iostream>
#include <fds_assert.h>
#include <util/Log.h>
#include <libs3.h>
#include <S3Client.h>

namespace fds {

S3TaskStatus::S3TaskStatus()
    : status(S3StatusOK)
{
    memset(&cbData, 0, sizeof(cbData));
}

S3Client::S3Client(const std::string &host,
                   const std::string &accessKey,
                   const std::string &secretKey)
{
    host_ = host;
    accessKey_ = accessKey;
    secretKey_ = secretKey;

    responseHandler_.propertiesCallback =
        &S3Client::responsePropertiesCallback_;
    responseHandler_.completeCallback =
        &S3Client::responseCompleteCallback_;

    auto status = S3_initialize("s3", S3_INIT_ALL, host.c_str());
    fds_verify(status == S3StatusOK);
}

S3Client::~S3Client()
{
    S3_deinitialize();
}

S3Status S3Client::createBucket(const std::string &bucketName)
{
    std::unique_ptr<S3TaskStatus> taskStatus(new S3TaskStatus);
    S3_create_bucket(S3ProtocolHTTP, accessKey_.c_str(), secretKey_.c_str(),
                     host_.c_str(), bucketName.c_str(), S3CannedAclPrivate,
                     NULL, NULL, &responseHandler_, taskStatus.get());
    taskStatus->await();
    return taskStatus->status;
}

S3Status S3Client::putFile(const std::string &bucketName,
                           const std::string &objName,
                           const std::string &filePath)
{
    S3BucketContext bucketContext;
    initBucketContext_(bucketName, bucketContext);

    std::unique_ptr<S3TaskStatus> taskStatus(new S3TaskStatus);

    S3PutObjectCbData *data = &(taskStatus->cbData.putCbData);
    struct stat statbuf;
    if (stat(filePath.c_str(), &statbuf) == -1) {
        GLOGERROR << "Failed to stat file " << filePath;
        return S3StatusErrorUnknown;
    }
    if (!(data->infile = fopen(filePath.c_str(), "r"))) {
        GLOGERROR << "Failed to open file " << filePath;
        return S3StatusErrorUnknown;
    }
    data->contentLength = statbuf.st_size;

    S3PutObjectHandler putObjectHandler =
    {
        responseHandler_,
        &S3Client::putObjectDataCallback_
    };
    S3_put_object(&bucketContext, objName.c_str(), data->contentLength,
                  NULL, NULL, &putObjectHandler, taskStatus.get());

    taskStatus->await();
    return taskStatus->status;
}

S3Status S3Client::getFile(const std::string &bucketName,
                           const std::string &objName,
                           const std::string &filePath)
{
    S3BucketContext bucketContext;
    initBucketContext_(bucketName, bucketContext);

    std::unique_ptr<S3TaskStatus> taskStatus(new S3TaskStatus);

    S3GetObjectCbData *data = &(taskStatus->cbData.getCbData);
    if (!(data->outfile = fopen(filePath.c_str(), "w"))) {
        GLOGDEBUG << "Failed to open file " << filePath;
        return S3StatusErrorUnknown;
    }

    S3GetObjectHandler getObjectHandler =
    {
        responseHandler_,
        &S3Client::getObjectDataCallback_
    };
    S3_get_object(&bucketContext, objName.c_str(), NULL,
                  0, 0, NULL, &getObjectHandler, taskStatus.get());

    taskStatus->await();
    fclose(data->outfile);
    return taskStatus->status;
}

void S3Client::initBucketContext_(const std::string &bucketName,
                                S3BucketContext& bucketContext)
{
    bucketContext = {
        host_.c_str(),
        bucketName.c_str(),
        S3ProtocolHTTP,
        S3UriStylePath,
        accessKey_.c_str(),
        secretKey_.c_str()
    };
}

S3Status S3Client::responsePropertiesCallback_(const S3ResponseProperties *properties,
                                               void *callbackData)
{
    return S3StatusOK;
}

void S3Client::responseCompleteCallback_(S3Status status,
                                         const S3ErrorDetails *error,
                                         void *callbackData)
{
    GLOGDEBUG << " Status: " << status;
    S3TaskStatus* taskStatus = reinterpret_cast<S3TaskStatus*>(callbackData);
    taskStatus->status = status;
    taskStatus->done();
    return;
}

int S3Client::putObjectDataCallback_(int bufferSize, char *buffer, void *callbackData)
{
    S3TaskStatus* taskStatus = reinterpret_cast<S3TaskStatus*>(callbackData);
    S3PutObjectCbData *data = &(taskStatus->cbData.putCbData);
    int ret = 0;
    if (data->contentLength) {
        int toRead = ((data->contentLength > (unsigned) bufferSize) ?
                      (unsigned) bufferSize : data->contentLength);
        ret = fread(buffer, 1, toRead, data->infile);
    }
    data->contentLength -= ret;
    if (data->contentLength == 0) {
        fclose(data->infile);
    }
    GLOGDEBUG << " read: " << ret << " left: " << data->contentLength;
    return ret;
}

S3Status S3Client::getObjectDataCallback_(int bufferSize, const char *buffer,
                                          void *callbackData)
{
    S3TaskStatus* taskStatus = reinterpret_cast<S3TaskStatus*>(callbackData);
    S3GetObjectCbData *data = &(taskStatus->cbData.getCbData);
    size_t wrote = fwrite(buffer, 1, bufferSize, data->outfile);
    return ((wrote < (size_t) bufferSize) ? S3StatusAbortedByCallback : S3StatusOK);
}

}  // namespace fds
