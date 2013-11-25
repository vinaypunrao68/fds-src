/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/s3connector.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
} // extern "C"

#include <fds_assert.h>
#include <shared/fds_types.h>
#include <am-engine/s3connector.h>

namespace fds {

AMEngine_S3 gl_AMEngineS3("AM S3 Engine");

// ---------------------------------------------------------------------------

S3_GetObject::S3_GetObject(AMEngine *eng, HttpRequest &req)
    : Conn_GetObject(eng, req)
{
}

S3_GetObject::~S3_GetObject()
{
}

ame_ret_e
S3_GetObject::ame_format_response_hdr()
{
    return AME_OK;
}

std::string S3_GetObject::get_bucket_id()
{
  return ame_req.getURIParts()[0];
}

std::string S3_GetObject::get_object_id()
{
  return ame_req.getURIParts()[1];
}

// ---------------------------------------------------------------------------

S3_PutObject::S3_PutObject(AMEngine *eng, HttpRequest &req)
    : Conn_PutObject(eng, req)
{
}

S3_PutObject::~S3_PutObject()
{
}

ame_ret_e
S3_PutObject::ame_format_response_hdr()
{
    return AME_OK;
}

std::string S3_PutObject::get_bucket_id()
{
  return ame_req.getURIParts()[0];
}

std::string S3_PutObject::get_object_id()
{
  return ame_req.getURIParts()[1];
}

// ---------------------------------------------------------------------------

S3_DelObject::S3_DelObject(AMEngine *eng, HttpRequest &req)
    : Conn_DelObject(eng, req)
{
}

S3_DelObject::~S3_DelObject()
{
}

ame_ret_e
S3_DelObject::ame_format_response_hdr()
{
    return AME_OK;
}

std::string S3_DelObject::get_bucket_id()
{
  return ame_req.getURIParts()[0];
}

std::string S3_DelObject::get_object_id()
{
  return ame_req.getURIParts()[1];
}

// ---------------------------------------------------------------------------

S3_GetBucket::S3_GetBucket(AMEngine *eng, HttpRequest &req)
    : Conn_GetBucket(eng, req)
{
}

S3_GetBucket::~S3_GetBucket()
{
}

ame_ret_e
S3_GetBucket::ame_format_response_hdr()
{
    return AME_OK;
}

// ---------------------------------------------------------------------------

S3_PutBucket::S3_PutBucket(AMEngine *eng, HttpRequest &req)
    : Conn_PutBucket(eng, req)
{
}

S3_PutBucket::~S3_PutBucket()
{
}

ame_ret_e
S3_PutBucket::ame_format_response_hdr()
{
  return AME_OK;
}

std::string S3_PutBucket::get_bucket_id()
{
  return ame_req.getURIParts()[0];
}

// ---------------------------------------------------------------------------

S3_DelBucket::S3_DelBucket(AMEngine *eng, HttpRequest &req)
    : Conn_DelBucket(eng, req)
{
}

S3_DelBucket::~S3_DelBucket()
{
}

ame_ret_e
S3_DelBucket::ame_format_response_hdr()
{
  return AME_OK;
}

std::string S3_DelBucket::get_bucket_id()
{
  return ame_req.getURIParts()[0];
}

} // namespace fds
