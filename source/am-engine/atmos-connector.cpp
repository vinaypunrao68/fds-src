/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/atmos-connector.h>

#include <string>
#include <fds_assert.h>
#include <shared/fds_types.h>

namespace fds {

AMEngine_Atmos gl_AMEngineAtmos("AM Atmos Engine");

// ---------------------------------------------------------------------------

Atmos_GetObject::Atmos_GetObject(AMEngine *eng, AME_HttpReq *req)
    : Conn_GetObject(eng, req)
{
}

Atmos_GetObject::~Atmos_GetObject()
{
}

int
Atmos_GetObject::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string Atmos_GetObject::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

std::string Atmos_GetObject::get_object_id()
{
    return ame_http.getURIParts()[1];
}

// ---------------------------------------------------------------------------

Atmos_PutObject::Atmos_PutObject(AMEngine *eng, AME_HttpReq *req)
    : Conn_PutObject(eng, req)
{
}

Atmos_PutObject::~Atmos_PutObject()
{
}

int
Atmos_PutObject::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string Atmos_PutObject::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

std::string Atmos_PutObject::get_object_id()
{
    return ame_http.getURIParts()[1];
}

// ---------------------------------------------------------------------------

Atmos_DelObject::Atmos_DelObject(AMEngine *eng, AME_HttpReq *req)
    : Conn_DelObject(eng, req)
{
}

Atmos_DelObject::~Atmos_DelObject()
{
}

int
Atmos_DelObject::ame_format_response_hdr()
{
    if (ame_resp_status == NGX_HTTP_OK) {
        ame_set_std_resp(NGX_HTTP_NO_CONTENT,
                         ame_req->headers_out.content_length_n);
    }
    return NGX_OK;
}

std::string Atmos_DelObject::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

std::string Atmos_DelObject::get_object_id()
{
    return ame_http.getURIParts()[1];
}

// ---------------------------------------------------------------------------

Atmos_GetBucket::Atmos_GetBucket(AMEngine *eng, AME_HttpReq *req)
    : Conn_GetBucket(eng, req)
{
}

Atmos_GetBucket::~Atmos_GetBucket()
{
}

int
Atmos_GetBucket::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string Atmos_GetBucket::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}
// ---------------------------------------------------------------------------

Atmos_PutBucket::Atmos_PutBucket(AMEngine *eng, AME_HttpReq *req)
    : Conn_PutBucket(eng, req)
{
}

Atmos_PutBucket::~Atmos_PutBucket()
{
}

int
Atmos_PutBucket::ame_format_response_hdr()
{
    return NGX_OK;
}

std::string Atmos_PutBucket::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

// ---------------------------------------------------------------------------

Atmos_DelBucket::Atmos_DelBucket(AMEngine *eng, AME_HttpReq *req)
    : Conn_DelBucket(eng, req)
{
}

Atmos_DelBucket::~Atmos_DelBucket()
{
}

int
Atmos_DelBucket::ame_format_response_hdr()
{
    if (ame_resp_status == NGX_HTTP_OK) {
        ame_set_std_resp(NGX_HTTP_NO_CONTENT,
                         ame_req->headers_out.content_length_n);
    }
    return NGX_OK;
}

std::string
Atmos_DelBucket::get_bucket_id()
{
    return ame_http.getURIParts()[0];
}

}   // namespace fds
