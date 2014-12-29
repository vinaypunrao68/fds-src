
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <memory>
#include <cstring>
#include <curl/curl.h>
#include <jansson.h>
#include <fds_assert.h>
#include <libs3.h>
#include <concurrency/taskstatus.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = reinterpret_cast<char*>(realloc(mem->memory, mem->size + realsize + 1));
    if (mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static std::string curlHttpsGet(const std::string &url)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    std::string retData;
    /* will be grown as needed by the realloc above */
    chunk.memory = reinterpret_cast<char*>(malloc(1));
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&chunk));

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            // TODO(Rao:) Change LOGWARN
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    if (chunk.memory) {
        retData = chunk.memory;
        free(chunk.memory);
    }

    curl_global_cleanup();
    return retData;
}

std::string getFormationS3Key(const std::string &endpoint)
{
    std::string s3key;
    std::string url = endpoint + "/api/auth/token?login=admin&password=admin";
    std::string retJson = curlHttpsGet(url);
    json_error_t error;

    json_t *root = json_loads(retJson.c_str(), 0, &error);
    if (root == nullptr) {
        return s3key;
    }

    json_t *tokJson = json_object_get(root, "token");
    if (tokJson == nullptr) {
        return s3key;
    }

    s3key = json_string_value(tokJson);
    json_decref(root);
    return s3key;
}

struct S3TaskStatus : concurrency::TaskStatus
{
    S3TaskStatus()
        : status(S3StatusOK)
    {
    }

    S3Status status;
};

struct S3Client
{
    S3Client(const std::string &host,
             const std::string &accessKey,
             const std::string &secretKey);
    ~S3Client();
    S3Status createBucket(const std::string &bucketName);
    S3Status putFile();
    static S3Status responsePropertiesCallback_(const S3ResponseProperties *properties,
                                            void *callbackData);
    static void responseCompleteCallback_(S3Status status,
                                          const S3ErrorDetails *error,
                                          void *callbackData);

 protected:
    std::string host_;
    std::string accessKey_;
    std::string secretKey_;
    S3ResponseHandler responseHandler_;
};

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

S3Status S3Client::putFile() {
    return S3StatusOK;
}

S3Status S3Client::responsePropertiesCallback_(const S3ResponseProperties *properties,
                                               void *callbackData)
{
    std::cout << __FUNCTION__ << " " << __LINE__ << std::endl;
    return S3StatusOK;
}

void S3Client::responseCompleteCallback_(S3Status status,
                                         const S3ErrorDetails *error,
                                         void *callbackData)
{
    std::cout << __FUNCTION__ << " Status: " << status << std::endl;
    S3TaskStatus* taskStatus = reinterpret_cast<S3TaskStatus*>(callbackData);
    taskStatus->status = status;
    taskStatus->done();
    return;
}

}  // namespace fds

TEST(s3, getFormationS3Key) {
    std::string key = fds::getFormationS3Key("https://localhost:7443");
    std::cout << "key is : " << key << std::endl;
    fds::S3Client cl("localhost:8050", "admin", key);
    auto status = cl.createBucket("b1");
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
