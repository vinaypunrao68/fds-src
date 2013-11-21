/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/s3connector.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
} // extern "C"

#include <iostream>
#include <shared/fds_types.h>
#include <fds_assert.h>
#include <am-plugin.h>

namespace fds {

S3_GetObject::S3_GetObject(HttpRequest &req)
    : Conn_GetObject(req)
{
}

S3_GetObject::~S3_GetObject()
{
}

ame_ret_e
S3_GetObject::ame_format_response_hdr()
{
}

std::string S3_GetObject::get_bucket_id()
{
  return ame_req.getURIParts()[0];
}

std::string S3_GetObject::get_object_id()
{
  return ame_req.getURIParts()[1];
}

} // namespace fds
