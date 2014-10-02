/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NGINX_DRIVER_HTTP_UTILS_H_
#define SOURCE_INCLUDE_NGINX_DRIVER_HTTP_UTILS_H_
#include <vector>
#include <string>
#include <fds_types.h>

extern "C" {
#include <ngx_http.h>
#include <ngx_md5.h>
}

namespace fds {

class HttpUtils {
  public:
    static void initEtag(ngx_md5_t *ctx);
    static void updateEtag(ngx_md5_t *ctx, const char* data, size_t len);
    static std::string finalEtag(ngx_md5_t *ctx);
    static std::string computeEtag(const char* data, size_t len);
};

class HttpRequest {
  public:
    explicit HttpRequest(ngx_http_request_t* ngx_req);
    std::vector<std::string>& getURIParts();
    void appendURIPart(const std::string &uri);
    const std::vector<std::string>& getURIParts() const;
    uint32_t getMethod() const;
    ngx_http_request_t* getNginxReq() const;

    // getReqHdrVal
    // --------------------
    // Return the value corresponding with the key in the request header.
    //
    fds_bool_t getReqHdrVal(char const *const key, std::string *data) const;

    std::string toString() const;
  private:
    std::vector<std::string> _uriParts;
    ngx_http_request_t* _pReq;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_NGINX_DRIVER_HTTP_UTILS_H_
