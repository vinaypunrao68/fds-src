/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <nginx-driver/s3connector.h>
#include <am-plugin.h>

#include <string>
#include <fds_assert.h>
#include <shared/fds_types.h>

namespace fds {

AMEngine_S3 gl_AMEngineS3("AM S3 Engine");

// ---------------------------------------------------------------------------

S3_GetObject::S3_GetObject(AMEngine *eng, AME_HttpReq *req)
    : Conn_GetObject(eng, req)
{
}

S3_GetObject::~S3_GetObject()
{
}

int
S3_GetObject::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string S3_GetObject::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

std::string S3_GetObject::get_object_id()
{
    return ame_http.getURIParts()[1];
}

// ---------------------------------------------------------------------------

S3_PutObject::S3_PutObject(AMEngine *eng, AME_HttpReq *req)
    : Conn_PutObject(eng, req)
{
}

S3_PutObject::~S3_PutObject()
{
}

int
S3_PutObject::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string S3_PutObject::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

std::string S3_PutObject::get_object_id()
{
    return ame_http.getURIParts()[1];
}

// ---------------------------------------------------------------------------

S3_DelObject::S3_DelObject(AMEngine *eng, AME_HttpReq *req)
    : Conn_DelObject(eng, req)
{
}

S3_DelObject::~S3_DelObject()
{
}

int
S3_DelObject::ame_format_response_hdr()
{
    if (ame_resp_status == NGX_HTTP_OK) {
        ame_set_std_resp(NGX_HTTP_NO_CONTENT,
                         ame_req->headers_out.content_length_n);
    }
    return NGX_OK;
}

std::string S3_DelObject::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

std::string S3_DelObject::get_object_id()
{
    return ame_http.getURIParts()[1];
}

// ---------------------------------------------------------------------------

S3_GetBucket::S3_GetBucket(AMEngine *eng, AME_HttpReq *req)
    : Conn_GetBucket(eng, req)
{
}

S3_GetBucket::~S3_GetBucket()
{
}

int
S3_GetBucket::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string S3_GetBucket::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}
// ---------------------------------------------------------------------------

S3_PutBucket::S3_PutBucket(AMEngine *eng, AME_HttpReq *req)
    : Conn_PutBucket(eng, req)
{
}

S3_PutBucket::~S3_PutBucket()
{
}

int
S3_PutBucket::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string S3_PutBucket::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

// ---------------------------------------------------------------------------

S3_DelBucket::S3_DelBucket(AMEngine *eng, AME_HttpReq *req)
    : Conn_DelBucket(eng, req)
{
}

S3_DelBucket::~S3_DelBucket()
{
}

int
S3_DelBucket::ame_format_response_hdr()
{
    if (ame_resp_status == NGX_HTTP_OK) {
        ame_set_std_resp(NGX_HTTP_NO_CONTENT,
                         ame_req->headers_out.content_length_n);
    }
    return NGX_OK;
}

std::string
S3_DelBucket::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

}   // namespace fds
