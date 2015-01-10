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
#include <S3Client.h>

namespace fds {

S3Client::S3Client(const std::string &host,
                   const std::string &accessKey,
                   const std::string &secretKey)
{
    host_ = host;
    accessKey_ = accessKey;
    secretKey_ = secretKey;
    curl_global_init(CURL_GLOBAL_ALL);
}

S3Client::~S3Client()
{
    curl_global_cleanup();
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
    uint32_t httpCode = 0;

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
    uint32_t httpCode = 0;

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
