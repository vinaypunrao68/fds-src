/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/stat.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <memory>
#include <iostream>
#include <fds_assert.h>
#include <util/Log.h>
#include <curl/curl.h>
#include <jansson.h>
#include <S3Client.h>

namespace fds {

S3Client::S3Client(const std::string &host,
                   const std::string &authEp,
                   const std::string &admin,
                   const std::string &passwd)
{
    host_ = host;
    authEp_ = authEp;
    admin_ = admin;
    passwd_ = passwd;

    curl_global_init(CURL_GLOBAL_ALL);

    if (!authEp_.empty()) {
        fds_assert(host.find("https://") == 0);
        fds_assert(authEp.find("https://") == 0);
        GLOGDEBUG << "Doing authentication";

        isHttps_ = true;
        Error e = authenticate_();
        if (e != ERR_OK) {
            LOGWARN << "Authentication failed";
        }
    } else {
        fds_assert(host.find("https://") == std::string::npos);
        isHttps_ = false;
    }
}

S3Client::~S3Client()
{
    curl_global_cleanup();
}

Error S3Client::authenticate_()
{
    std::string s3key;
    std::stringstream ss;
    std::string url;
    std::string retJson;
    json_error_t jsonErr;
    Error err = ERR_OK;

    fds_assert(isHttps_ == true);

    ss << authEp_ << "/api/auth/token?login=" << admin_ << "&password=" << passwd_;
    url = ss.str();

    err = curlGet_(url, retJson);
    if (err != ERR_OK) {
        return err;
    }

    json_t *root = json_loads(retJson.c_str(), 0, &jsonErr);
    if (root == nullptr) {
        return ERR_INVALID;
    }

    json_t *tokJson = json_object_get(root, "token");
    if (tokJson == nullptr) {
        return ERR_INVALID;
    }

    accessKey_ = json_string_value(tokJson);
    json_decref(root);
    GLOGDEBUG << "Authentication succeded";
    return ERR_OK;
}

Error S3Client::createBucket(const std::string &bucketName)
{
    // TODO(Rao): Impmement this
    fds_panic("Not impl");
    return ERR_OK;
}

Error S3Client::putFile(const std::string &bucketName,
                           const std::string &objName,
                           const std::string &filePath)
{
    CURL *curl;
    CURLcode res;
    FILE * hd_src;
    struct stat file_info;
    Error ret = ERR_OK;
    int64_t httpCode = 200;

    std::string url;

    std::stringstream ss;
    ss << host_ << "/" << bucketName << "/" << objName;
    url = ss.str();

    /* get the file size of the local file */
    stat(filePath.c_str(), &file_info);

    hd_src = fopen(filePath.c_str(), "rb");
    if (hd_src == nullptr) {
        GLOGWARN << "Failed to open file: " << filePath;
        return ERR_INVALID;
    }

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, &S3Client::putFileCb);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                         (curl_off_t)file_info.st_size);
        if (isHttps_) {
            /* Disable verificatoins */
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        res = curl_easy_perform(curl);
        // curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        /* Check for errors */
        if (res != CURLE_OK || httpCode != 200) {
            GLOGWARN<< "curl_easy_perform() failed.  Res: " << res
                << " httpstatus: " << httpCode
                << "error msg: " << curl_easy_strerror(res);
            ret = ERR_INVALID;
        }
        curl_easy_cleanup(curl);
    }
    fclose(hd_src); /* close the local file */

    return ret;
}

Error S3Client::getFile(const std::string &bucketName,
                           const std::string &objName,
                           const std::string &filePath)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    std::string url;
    Error ret = ERR_OK;
    std::stringstream ss;
    int64_t httpCode = 200;

    ss << host_ << "/" << bucketName << "/" << objName;
    url = ss.str();

    fp = fopen(filePath.c_str(), "wb");
    if (fp == nullptr) {
        GLOGWARN << "Failed to open file: " << filePath;
        return ERR_INVALID;
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, getFileCb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        if (isHttps_) {
            /* Disable verificatoins */
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        /* Check for errors */
        if (res != CURLE_OK || httpCode != 200) {
            GLOGWARN<< "curl_easy_perform() failed.  Res: " << res
                << " httpstatus: " << httpCode
                << "error msg: " << curl_easy_strerror(res);
            ret = ERR_INVALID;
        }
        curl_easy_cleanup(curl);
    }
    fclose(fp);
    return ret;
}

Error S3Client::curlGet_(const std::string &url, std::string &retData)
{
    CURL *curl;
    CURLcode res;
    Error err = ERR_OK;
    struct CurlMemoryStruct chunk;
    /* will be grown as needed by the realloc above */
    chunk.memory = reinterpret_cast<char*>(malloc(1));
    chunk.size = 0;    /* no data at this point */

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &S3Client::curlGetMemoryCb);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void *>(&chunk));

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            GLOGWARN << curl_easy_strerror(res);
            err = ERR_INVALID;
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    if (chunk.memory) {
        if (err == ERR_OK) {
            retData = chunk.memory;
        }
        free(chunk.memory);
    }

    return err;
}

size_t
S3Client::curlGetMemoryCb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct CurlMemoryStruct *mem = (struct CurlMemoryStruct*)userp;

    mem->memory = reinterpret_cast<char*>(realloc(mem->memory, mem->size + realsize + 1));
    if (mem->memory == NULL) {
        /* out of memory! */
        GLOGWARN << "not enough memory (realloc returned NULL)";
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

size_t S3Client::putFileCb(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t retcode;
    curl_off_t nread;

    retcode = fread(ptr, size, nmemb, reinterpret_cast<FILE*>(stream));

    nread = (curl_off_t)retcode;

    return retcode;
}

size_t S3Client::getFileCb(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}


}  // namespace fds
