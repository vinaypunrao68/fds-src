
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <memory>
#include <cstring>
#include <curl/curl.h>
#include <jansson.h>
#include <fds_assert.h>
#include <S3Client.h>
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

}  // namespace fds

S3Status responsePropertiesCallback(
                const S3ResponseProperties *properties,
                void *callbackData);
static void responseCompleteCallback(
                S3Status status,
                const S3ErrorDetails *error,
                void *callbackData);

const char access_key[] = "ACCESS_KEY";
const char secret_key[] = "SECRET_KEY";
const char host[] = "localhost:8000";
const char sample_bucket[] = "sample_bucket";
const char sample_key[] = "hello.txt";
const char sample_file[] = "/home/nbayyana/playground/cpp/curl/curlget.cpp";
S3ResponseHandler responseHandler =
{
        &responsePropertiesCallback,
        &responseCompleteCallback
};
typedef struct put_object_callback_data
{
    FILE *infile;
    uint64_t contentLength;
} put_object_callback_data;

S3Status responsePropertiesCallback(
                const S3ResponseProperties *properties,
                void *callbackData)
{
    std::cout << __FUNCTION__ << std::endl;
    return S3StatusOK;
}

static void responseCompleteCallback(
                S3Status status,
                const S3ErrorDetails *error,
                void *callbackData)
{
    std::cout << __FUNCTION__ << " Status: " << status << std::endl;
    return;
}

static int putObjectDataCallback(int bufferSize,
                                 char *buffer, void *callbackData)
{
    put_object_callback_data *data =
        reinterpret_cast<put_object_callback_data *>(callbackData);
    int ret = 0;
    if (data->contentLength) {
        int toRead = ((data->contentLength > (unsigned) bufferSize) ?
                      (unsigned) bufferSize : data->contentLength);
        ret = fread(buffer, 1, toRead, data->infile);
    }
    data->contentLength -= ret;

    std::cout << __FUNCTION__ << " ret: " << ret << std::endl;
    return ret;
}

TEST(s3, test) {
    S3_initialize("s3", S3_INIT_ALL, host);
    S3_create_bucket(S3ProtocolHTTP, access_key,
                     secret_key, host, sample_bucket,
                     S3CannedAclPrivate, NULL, NULL, &responseHandler, NULL);
    S3_deinitialize();
}

TEST(s3, putTest)
{
    S3BucketContext bucketContext =
    {
        host,
        sample_bucket,
        S3ProtocolHTTP,
        S3UriStylePath,
        access_key,
        secret_key
    };
    S3_initialize("s3", S3_INIT_ALL, host);
    S3PutObjectHandler putObjectHandler =
    {
        responseHandler,
        &putObjectDataCallback
    };
    put_object_callback_data data;
    struct stat statbuf;
    if (stat(sample_file, &statbuf) == -1) {
        fprintf(stderr, "\nERROR: Failed to stat file %s: ", sample_file);
        perror(0);
        exit(-1);
    }

    int contentLength = statbuf.st_size;
    data.contentLength = contentLength;

    if (!(data.infile = fopen(sample_file, "r"))) {
        fprintf(stderr, "\nERROR: Failed to open input file %s: ", sample_file);
        perror(0);
        exit(-1);
    }


    S3_put_object(&bucketContext, sample_key, contentLength,
                  NULL, NULL, &putObjectHandler, &data);
    S3_deinitialize();
}

TEST(s3, getFormationS3Key) {
    // std::string key = fds::getFormationS3Key("https://localhost:7443");
    std::string key = "my-accesss-key";
    std::cout << "key is : " << key << std::endl;
    fds::S3Client cl("localhost:8000", "admin", key);
    auto status = cl.createBucket("sample_bucket");
    ASSERT_TRUE(status == 0 || status == 52);
    status = cl.putFile("sample_bucket", "f1",
                        "/home/nbayyana/playground/cpp/curl/curlget.cpp");
    ASSERT_EQ(status, 0);
    status = cl.getFile("sample_bucket", "f1",
                        "/home/nbayyana/playground/cpp/curl/curlget.cpp_ret");
    ASSERT_EQ(status, 0);
}


int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
