/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_
#include <vector>
#include <string>
#include <fds_types.h>

extern "C" {
#include <ngx_http.h>
#include <ngx_md5.h>
}

class HttpUtils {
public:
  static std::string computeEtag(const char* data, size_t len);
};

class HttpRequest {
public:
  HttpRequest(ngx_http_request_t* ngx_req);
  std::vector<std::string>& getURIParts() ;
  const std::vector<std::string>& getURIParts() const;
  uint32_t getMethod() const;
  ngx_http_request_t* getNginxReq() const;

  // getReqHdrVal
  // --------------------
  // Return the value corresponding with the key in the request header.
  //
  fds_bool_t getReqHdrVal(char const *const key, std::string& data) const;

  std::string toString() const;
private:
  std::vector<std::string> _uriParts;
  ngx_http_request_t* _pReq;
};

#endif //_HTTP_UTILS_H_
