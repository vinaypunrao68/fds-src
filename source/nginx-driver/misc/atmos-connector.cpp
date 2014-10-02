/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <nginx-driver/atmos-connector.h>

#include <string>
#include <vector>
#include <fds_assert.h>
#include <shared/fds_types.h>
#include <am-plugin.h>

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
    const char          *content_type = "application/octet-stream";

    ame_set_resp_keyval(sgt_AMEKey[RESP_CONTENT_TYPE].u.kv_key_name,
                        sgt_AMEKey[RESP_CONTENT_TYPE].kv_keylen,
                        const_cast<char *>(content_type),
                        std::strlen(content_type));

    return NGX_OK;
}

std::string Atmos_GetObject::get_bucket_id()
{
    if (!ame_http.getURIParts()[0].compare("rest") &&
        !ame_http.getURIParts()[1].compare("objects")) {
        return ("atmos");
    }
    fds_assert(!"Invalid Atmos URI");
    return ame_http.getURIParts()[0] + ame_http.getURIParts()[1];
}

std::string Atmos_GetObject::get_object_id()
{
    if (!ame_http.getURIParts()[0].compare("rest") &&
        !ame_http.getURIParts()[1].compare("objects")) {
        return ame_http.getURIParts()[2];
    }
    fds_assert(!"Invalid Atmos URI");
    return ame_http.getURIParts()[2];
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
    const char          *content_type = "text/plain; charset=UTF-8";

    std::vector<std::string> uriParts = ame_http.getURIParts();
    std::string         obj_loc = "/rest/objects/" + uriParts[2];

    ame_set_resp_keyval(sgt_AMEKey[RESP_CONTENT_TYPE].u.kv_key_name,
                        sgt_AMEKey[RESP_CONTENT_TYPE].kv_keylen,
                        const_cast<char *>(content_type),
                        std::strlen(content_type));
    ame_set_resp_keyval(sgt_AMEKey[RESP_LOCATION].u.kv_key_name,
                        sgt_AMEKey[RESP_LOCATION].kv_keylen,
                        const_cast<char *>(obj_loc.c_str()),
                        std::strlen(const_cast<char *>(obj_loc.c_str())));

    return NGX_OK;
}

static int
atmos_putobj_cbfn(void *reqContext, fds_uint64_t bufferSize, fds_off_t offset,
                  char *buffer, void *callbackData, FDSN_Status status,
                  ErrorDetails* errDetails)
{
    AME_Ctx        *ctx = static_cast<AME_Ctx *>(reqContext);
    Conn_PutObject *conn_po = static_cast<Conn_PutObject *>(callbackData);

    ctx->ame_update_input_buf(bufferSize);
    if (status == ERR_OK) {
        status = FDSN_StatusCreated;
    }
    conn_po->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
    return 0;
}

void
Atmos_PutObject::ame_request_handler()
{
    fds_uint32_t  len;
    char          *buf;
    FDS_NativeAPI *api;
    fds_uint64_t  offset = 0;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    buf = ame_reqt_iter_data(&len);
    if (buf == NULL || len == 0) {
        ame_finalize_request(NGX_HTTP_BAD_REQUEST);
        return;
    }
    // Create a fake transaction for now
    BlobTxId::ptr txDesc(new BlobTxId());

    api = ame->ame_fds_hook();
    api->PutBlob(&bucket_ctx, get_object_id(), NULL,
                 static_cast<void *>(ame_ctx), buf, offset,
                 len, txDesc, false,
                 atmos_putobj_cbfn, static_cast<void *>(this));
}

std::string Atmos_PutObject::get_bucket_id()
{
    if (!ame_http.getURIParts()[0].compare("rest") &&
        !ame_http.getURIParts()[1].compare("objects")) {
        return ("atmos");
    }
    fds_assert(!"Invalid Atmos URI");
    return ame_http.getURIParts()[0] + ame_http.getURIParts()[1];
}

std::string Atmos_PutObject::get_object_id()
{
    if (!ame_http.getURIParts()[0].compare("rest") &&
        !ame_http.getURIParts()[1].compare("objects")) {
        return ame_http.getURIParts()[2];
    }
    fds_assert(!"Invalid Atmos URI");
    return ame_http.getURIParts()[2];
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
    fds_assert(!"not implemented");
    return NULL;
    // return ame_http.getURIParts()[0];
}

std::string Atmos_DelObject::get_object_id()
{
    fds_assert(0);
    return ame_http.getURIParts()[2];
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
    fds_assert(!"not implemented");
    return NULL;
    // return ame_http.getURIParts()[0];
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
    fds_assert(!"not implemented");
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
    fds_assert(!"not implemented");
    return ame_http.getURIParts()[0];
}

}   // namespace fds
