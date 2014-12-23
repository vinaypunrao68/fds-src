
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <curl/curl.h>
#include <jansson.h>
#include <fds_assert.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

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
 
  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
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

    chunk.memory = (char*) malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;    /* no data at this point */ 

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        }

        /* always cleanup */ 
        curl_easy_cleanup(curl);
    }

    if(chunk.memory) {
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

    json_t *root = json_loads(url.c_str(), 0, &error);
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

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
