/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_S3_CONNECTOR_H_
#define INCLUDE_S3_CONNECTOR_H_

#include <am-engine/am-engine.h>

struct ngx_http_request_s;
namespace fds {

// S3 Get connector which does S3 specific object get semantic.
//
class S3_GetObject : public Conn_GetObject
{
  public:
    S3_GetObject(struct ngx_http_request_s *req);
    ~S3_GetObject();

    // Format response header in S3 protocol.
    //
    virtual ame_ret_e ame_format_response_hdr();

  protected:
    // List of known key-values that we'll need to send the GET response.
    //
    char *resp_xx_key;
    char *resp_xx_value;
};

} // namespace fds

#endif /* INCLUDE_AM_ENGINE_H_ */
