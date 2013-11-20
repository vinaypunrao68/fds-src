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

S3_GetObject::S3_GetObject(ngx_http_request_t *req)
    : FDSN_GetObject(req)
{
}

S3_GetObject::~S3_GetObject()
{
}

ame_ret_e
S3_GetObject::ame_send_response_hdr()
{
}

} // namespace fds
