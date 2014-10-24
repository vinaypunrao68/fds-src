/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <nginx-driver/am-engine.h>
#include <nginx-driver/http_utils.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}   // extern "C"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <shared/fds_types.h>
#include <fds_assert.h>
#include <fds_process.h>

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <string>
#include <iostream>

#include <util/fds_stat.h>
#include <util/timeutils.h>
#include <am-plugin.h>
#include <native_api.h>
#include <util/Log.h>

namespace fds {

const int    NGINX_ARG_PARAM = 80;
const int    NGX_RESP_CHUNK_SIZE = (16 << 10);
static char  nginx_prefix[NGINX_ARG_PARAM];
static char  nginx_config[NGINX_ARG_PARAM];
static char  nginx_signal[NGINX_ARG_PARAM];

static const stat_decode_t stat_ngx_decode[] =
{
    { STAT_NGX_GET,             "Nginx GET E2E" },
    { STAT_NGX_GET_FDSN,        "Nginx reached FDSN GET" },
    { STAT_NGX_GET_FDSN_RET,    "Nginx returned from FDSN GET" },
    { STAT_NGX_GET_FDSN_CB,     "Nginx received cb from FDSN" },
    { STAT_NGX_GET_RESUME,      "Nginx resumed GET event" },

    { STAT_NGX_PUT,             "Nginx PUT E2E" },
    { STAT_NGX_PUT_FDSN,        "Nginx reached FDSN PUT" },
    { STAT_NGX_PUT_FDSN_RET,    "Nginx returned from FDSN PUT" },
    { STAT_NGX_PUT_FDSN_CB,     "Nginx received cb from FDSN" },
    { STAT_NGX_PUT_RESUME,      "Nginx resumed PUT event" },

    { STAT_NGX_DEL,             "Nginx DEL E2E" },
    { STAT_NGX_DEL_FDSN,        "Nginx reached FDSN DEL" },
    { STAT_NGX_DEL_FDSN_RET,    "Nginx returned from FDSN DEL" },
    { STAT_NGX_DEL_FDSN_CB,     "Nginx received cb from FDSN" },
    { STAT_NGX_DEL_RESUME,      "Nginx resumed DEL event" },

    { STAT_NGX_GET_BK,          "Nginx GET bucket E2E" },
    { STAT_NGX_GET_BK_FDSN,     "Nginx reached FDSN GET bucket" },
    { STAT_NGX_GET_BK_FDSN_RET, "Nginx returned from FDSN GET bucket" },
    { STAT_NGX_GET_BK_FDSN_CB,  "Nginx received cb from FDSN" },
    { STAT_NGX_GET_BK_RESUME,   "Nginx resumed GET bucket event" },

    { STAT_NGX_PUT_BK,          "Nginx PUT bucket E2E" },
    { STAT_NGX_PUT_BK_FDSN,     "Nginx reached FDSN PUT bucket" },
    { STAT_NGX_PUT_BK_FDSN_RET, "Nginx returned from FDSN PUT bucket" },
    { STAT_NGX_PUT_BK_FDSN_CB,  "Nginx received cb from FDSN" },
    { STAT_NGX_PUT_BK_RESUME,   "Nginx resumed PUT bucket event" },

    { STAT_NGX_DEL_BK,          "Nginx DEL bucket E2E" },
    { STAT_NGX_DEL_BK_FDSN,     "Nginx reached FDSN DEL bucket" },
    { STAT_NGX_DEL_BK_FDSN_RET, "Nginx returned from FDSN DEL bucket" },
    { STAT_NGX_DEL_BK_FDSN_CB,  "Nginx received cb from FDSN" },
    { STAT_NGX_DEL_BK_RESUME,   "Nginx resumed DEL bucket event" },

    { STAT_NGX_DEFAULT,         "Untraced ngx command" },
    { STAT_NGX_POINT_MAX,       NULL }
};

static char const *nginx_start_argv[] =
{
    nullptr,
    "-p", nginx_prefix,
    "-c", nginx_config
};

static char const *nginx_signal_argv[] =
{
    nullptr,
    "-s", nginx_signal
};

// mod_init
// --------
//
int
AMEngine::mod_init(SysParams const *const p)
{
    namespace po = boost::program_options;

    Module::mod_init(p);
    po::variables_map       vm;
    po::options_description desc("Access Manager Options");

    desc.add_options()
            ("help", "Show this help text")
            ("signal", po::value<std::string>(&eng_signal),
             "Send signal to access manager: stop, quit, reopen, reload");

    po::store(po::command_line_parser(p->p_argc, p->p_argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    if (vm.count("signal")) {
        if ((eng_signal != "stop") && (eng_signal != "quit") &&
            (eng_signal != "reopen") && (eng_signal != "reload")) {
            std::cout << "Expect --signal stop|quit|reopen|reload" << std::endl;
            return 1;
        }
    }
    // Fix up the key table setup.
    for (int i = 0; sgt_AMEKey[i].kv_idx != AME_HDR_KEY_MAX; i++) {
        fds_verify(sgt_AMEKey[i].kv_idx == i);
        sgt_AMEKey[i].kv_keylen = strlen(sgt_AMEKey[i].u.kv_key);
    }
    return 0;
}

// mod_startup
// -----------
//
void
AMEngine::mod_startup()
{
    static int __reg = 0;
    // FIXME: each engine type should have their own stat.
    if (!__reg) {
        __reg = 1;
        gl_fds_stat.stat_reg_mod(STAT_NGX, stat_ngx_decode);
    }
}

void
AMEngine::init_server(FDS_NativeAPI *api)
{
    const std::string *fds_root;
    char path[NGINX_ARG_PARAM];

    fds_assert(eng_api == NULL && api);
    eng_api = api;
    fds_root = &mod_params->fds_root;
    if (eng_signal != "") {
        nginx_signal_argv[0] = mod_params->p_argv[0];
        strncpy(nginx_signal, eng_signal.c_str(), NGINX_ARG_PARAM);
        ngx_main(FDS_ARRAY_ELEM(nginx_signal_argv), nginx_signal_argv);
        return;
    }
    snprintf(nginx_config, NGINX_ARG_PARAM, "%s/etc/fds.conf", fds_root->c_str());
    snprintf(nginx_prefix, NGINX_ARG_PARAM, "%s/nginx", fds_root->c_str());

    // Create all directories if they are not exists.
    snprintf(path, NGINX_ARG_PARAM, "%s/logs", nginx_prefix);
    FdsRootDir::fds_mkdir(path);

    snprintf(path, NGINX_ARG_PARAM, "%s/etc", fds_root->c_str());
    FdsRootDir::fds_mkdir(path);

    ngx_register_plugin(api->clientType, this);
    nginx_start_argv[0] = mod_params->p_argv[0];
}

// run_server
// ----------
//
void
AMEngine::run_server(fds::FDS_NativeAPI *api)
{
    init_server(api);
    ngx_main(FDS_ARRAY_ELEM(nginx_start_argv), nginx_start_argv);
}

void
AMEngine::run_all_servers()
{
    ngx_main(FDS_ARRAY_ELEM(nginx_start_argv), nginx_start_argv);
}

void
AMEngine::mod_shutdown()
{
}

Conn_PutBucketParams *AMEngine::ame_putbucketparams_hdler(AME_HttpReq *req) {
    return new Conn_PutBucketParams(this, req);
}

Conn_GetBucketStats *AMEngine::ame_getbucketstats_hdler(AME_HttpReq *req) {
    return new Conn_GetBucketStats(this, req);
}

// ---------------------------------------------------------------------------
// Generic request/response protocol through NGINX module.
//
// Reference URL:
// http://docs.aws.amazon.com/AmazonS3/latest/API/
//      RESTCommonResponseHeaders.html
//      RESTBucketGET.html
// ---------------------------------------------------------------------------
ame_keytab_t sgt_AMEKey[] =
{
    { { "Content-Length" },      0, RESP_CONTENT_LEN },
    { { "Content-Type" },        0, RESP_CONTENT_TYPE },
    { { "Connection" },          0, RESP_CONNECTION },
    { { "open" },                0, RESP_CONNECTION_OPEN },
    { { "close" },               0, RESP_CONNECTION_CLOSE },
    { { "Etag" },                0, RESP_ETAG },
    { { "Date" },                0, RESP_DATE },
    { { "Server" },              0, RESP_SERVER },
    { { "Location" },            0, RESP_LOCATION},

    // RESTBucketGET response keys
    { { "ListBucketResult" },    0, REST_LIST_BUCKET },
    { { "Name" },                0, REST_NAME },
    { { "Prefix" },              0, REST_PREFIX },
    { { "Marker" },              0, REST_MARKER },
    { { "MaxKeys" },             0, REST_MAX_KEYS },
    { { "IsTruncated" },         0, REST_IS_TRUNCATED },
    { { "Contents" },            0, REST_CONTENTS },
    { { "Key" },                 0, REST_KEY },
    { { "Etag" },                0, REST_ETAG },
    { { "Size" },                0, REST_SIZE },
    { { "StorageClass" },        0, REST_STORAGE_CLASS },
    { { "Owner" },               0, REST_OWNER },
    { { "ID" },                  0, REST_ID },
    { { "DisplayName" },         0, REST_DISPLAY_NAME },

    // Bucket Stats/Policy response keys -- our own
    { { "priority" },            0, RESP_QOS_PRIORITY },
    { { "sla" },                 0, RESP_QOS_SLA },
    { { "limit" },               0, RESP_QOS_LIMIT },
    { { "performance" },         0, RESP_QOS_PERFORMANCE },
    { { "time" },                0, RESP_STATS_TIME },
    { { "volumes" },             0, RESP_STATS_VOLS },
    { { "id" },                  0, RESP_STATS_ID },

    { { nullptr },               0, AME_HDR_KEY_MAX }
};

int AME_Request::ame_map_fdsn_status(FDSN_Status status) {
    LOGNORMAL << "fdsn status : " << status;
    switch (status) {
        case ERR_OK                                         : return NGX_HTTP_OK;
        case FDSN_StatusCreated                             : return NGX_HTTP_CREATED;

        case FDSN_StatusInvalidBucketNameTooLong            :
        case FDSN_StatusInvalidBucketNameFirstCharacter     :
        case FDSN_StatusInvalidBucketNameCharacter          :
        case FDSN_StatusInvalidBucketNameCharacterSequence  :
        case FDSN_StatusInvalidBucketNameTooShort           :
        case FDSN_StatusInvalidBucketNameDotQuadNotation    :
        case FDSN_StatusQueryParamsTooLong                  :
        case FDSN_StatusMetaDataHeadersTooLong              :
        case FDSN_StatusBadMetaData                         :
        case FDSN_StatusBadContentType                      :
        case FDSN_StatusContentTypeTooLong                  :
        case FDSN_StatusBadMD5                              :
        case FDSN_StatusMD5TooLong                          :
        case FDSN_StatusBadCacheControl                     :
        case FDSN_StatusCacheControlTooLong                 :
        case FDSN_StatusBadContentDispositionFilename       :
        case FDSN_StatusContentDispositionFilenameTooLong   :
        case FDSN_StatusBadContentEncoding                  :
        case FDSN_StatusContentEncodingTooLong              :
        case FDSN_StatusBadIfMatchETag                      :
        case FDSN_StatusIfMatchETagTooLong                  :
        case FDSN_StatusBadIfNotMatchETag                   :
        case FDSN_StatusIfNotMatchETagTooLong               :
        case FDSN_StatusHeadersTooLong                      :
        case FDSN_StatusKeyTooLong                          :
        case FDSN_StatusUriTooLong                          :
        case FDSN_StatusXmlParseFailure                     :
        case FDSN_StatusEmailAddressTooLong                 :
        case FDSN_StatusUserIdTooLong                       :
        case FDSN_StatusUserDisplayNameTooLong              :
        case FDSN_StatusGroupUriTooLong                     :
        case FDSN_StatusPermissionTooLong                   :
        case FDSN_StatusTargetBucketTooLong                 :
        case FDSN_StatusTargetPrefixTooLong                 :
        case FDSN_StatusTooManyGrants                       :
        case FDSN_StatusBadGrantee                          :
        case FDSN_StatusBadPermission                       :
        case FDSN_StatusXmlDocumentTooLarge                 :
        case FDSN_StatusErrorAmbiguousGrantByEmailAddress   :
        case FDSN_StatusErrorBadDigest                      :
        case FDSN_StatusErrorEntityTooSmall                 :
        case FDSN_StatusErrorEntityTooLarge                 : return NGX_HTTP_BAD_REQUEST; //NOLINT

        case FDSN_StatusRequestTimedOut                     : return NGX_HTTP_GATEWAY_TIME_OUT; //NOLINT
        case FDSN_StatusEntityEmpty                         : return NGX_HTTP_NO_CONTENT;

        case FDSN_StatusErrorAccessDenied                   :
        case FDSN_StatusErrorAccountProblem                 :
        case FDSN_StatusErrorCrossLocationLoggingProhibited : return NGX_HTTP_FORBIDDEN;

        case FDSN_StatusErrorBucketAlreadyExists            :
        case FDSN_StatusErrorBucketAlreadyOwnedByYou        :
        case FDSN_StatusErrorBucketNotEmpty                 : return NGX_HTTP_CONFLICT;

        case FDSN_StatusEntityDoesNotExist                  :
        case FDSN_StatusErrorBucketNotExists                : return NGX_HTTP_NOT_FOUND;

        case FDSN_StatusInternalError                       :
        case FDSN_StatusOutOfMemory                         :
        case FDSN_StatusInterrupted                         :
        case FDSN_StatusNameLookupError                     :
        case FDSN_StatusFailedToConnect                     :
        case FDSN_StatusServerFailedVerification            :
        case FDSN_StatusConnectionFailed                    :
        case FDSN_StatusAbortedByCallback                   : return NGX_HTTP_INTERNAL_SERVER_ERROR; //NOLINT

        case FDSN_StatusErrorMissingContentLength           : return NGX_HTTP_LENGTH_REQUIRED; //NOLINT

        default :
            LOGWARN << "unknown error [" << status << "]";
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
}

AME_Request::AME_Request(AMEngine *eng, AME_HttpReq *req)
        : fdsio::Request(true), ame(eng), ame_req(req),
          ame_http(req), ame_finalize(false)
{
    ame_clk_all     = fds::util::getClockTicks();
    ame_stat_pt     = STAT_NGX_DEFAULT;
    ame_clk_fdsn    = 0;
    ame_clk_fdsn_cb = 0;

    ame_resp_status = 500;
    eng->ame_get_queue()->rq_enqueue(this, 0);
}

AME_Request::~AME_Request()
{
    req_complete();
    fds_stat_record(STAT_NGX, ame_stat_pt, ame_clk_all, fds::util::getClockTicks());
}

// ame_add_context
// ---------------
//
void
AME_Request::ame_add_context(AME_Ctx *ctx)
{
    ame_ctx = ctx;
    if (ame_req->request_body && ame_req->request_body->bufs) {
        ame_ctx->ame_save_input_buf(ame_req->request_body->bufs);
    }
}

// ame_signal_resume
// -----------------
// Called by FDSN to signal the resumption of the event loop.
//
void
AME_Request::ame_signal_resume(int status)
{
    ame_resp_status = status;

    // This will cause the event loop to run the request_resume() method.
    ame_ctx->ame_notify_handler();
}

void
AME_Request::appendURIPart(const std::string &uri)
{
    ame_http.appendURIPart(uri);
}

// ame_reqt_iter_reset
// -------------------
//
void
AME_Request::ame_reqt_iter_reset()
{
}

// ame_reqt_iter_next
// ------------------
//
int
AME_Request::ame_reqt_iter_next()
{
    // todo: rao implement this to the post/put client buffer
    return NGX_OK;
}

/**
 * Gets next data from request buffer.
 *
 * @param[in] data_len Max bytes to return
 * @param[out] len Bytes actually returned
 * @return pointer to buf, NULL if no data.
 */
char *
AME_Request::ame_reqt_iter_data_next(fds_uint32_t data_len,
                                     fds_uint32_t *len)
{
    LOGDEBUG << "content-length : " << ame_req->headers_in.content_length_n
             << ":" << ame_req->headers_in.content_length;
    fds_uint32_t content_len = ame_req->headers_in.content_length_n;

    if (content_len < data_len) {
        // If the content is smaller than the max buf
        // adjust the buf to the smaller size
        data_len = content_len;
    }

    // TODO(Andrew): Don't call this each time to reset the
    // ack count and don't assume the length won't change
    fds_uint32_t ack_count = 0;
    if (content_len > 0 && data_len > 0) {
        ack_count = content_len / data_len;
        if ((content_len % data_len) != 0) {
            // There's a remainder that needs to be acked
            ack_count++;
        }
    } else {
        ack_count++;
    }
    ame_ctx->set_ack_count(ack_count);
    *len = data_len;

    LOGDEBUG << " content-len:" << content_len
             << " data_len:" << data_len
             << " ack_count:" << ack_count
             << " len:" << *len;

    char *data  = ame_ctx->ame_next_buf_ptr(len);

    fds_verify(*len <= data_len);

    if ((data != NULL) && (*len < data_len)) {
        // We could not get all of the requested data
        // in one shot because the data is not in a
        // single nginx buffer. Since the caller expects
        // a single buffer, we need to copy these
        char *ame_data = static_cast<char *>(ngx_palloc(ame_req->pool, data_len));
        fds_verify(ame_data != NULL);

        fds_uint32_t copy_len = 0;
        while (data != NULL) {
            ngx_memcpy(ame_data + copy_len, data, *len);
            copy_len += *len;

            // Get a pointer to the next data even
            // if it's in another nginx buffer
            data = ame_ctx->ame_next_buf_ptr(len);
            fds_verify(*len <= data_len);
            // LOGTRACE << "Copied " << copy_len
            //         << " bytes from nginx buf into ame buf";
        }
        fds_verify(copy_len <= data_len);

        // Track this allocation in the ctx so that we can
        // free it later
        ame_ctx->ame_add_alloc_buf(ame_data, data_len);

        // Return the single buffer with all of the data
        // from the fragmented nginx buffers
        *len = copy_len;
        return ame_data;
    }

    return data;
}

/**
 * Gets a buffer from ngx buf pool and
 * fills with next data.
 */
char *
AME_Request::ame_reqt_iter_data(fds_uint32_t *len)
{
    char *data, *curr;
    fds_uint32_t   data_len, copy_len;

    if (ame_ctx->ame_curr_input_buf(len) == NULL) {
        return NULL;
    }

    // Temporary code so that we return a single buffer containing all
    // content in a body.
    *len = 0;
    copy_len = 0;
    data_len = ame_req->headers_in.content_length_n;

    data = static_cast<char *>(ngx_palloc(ame_req->pool, data_len));
    fds_verify(data != NULL);

    do {
        curr = ame_ctx->ame_curr_input_buf(&copy_len);
        fds_verify(curr != NULL);

        ngx_memcpy(&data[*len], curr, copy_len);
        *len += copy_len;
    } while (ame_ctx->ame_next_input_buf() == true);

    fds_verify(*len == data_len);

    // Save this so that we can free it later.
    ame_ctx->ame_temp_buf = data;
    return data;
}

// ame_set_resp_keyval
// -------------------
//
int
AME_Request::ame_set_resp_keyval(char *k, int klen, char *v, int vlen)
{
    ngx_table_elt_t    *h;
    ngx_http_request_t *r;

    r = ame_req;
    h = static_cast<ngx_table_elt_t *>(ngx_list_push(&r->headers_out.headers));

    h->key.len    = klen;
    h->key.data   = reinterpret_cast<u_char *>(k);
    h->hash       = ngx_hash_key_lc(h->key.data, h->key.len);
    h->value.len  = vlen;
    h->value.data = reinterpret_cast<u_char *>(v);

    return NGX_OK;
}

// ame_set_std_resp
// ----------------
// Common code path to set standard response to client.
//
int
AME_Request::ame_set_std_resp(int status, int len)
{
    // int                used;
    // char               *buf;
    ngx_http_request_t *r;

    r = ame_req;
    r->headers_out.status           = status;
    r->headers_out.content_length_n = len;
    if (len == 0) {
        r->header_only = 1;
    }
    return NGX_OK;
}

// ame_send_response_hdr
// ---------------------
// Common code path to send the complete response header to client.
//
int
AME_Request::ame_send_response_hdr()
{
    ngx_int_t          rc;
    ngx_http_request_t *r;

    // Any protocol-connector specific response format.
    ame_format_response_hdr();

    if (ame_etag.size() > 0) {
        ame_set_resp_keyval(sgt_AMEKey[REST_ETAG].u.kv_key_name,
                            sgt_AMEKey[REST_ETAG].kv_keylen,
                            const_cast<char *>(ame_etag.c_str()), ame_etag.size());
    }
    // Do actual sending.
    r  = ame_req;
    rc = ngx_http_send_header(r);
    if (ame_finalize == true || r->header_only) {
        ngx_http_finalize_request(r, rc);
    }
    return NGX_OK;
}

// ame_finalize_response
// ---------------------
// Common path to finalize the request by sending the response w/out data back
// to client.
void
AME_Request::ame_finalize_response(int status)
{
    ame_set_std_resp(status, 0);
    ame_send_response_hdr();
}

// ame_send_resp_data
// ------------------
//
int
AME_Request::ame_send_resp_data(ame_buf_t *buf, int len, fds_bool_t last)
{
    ngx_int_t          rc;
    ngx_chain_t        out;
    ngx_http_request_t *r;

    r = ame_req;
    buf->last = buf->pos + len;

    if (last == true) {
        buf->last_buf      = 1;
        buf->last_in_chain = 1;
    }
    out.buf  = buf;
    out.next = NULL;

    rc = ngx_http_output_filter(r, &out);
    LOGDEBUG << "return code:" << rc;
    if (last) {
        ngx_http_finalize_request(r, rc);
        LOGDEBUG << "last return code:" << rc;
    }
    return NGX_OK;
}

// ame_finalize_request
// --------------------
// Finalize request to cut short the processing to return back to client with
// error code.
void
AME_Request::ame_finalize_request(int status)
{
    // TODO(rao): map status back to NGX_ rc error code.
    ngx_http_finalize_request(ame_req, status);

    // Will free this object and its context.
    ame_ctx->ame_free_context();
}

// ---------------------------------------------------------------------------
// GetObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_GetObject::Conn_GetObject(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
    ame_stat_pt = STAT_NGX_GET;

    // TODO(Andrew): Don't hard code
    get_obj_max_buf_len = 2 * 1024 * 1024;
}

Conn_GetObject::~Conn_GetObject()
{
}

fds_uint32_t
Conn_GetObject::get_max_buf_len() const {
    return get_obj_max_buf_len;
}

// fdsn_getobj_cbfn
// ----------------
//
static FDSN_Status
fdsn_getobj_cbfn(BucketContextPtr bucket_ctx,
                 void *req, fds_uint64_t bufSize, fds_off_t offset,
                 const char *buf, fds_uint64_t blobSize,
                 const std::string &blobEtag, void *cbData,
                 FDSN_Status status, ErrorDetails *errdetails)
{
    Error          err(ERR_OK);
    AME_Ctx        *ctx = static_cast<AME_Ctx *>(req);
    Conn_GetObject *conn_go = static_cast<Conn_GetObject *>(cbData);
    FDS_NativeAPI  *api = conn_go->ame_get_ame()->ame_fds_hook();

    conn_go->ame_clk_fdsn_cb = fds::util::getClockTicks();
    fds_stat_record(STAT_NGX, STAT_NGX_GET_FDSN_CB,
                    conn_go->ame_clk_fdsn, conn_go->ame_clk_fdsn_cb);

    LOGNORMAL << "Received FDSN get() callback for blob " << conn_go->get_object_id()
              << " with data at offset " << offset << " of length " << bufSize
              << " with total blob size " << blobSize << " and result "
              << status;

    // TODO(Andrew): If one of the streaming gets() failed, we
    // will abort the stream, but may have sent the header back
    // already and may have stream info to still clean up...
    if (status == ERR_OK) {
        fds_verify(bufSize <= blobSize);

        // Update the ctx with whatever etag we received for the blob
        ctx->ame_set_etag_result(blobEtag);

        // If it's not already set, set how many bytes we need before
        // calling the request complete. If we just set the specific
        // size, we need to issue additional requests if we haven't
        // reached the requested size
        fds_bool_t allWasSet = ctx->ame_set_specific(blobSize);

        // Update the get buffer map with how much we received
        ctx->ame_set_get_len(offset, bufSize);

        if (allWasSet) {
            // We just received the first, of possibly many gets
            // Issue the remaining gets
            while (ctx->ame_get_offset() < ctx->ame_get_requested_len()) {
                fds_off_t ame_cur_offset = ctx->ame_get_off();

                fds_uint32_t buf_req_len = ctx->ame_get_requested_len() - ame_cur_offset;
                if (buf_req_len > conn_go->get_max_buf_len()) {
                    buf_req_len = conn_go->get_max_buf_len();
                }

                int  len;
                char *adr;
                // Get an AME buffer from nginx to read into
                ame_buf_t *getBuf = ctx->ame_alloc_buf(buf_req_len, &adr, &len);
                fds_verify(getBuf != NULL);
                fds_verify((fds_uint32_t)len == buf_req_len);

                // Store a link to the buffer
                // Note, this assumes the buffers are linked
                // in offset order...we will return data to nginx
                // in this order
                ctx->ame_add_get_buf(ame_cur_offset, getBuf);

                // Update the context with the waiting request
                Error err = ctx->ame_add_ctx_req(ame_cur_offset);
                fds_verify(err == ERR_OK);

                // Move the offset forward for the (possible) next read
                ctx->ame_mv_off(buf_req_len);

                // Issue the get() request with the desired read length set to the
                // current buffer length since we don't know how much data to read
                // api->GetObject(bucket_ctx, conn_go->get_object_id(), NULL,
                //            ame_cur_offset,
                //             buf_req_len, adr, buf_req_len,
                //             static_cast<void *>(ctx), fdsn_getobj_cbfn,
                //             static_cast<void *>(conn_go));
            }
        }  // Ends if (wasAllSet)
    }  // Ends if (status == FDSN_Status_OK)

    // Update the ack map with the result
    AME_Ctx::AME_Ctx_Ack ack_status = AME_Ctx::OK;
    if (status != ERR_OK) {
        ack_status = AME_Ctx::FAILED;
    }
    err = ctx->ame_upd_ctx_req(offset, ack_status);
    fds_verify(err == ERR_OK);

    // Signal nginx that it can stream back whatever buffers
    // are ready
    // TODO(Andrew): Need to figure out how to return
    // error when a later get() fails since we may have
    // already returned the header...
    if (status == ERR_OK) {
        conn_go->ame_signal_resume(AME_Request::ame_map_fdsn_status(ERR_OK));
    } else {
        conn_go->ame_signal_resume(
            AME_Request::ame_map_fdsn_status(status));
    }

    return ERR_OK;
}

// ame_request_resume
// ------------------
//
int
Conn_GetObject::ame_request_resume()
{
    fds_uint32_t len;
    ame_buf_t    *buf;
    int result = NGX_AGAIN;

    fds_stat_record(STAT_NGX,
                    STAT_NGX_GET_RESUME, ame_clk_fdsn_cb, fds::util::getClockTicks());

    // Lock the context during the resume since we want
    // all send/state updates to be atomic
    ame_ctx->ame_ctx_lock();

    if (ame_resp_status == NGX_HTTP_OK) {
        // Check if the header has been sent already or not
        if (ame_ctx->ame_get_header_sent_locked() == false) {
            // TODO(Andrew): Need to get etag out of blob rather
            // than compute it here
            ame_etag = ame_ctx->ame_get_etag_result_locked();
            // When streaming, the requested length is either
            // the whole blob size, if no size was specified in
            // the request or a smaller size specified during the
            // initial request
            ame_set_std_resp(ame_resp_status,
                             ame_ctx->ame_get_requested_len_locked());
            ame_send_response_hdr();
            ame_ctx->ame_set_header_sent_locked(true);
        }

        // Send back any ready in order buffers
        // FIX the fact that we don't stream and
        // send all of the buffers back at once
        fds_bool_t last;
        fds_off_t  offset;
        Error err = ame_ctx->ame_get_unsent_offset_locked(&offset, &last);
        while (err != ERR_NOT_FOUND) {
            // Get the ame buf from the get map
            ame_ctx->ame_get_get_info_locked(offset, &buf, &len);

            // Note that we expect nginx to free the buffer
            // so we don't have to free it...simply clear the map
            ame_send_resp_data(buf, len, last);
            // TODO(Andrew): Should we check if the buffer was released?

            // Mark the offset as sent
            err = ame_ctx->ame_upd_ctx_req_locked(offset, AME_Ctx::SENT);

            // Get the next unsent offset unless we already sent
            // the last buffer
            if (last == true) {
                break;
            }
            err = ame_ctx->ame_get_unsent_offset_locked(&offset, &last);
        }
        if (last == true) {
            // If we just sent the last offset
            // then we're done
            result = NGX_DONE;
        }
    } else {
        // Just send back the err response
        AME_Ctx::AME_Ctx_Ack ack_status =
                ame_ctx->ame_check_status_locked();
        if ((ack_status == AME_Ctx::OK) ||
            (ack_status == AME_Ctx::FAILED)) {
            // If there aren't any more waiting requests
            // then just return with this error code.
            // Any waiting requests will just send o back
            // whatever data we have.
            // TODO(Andrew): We can actually just truncate
            // the request here and clean up the waiting requests
            ame_finalize_response(ame_resp_status);
            result = NGX_DONE;
            // TODO(Andrew): Since we didn't call send_resp_data()
            // for the failed bufs, they're still allocated. We
            // should iterate any failed offset in the ctx and free em.
        }
    }

    // Unlock the context
    ame_ctx->ame_ctx_unlock();
    return result;
}

// ame_request_handler
// -------------------
//
void
Conn_GetObject::ame_request_handler()
{
    fds_uint32_t  buf_req_len = get_max_buf_len();
    int           len;
    char          *adr;
    ame_buf_t     *buf;
    FDS_NativeAPI *api;
    BucketContextPtr bucket_ctx(new BucketContext(
        "host", get_bucket_id(), "accessid", "secretkey"));

    // This connector always reads the whole object, so set
    // the request end to be all of the blob. We don't know
    // the blob size until our first callback
    ame_ctx->ame_set_request_all();

    // Get an AME buffer from nginx to read into
    buf = ame_ctx->ame_alloc_buf(buf_req_len, &adr, &len);
    fds_verify(buf != NULL);
    fds_verify((fds_uint32_t)len == buf_req_len);

    // Get the current offset in the stream
    fds_off_t ame_cur_offset = ame_ctx->ame_get_off();

    // Store a link to the buffer
    // Note, this assumes the buffers are linked
    // in offset order...we will return data to nginx
    // in this order
    ame_ctx->ame_add_get_buf(ame_cur_offset, buf);

    // Update the context with the waiting request
    Error err = ame_ctx->ame_add_ctx_req(ame_cur_offset);
    fds_verify(err == ERR_OK);

    // Move the offset forward for the (possible) next read
    ame_ctx->ame_mv_off(buf_req_len);

    // Hard coded object name for getting stats
    // TODO(Andrew): Don't hard code the stats name
    // here...need consistent REST format
    if (get_bucket_id() == "stat") {
        len = gl_fds_stat.stat_out(STAT_MAX_MODS, adr, len);
        ame_ctx->ame_update_output_buf(len);
        ame_signal_resume(200);
        return;
    }
    ame_clk_fdsn = fds::util::getClockTicks();
    fds_stat_record(STAT_NGX, STAT_NGX_GET_FDSN, ame_clk_all, ame_clk_fdsn);

    api = ame->ame_fds_hook();
    // Issue the get() request with the desired read length set to the
    // current buffer length since we don't know how much data to read
    // api->GetObject(bucket_ctx, get_object_id(), NULL, ame_cur_offset,
    //            len, adr, len,
    //             static_cast<void *>(ame_ctx), fdsn_getobj_cbfn,
    //             static_cast<void *>(this));

    fds_stat_record(STAT_NGX,
                    STAT_NGX_GET_FDSN_RET, ame_clk_fdsn, fds::util::getClockTicks());
}

// ---------------------------------------------------------------------------
// PutObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_PutObject::Conn_PutObject(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
    ame_finalize = true;
    ame_stat_pt  = STAT_NGX_PUT;

    // TODO(Andrew): Don't hard code
    put_obj_max_buf_len = 2 * 1024 * 1024;
}

Conn_PutObject::~Conn_PutObject()
{
}

// put_callback_fn
// ---------------
//
static int
fdsn_putobj_cbfn(void *reqContext, fds_uint64_t bufferSize, fds_off_t offset,
                 char *buffer, void *callbackData, FDSN_Status status,
                 ErrorDetails* errDetails)
{
    AME_Ctx        *ctx = static_cast<AME_Ctx *>(reqContext);
    Conn_PutObject *conn_po = static_cast<Conn_PutObject *>(callbackData);

    LOGNORMAL << "Received FDSN put() callback for blob " << conn_po->get_object_id()
              << " with data at offset " << offset << " of length " << bufferSize
              << " and result " << status;

    // Update the status of this single request
    AME_Ctx::AME_Ctx_Ack ack_status = AME_Ctx::OK;
    if (status != ERR_OK) {
        ack_status = AME_Ctx::FAILED;
    }
    Error err = ctx->ame_upd_ctx_req(offset, ack_status);
    fds_verify(err == ERR_OK);

    // Check the status of the entire put request
    ack_status = ctx->ame_check_status();
    if (ack_status == AME_Ctx::OK) {
        conn_po->ame_signal_resume(AME_Request::ame_map_fdsn_status(ERR_OK));
    } else if (ack_status == AME_Ctx::FAILED) {
        conn_po->ame_signal_resume(
            AME_Request::ame_map_fdsn_status(status));
    } else {
        fds_verify(ack_status == AME_Ctx::WAITING);
    }

    return 0;
}

// ame_request_resume
// ------------------
// We're here after the callback function wakes up the event loop to resume
// the putobject method.
//
int
Conn_PutObject::ame_request_resume()
{
    fds_uint32_t len;
    char *buf;

    /* Compute etag to be sent as response.  Ideally this is done by AM */
    buf = ame_ctx->ame_curr_input_buf(&len);

    // Set etag value from ctx
    ame_etag = "\"" + ame_etag + "\"";

    // Release any buffers we allocated from nginx. Often
    // these buffers were allocated to collect data from
    // many nginx chain buffers into a single ame put buffer.
    ame_ctx->ame_free_bufs();

    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

fds_uint32_t
Conn_PutObject::get_max_buf_len() const {
    return put_obj_max_buf_len;
}

// ame_request_handler
// -------------------
//
void
Conn_PutObject::ame_request_handler()
{
    fds_uint32_t  len;
    char          *buf;
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");
    fds_bool_t    last_byte;
    PutPropertiesPtr putProps;

    if (-1 == ame_req->headers_in.content_length_n) {
        LOGWARN << "missing content-length header";
        ame_signal_resume(AME_Request::ame_map_fdsn_status(FDSN_StatusErrorMissingContentLength)); //NOLINT
        return;
    }

    buf = ame_reqt_iter_data_next(get_max_buf_len(), &len);
    LOGDEBUG << "request data size :" << len;

    // Create a fake transaction for now
    BlobTxId::ptr txDesc(new BlobTxId());

    if (len == 0) {
        LOGWARN <<"zero size request received";
        // special case where no data is given
        // Issue async request
        fds_off_t offset= ame_ctx->ame_get_offset();
        Error err = ame_ctx->ame_add_ctx_req(offset);
        fds_verify(err == ERR_OK);
        api = ame->ame_fds_hook();
        api->PutBlob(&bucket_ctx, get_object_id(), putProps,
                     static_cast<void *>(ame_ctx), buf, 0,
                     len, txDesc, true, fdsn_putobj_cbfn,
                     static_cast<void *>(this));
    }

    while (len != 0) {
        // Update the context with the outstanding request
        fds_off_t offset= ame_ctx->ame_get_offset();
        Error err = ame_ctx->ame_add_ctx_req(offset);
        fds_verify(err == ERR_OK);

        // Updates the stream offset by len bytes
        ame_ctx->ame_mv_off(len);

        // Update the rolling etag calculation
        // TODO(Andrew): We're calculating the etag from the nginx request
        // structure, since that has all of the data. We should actually have
        // AM calcuate this based on what it wrote out over the network so
        // that the etag tracks what we wrote, not just what was received
        HttpUtils::updateEtag(ame_ctx->ame_get_etag(), buf, len);

        // Check if this update is the last buffer in
        // the request
        last_byte = false;
        if ((offset + len) == (fds_off_t)ame_req->headers_in.content_length_n) {
            last_byte = true;
            ame_etag = HttpUtils::finalEtag(ame_ctx->ame_get_etag());
            putProps.reset(new PutProperties());
            putProps->md5 = ame_etag;
            LOGDEBUG << "Setting offset " << offset << " and length " << len
                     << " as the last update to the blob with etag "
                     << ame_etag;
        }

        // Issue async request
        api = ame->ame_fds_hook();
        api->PutBlob(&bucket_ctx, get_object_id(), putProps,
                     static_cast<void *>(ame_ctx), buf, offset,
                     len, txDesc, last_byte, fdsn_putobj_cbfn,
                     static_cast<void *>(this));

        // Get the next data and data len
        buf = ame_reqt_iter_data_next(get_max_buf_len(), &len);
    }
    // We break when our last buffer wasn't the full requested
    // amount, meaning it was the tail
}

// ---------------------------------------------------------------------------
// DelObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_DelObject::Conn_DelObject(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
    ame_stat_pt = STAT_NGX_DEL;
}

Conn_DelObject::~Conn_DelObject()
{
}

// fdsn_delobj_cbfn
// ----------------
//
static void
fdsn_delobj_cbfn(FDSN_Status status, const ErrorDetails *err, void *arg)
{
    Conn_DelObject *conn_do = static_cast<Conn_DelObject *>(arg);
    conn_do->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
}

// ame_request_resume
// ------------------
//
int
Conn_DelObject::ame_request_resume()
{
    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_DelObject::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame->ame_fds_hook();
    api->DeleteObject(&bucket_ctx, get_object_id(),
                      NULL, fdsn_delobj_cbfn, this);
}

// ---------------------------------------------------------------------------
// PutBucket Connector Adapter
// ---------------------------------------------------------------------------
Conn_PutBucket::Conn_PutBucket(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
    ame_finalize = true;
    ame_stat_pt  = STAT_NGX_PUT_BK;
}

Conn_PutBucket::~Conn_PutBucket()
{
}

// fdsn_putbucket_cbfn
// -------------------
//
static void
fdsn_putbucket_cbfn(FDSN_Status status, const ErrorDetails *err, void *arg)
{
    Conn_PutBucket *conn_pb = static_cast<Conn_PutBucket *>(arg);
    conn_pb->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
}

// ame_request_resume
// ------------------
//
int
Conn_PutBucket::ame_request_resume()
{
    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_PutBucket::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame->ame_fds_hook();
    api->CreateBucket(&bucket_ctx, CannedAclPrivate,
                      NULL, fdsn_putbucket_cbfn, this);
}

// ---------------------------------------------------------------------------
// GetBucket Connector Adapter
// ---------------------------------------------------------------------------
Conn_GetBucket::Conn_GetBucket(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
    ame_stat_pt = STAT_NGX_GET_BK;
}

Conn_GetBucket::~Conn_GetBucket()
{
}

// fdsn_getbucket_cbfn
// -------------------
// Callback from FDSN to notify us that they have data to send out.
//
static void
fdsn_getbucket_cbfn(int isTruncated, const char *nextMarker, int contentCount,
                    const ListBucketContents *contents, int commPrefixCnt,
                    const char **commPrefixes, void *cbarg, FDSN_Status status)
{
    Conn_GetBucket *gbucket = static_cast<Conn_GetBucket *>(cbarg);

    if (status != ERR_OK) {
        gbucket->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
        return;
    }
    gbucket->ame_fmt_resp_data(isTruncated, nextMarker, contentCount,
                               contents, commPrefixCnt, commPrefixes);

    gbucket->ame_signal_resume(NGX_HTTP_OK);
}

// ame_fmt_resp_data
// -----------------
// Format response data with result from listing objects in the bucket.
//
void
Conn_GetBucket::ame_fmt_resp_data(int is_truncated, const char *next_marker,
                                  int content_cnt, const ListBucketContents *contents,
                                  int comm_prefix_cnt, const char **comm_prefix)
{
    int        i, got, used, sent;
    char      *cur;
    ame_buf_t *buf;

    // assuming 200 + (2*keysize) is 400
    // for 2600 objects, the buffer size is 1MB

    // this is for about 2600 keys
    int bufsize = NGX_RESP_CHUNK_SIZE*64;

    // we need to support max-keys for this or else we will blow out soon !!!
    if (content_cnt/2600 > 0) {
        bufsize  += (bufsize * (content_cnt/2600));
    }

    buf = ame_ctx->ame_alloc_buf(bufsize, &cur, &got);
    ame_ctx->ame_push_output_buf(buf);

    // Format the header to send out.
    sent = 0;
    used = snprintf(cur, got,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<%s>\n"
                    "  <%s></%s>\n"
                    "  <%s>%s</%s>\n"
                    "  <%s>%s</%s>\n"
                    "  <%s>%d</%s>\n"
                    "  <%s>%s</%s>\n",
                    sgt_AMEKey[REST_LIST_BUCKET].u.kv_key,
                    sgt_AMEKey[REST_NAME].u.kv_key, sgt_AMEKey[REST_NAME].u.kv_key,

                    sgt_AMEKey[REST_PREFIX].u.kv_key,
                    comm_prefix == nullptr ? "" : comm_prefix[0],
                    sgt_AMEKey[REST_PREFIX].u.kv_key,

                    sgt_AMEKey[REST_MARKER].u.kv_key,
                    next_marker == nullptr ? "" : next_marker,
                    sgt_AMEKey[REST_MARKER].u.kv_key,

                    sgt_AMEKey[REST_MAX_KEYS].u.kv_key, content_cnt,
                    sgt_AMEKey[REST_MAX_KEYS].u.kv_key,

                    sgt_AMEKey[REST_IS_TRUNCATED].u.kv_key,
                    is_truncated ? "true" : "false",
                    sgt_AMEKey[REST_IS_TRUNCATED].u.kv_key);

    // We shouldn't run out of room here.
    fds_verify(used < got);
    sent += used;
    cur  += used;
    got  -= used;

    for (i = 0; i < content_cnt; i++) {
        used = snprintf(cur, got,
                        "  <%s>\n"
                        "    <%s>%s</%s>\n"
                        "    <%s>%s</%s>\n"
                        "    <%s>%llu</%s>\n"
                        "    <%s>%s</%s>\n"
                        "    <%s>\n"
                        "      <%s>%s</%s>\n"
                        "      <%s>%s</%s>\n"
                        "    </%s>\n"
                        "  </%s>\n",
                        sgt_AMEKey[REST_CONTENTS].u.kv_key,

                        sgt_AMEKey[REST_KEY].u.kv_key,
                        contents[i].objKey.c_str(),
                        sgt_AMEKey[REST_KEY].u.kv_key,

                        sgt_AMEKey[REST_ETAG].u.kv_key,
                        contents[i].eTag.c_str(),
                        sgt_AMEKey[REST_ETAG].u.kv_key,

                        sgt_AMEKey[REST_SIZE].u.kv_key,
                        contents[i].size,
                        sgt_AMEKey[REST_SIZE].u.kv_key,

                        sgt_AMEKey[REST_STORAGE_CLASS].u.kv_key,
                        "STANDARD",
                        sgt_AMEKey[REST_STORAGE_CLASS].u.kv_key,

                        sgt_AMEKey[REST_OWNER].u.kv_key,

                        sgt_AMEKey[REST_ID].u.kv_key,
                        contents[i].ownerId.c_str(),
                        sgt_AMEKey[REST_ID].u.kv_key,

                        sgt_AMEKey[REST_DISPLAY_NAME].u.kv_key,
                        contents[i].ownerDisplayName.c_str(),
                        sgt_AMEKey[REST_DISPLAY_NAME].u.kv_key,

                        sgt_AMEKey[REST_OWNER].u.kv_key,

                        sgt_AMEKey[REST_CONTENTS].u.kv_key);

        if (used == got) {
            // XXX: not yet handle!
            fds_assert(!"Increase bigger buffer size!");
        }
        sent += used;
        cur  += used;
        got  -= used;
    }
    used = snprintf(cur, got, "</%s>\n",
                    sgt_AMEKey[REST_LIST_BUCKET].u.kv_key);
    sent += used;
    buf->last = buf->pos + sent;
}

// ame_request_resume
// ------------------
//
int
Conn_GetBucket::ame_request_resume()
{
    fds_uint32_t len;
    char      *adr;
    ame_buf_t *buf;

    adr = ame_ctx->ame_curr_output_buf(&buf, &len);
    if (ame_resp_status == NGX_HTTP_OK) {
        ame_etag = "\"" + HttpUtils::computeEtag(adr, len) + "\"";
        ame_set_std_resp(ame_resp_status, len);
        ame_send_response_hdr();
        ame_send_resp_data(buf, len, true);
    } else {
        ame_finalize_response(ame_resp_status);
    }
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_GetBucket::ame_request_handler()
{
    FDS_NativeAPI *api;
    std::string    prefix, marker, deli;
    BucketContext  bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame->ame_fds_hook();
    api->GetBucket(&bucket_ctx, prefix, marker, deli, 1000, NULL,
                   fdsn_getbucket_cbfn, static_cast<void *>(this));
}

// ---------------------------------------------------------------------------
// PutBucketParams Connector Adapter
// ---------------------------------------------------------------------------
Conn_PutBucketParams::Conn_PutBucketParams(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req), validParams(true)
{
    ame_finalize = true;
}

Conn_PutBucketParams::~Conn_PutBucketParams()
{
}

// fdsn_putbucket_param_cb
// -----------------------
// Handle cb from FDSN comming from another thread.
//
static void
fdsn_putbucket_param_cb(FDSN_Status status,
                        const ErrorDetails *errorDetails,
                        void *callbackData)
{
    Conn_PutBucketParams *conn_pbp;

    conn_pbp = static_cast<Conn_PutBucketParams *>(callbackData);
    conn_pbp->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
}

// ame_request_resume
// ------------------
//
int
Conn_PutBucketParams::ame_request_resume()
{
    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_PutBucketParams::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    fds_uint32_t len;
    char *buf;

    buf = ame_reqt_iter_data(&len);
    if (buf == NULL || len == 0) {
        validParams = false;
    }

    Json::Value  root;
    Json::Reader reader;
    std::string  strBuf(buf, len);
    fds_bool_t   parseOk = reader.parse(strBuf, root);
    if (parseOk == false) {
        validParams = false;
    } else {
        relative_prio = root.get("priority", 0).asUInt();
        if (relative_prio == 0) {
            validParams = false;
        }
        iops_min = root.get("sla", -1).asDouble();
        if (iops_min == -1) {
            validParams = false;
        }
        iops_max = root.get("limit", -1).asDouble();
        if (iops_max == -1) {
            validParams = false;
        }
    }

    if (validParams == false) {
        ame_finalize_response(NGX_HTTP_INTERNAL_SERVER_ERROR);
    } else {
        QosParams qos_params(iops_min, iops_max, relative_prio);

        api = ame->ame_fds_hook();
        api->ModifyBucket(&bucket_ctx, qos_params, NULL,
                          fdsn_putbucket_param_cb, static_cast<void *>(this));
    }
}

int
Conn_PutBucketParams::ame_format_response_hdr()
{
    return NGX_OK;
}

// ---------------------------------------------------------------------------
// DeleteBucket Connector Adapter
// ---------------------------------------------------------------------------
Conn_DelBucket::Conn_DelBucket(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
    ame_stat_pt = STAT_NGX_DEL_BK;
}

Conn_DelBucket::~Conn_DelBucket()
{
}

// fdsn_delbucket_cbfn
// -------------------
//
static void
fdsn_delbucket_cbfn(FDSN_Status status, const ErrorDetails *err, void *arg)
{
    LOGDEBUG << "status:" << status << ":" << static_cast<int>(status);
    Conn_DelBucket *conn_pb = static_cast<Conn_DelBucket *>(arg);
    conn_pb->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
}

// ame_request_resume
// ------------------
//
int
Conn_DelBucket::ame_request_resume()
{
    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_DelBucket::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame->ame_fds_hook();
    api->DeleteBucket(&bucket_ctx, NULL, fdsn_delbucket_cbfn, this);
}


// ---------------------------------------------------------------------------
// GetBucketStats Connector Adapter
// ---------------------------------------------------------------------------
Conn_GetBucketStats::Conn_GetBucketStats(AMEngine *eng, AME_HttpReq *req)
        : AME_Request(eng, req)
{
}

Conn_GetBucketStats::~Conn_GetBucketStats()
{
}

// fdsn_getbucket_stat_cb
// ----------------------
// Callback from FDSN to notify us that they have data to send out.
//
static void
fdsn_getbucket_stat_cb(const std::string& timestamp,
                       int content_count,
                       const BucketStatsContent* contents,
                       void *req_context,
                       void *callback_data,
                       FDSN_Status status,
                       ErrorDetails *err_details)
{
    Conn_GetBucketStats *gbstats;

    gbstats = static_cast<Conn_GetBucketStats *>(callback_data);
    if (status != ERR_OK) {
        gbstats->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
        return;
    }
    gbstats->ame_fmt_resp_data(timestamp, content_count, contents);
    gbstats->ame_signal_resume(NGX_HTTP_OK);
}

// ame_fmt_resp_data
// -----------------
// Format response stat data before sending back to the client.
//
void
Conn_GetBucketStats::ame_fmt_resp_data(const std::string &timestamp,
                                       int content_count,
                                       const BucketStatsContent *contents)
{
    int         i, got, used, sent;
    char       *cur;
    ame_buf_t  *buf;

    buf = ame_ctx->ame_alloc_buf(NGX_RESP_CHUNK_SIZE, &cur, &got);
    ame_ctx->ame_push_output_buf(buf);

    /* we are sending stats in json format */
    // Format the header to send out.
    sent = 0;
    used = snprintf(cur, got,
                    "{\n"
                    "\"%s\": \"%s\", \n"
                    "\"%s\":\n"
                    "  [\n",
                    sgt_AMEKey[RESP_STATS_TIME].u.kv_key,
                    timestamp.c_str(),
                    sgt_AMEKey[RESP_STATS_VOLS].u.kv_key);

    // We shouldn't run out of room here.
    fds_verify(used < got);
    sent += used;
    cur  += used;
    got  -= used;

    for (i = 0; i < content_count; i++) {
        used = snprintf(cur, got,
                        "    {\"%s\": \"%s\", \"%s\": %d, \"%s\": %d, \"%s\": %d, "
                        "\"%s\": %d}",
                        sgt_AMEKey[RESP_STATS_ID].u.kv_key,
                        (contents[i].bucket_name).c_str(),

                        sgt_AMEKey[RESP_QOS_PRIORITY].u.kv_key,
                        contents[i].priority,

                        sgt_AMEKey[RESP_QOS_PERFORMANCE].u.kv_key,
                        static_cast<int>(contents[i].performance),

                        sgt_AMEKey[RESP_QOS_SLA].u.kv_key,
                        static_cast<int>(contents[i].sla),

                        sgt_AMEKey[RESP_QOS_LIMIT].u.kv_key,
                        static_cast<int>(contents[i].limit));

        if (used == got) {
            // XXX: not yet handle!
            fds_assert(!"Increase bigger buffer size!");
        }
        sent += used;
        cur  += used;
        got  -= used;

        if (i < (content_count-1)) {
            used = snprintf(cur, got, ", \n");
        } else {
            used = snprintf(cur, got, "\n");
        }
        if (used == got) {
            // XXX: not yet handle!
            fds_assert(!"Increase bigger buffer size!");
        }
        sent += used;
        cur  += used;
        got  -= used;
    }
    used = snprintf(cur, got, "  ]\n}\n");
    sent += used;
    buf->last = buf->pos + sent;
}

// ame_request_resume
// ------------------
//
int
Conn_GetBucketStats::ame_request_resume()
{
    fds_uint32_t len;
    char      *adr;
    ame_buf_t *buf;

    adr = ame_ctx->ame_curr_output_buf(&buf, &len);
    if (ame_resp_status == NGX_HTTP_OK) {
        ame_etag = "\"" + HttpUtils::computeEtag(adr, len) + "\"";
        ame_set_std_resp(ame_resp_status, len);
        ame_send_response_hdr();
        ame_send_resp_data(buf, len, true);
    } else {
        ame_finalize_response(ame_resp_status);
    }
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_GetBucketStats::ame_request_handler()
{
    FDS_NativeAPI *api;

    api = ame->ame_fds_hook();
    api->GetVolumeStats(NULL, fdsn_getbucket_stat_cb,
                        static_cast<void *>(this));
}

int
Conn_GetBucketStats::ame_format_response_hdr()
{
    return NGX_OK;
}
}   // namespace fds
