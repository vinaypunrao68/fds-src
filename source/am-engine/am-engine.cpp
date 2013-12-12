/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/am-engine.h>
#include <am-engine/http_utils.h>
#include <am-plugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
} // extern "C"

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <shared/fds_types.h>
#include <fds_assert.h>
#include <native_api.h>
#include <am-plugin.h>

namespace fds {

const int    NGINX_ARG_PARAM = 80;
const int    NGX_RESP_CHUNK_SIZE = (16 << 10);
static char  nginx_prefix[NGINX_ARG_PARAM];
static char  nginx_config[NGINX_ARG_PARAM];
static char  nginx_signal[NGINX_ARG_PARAM];

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

int
AMEngine::mod_init(SysParams const *const p)
{
    using namespace std;
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
        cout << desc << endl;
        return 1;
    }
    if (vm.count("signal")) {
        if ((eng_signal != "stop") && (eng_signal != "quit") &&
            (eng_signal != "reopen") && (eng_signal != "reload")) {
            cout << "Expect --signal stop|quit|reopen|reload" << endl;
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

void
AMEngine::mod_startup()
{
}

// make_nginix_dir
// ---------------
// Make a directory that nginx server requies.
//
static void
make_nginx_dir(char const *const path)
{
    if (mkdir(path, 0755) != 0) {
        if (errno == EACCES) {
            std::cout << "Don't have permission to " << path << std::endl;
            exit(1);
        }
        fds_verify(errno == EEXIST);
    }
}

// run_server
// ----------
//
void
AMEngine::run_server(FDS_NativeAPI *api)
{
    using namespace std;
    const string *fds_root;
    char          path[NGINX_ARG_PARAM];

    eng_api = api;
    fds_root = &mod_params->fds_root;
    if (eng_signal != "") {
        nginx_signal_argv[0] = mod_params->p_argv[0];
        strncpy(nginx_signal, eng_signal.c_str(), NGINX_ARG_PARAM);
        ngx_main(FDS_ARRAY_ELEM(nginx_signal_argv), nginx_signal_argv);
        return;
    }
    strncpy(nginx_config, eng_conf, NGINX_ARG_PARAM);
    snprintf(nginx_prefix, NGINX_ARG_PARAM, "%s", fds_root->c_str());

    // Create all directories if they are not exists.
    umask(0);
    snprintf(path, NGINX_ARG_PARAM, "%s/%s", nginx_prefix, eng_logs);
    ModuleVector::mod_mkdir(path);
    snprintf(path, NGINX_ARG_PARAM, "%s/%s", nginx_prefix, eng_etc);
    ModuleVector::mod_mkdir(path);

    ngx_register_plugin(this);
    nginx_start_argv[0] = mod_params->p_argv[0];
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
    { { "Connection" },          0, RESP_CONNECTION },
    { { "open" },                0, RESP_CONNECTION_OPEN },
    { { "close" },               0, RESP_CONNECTION_CLOSE },
    { { "Etag" },                0, RESP_ETAG },
    { { "Date" },                0, RESP_DATE },
    { { "Server" },              0, RESP_SERVER },

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

int AME_Request::ame_map_fdsn_status(FDSN_Status status)
{
  if (status == FDSN_StatusOK) {
    return NGX_HTTP_OK;
  } else {
    // todo: do better mapping.  Log the error
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
}

AME_Request::AME_Request(AMEngine *eng, AME_HttpReq *req)
    : fdsio::Request(true), ame(eng), ame_req(req),
      ame_http(req), ame_finalize(false)
{
    ame_resp_status = 500;
    eng->ame_get_queue()->rq_enqueue(this, 0);
}

AME_Request::~AME_Request()
{
    req_complete();
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

// ame_reqt_iter_data
// ------------------
//
char *
AME_Request::ame_reqt_iter_data(int *len)
{
    char *data, *curr;
    int   data_len, copy_len;

    if (ame_ctx->ame_curr_input_buf(len) == NULL) {
        return NULL;
    }
    // Temporary code so that we return a single buffer containing all
    // content in a body.
    *len = 0;
    copy_len = 0;
    data_len = ame_req->headers_in.content_length_n;

    data = (char *)ngx_palloc(ame_req->pool, data_len);
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
    h = (ngx_table_elt_t *)ngx_list_push(&r->headers_out.headers);

    h->key.len    = klen;
    h->key.data   = (u_char *)k;
    h->hash       = ngx_hash_key_lc(h->key.data, h->key.len);
    h->value.len  = vlen;
    h->value.data = (u_char *)v;

    return NGX_OK;
}

// ame_set_std_resp
// ----------------
// Common code path to set standard response to client.
//
int
AME_Request::ame_set_std_resp(int status, int len)
{
    int                used;
    char               *buf;
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
    if (last) {
        ngx_http_finalize_request(r, rc);
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
    // TODO: map status back to NGX_ rc error code.
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
}

Conn_GetObject::~Conn_GetObject()
{
}

// fdsn_getobj_cbfn
// ----------------
//
static FDSN_Status
fdsn_getobj_cbfn(void *req, fds_uint64_t bufsize,
                 const char *buf, void *cbData,
                 FDSN_Status status, ErrorDetails *errdetails)
{
    AME_Ctx        *ctx = (AME_Ctx *)req;
    Conn_GetObject *conn_go = (Conn_GetObject *)cbData;

    FDS_PLOG(conn_go->ame_get_log()) << "GetObject bucket: "
        << conn_go->get_bucket_id() << " , object: "
        << conn_go->get_object_id() << " , len: "
        << bufsize << ", status: " << status;

    ctx->ame_update_output_buf(bufsize);
    conn_go->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
    return FDSN_StatusOK;
}

// ame_request_resume
// ------------------
//
int
Conn_GetObject::ame_request_resume()
{
    int        len;
    char      *adr;
    ame_buf_t *buf;

    adr = ame_ctx->ame_curr_output_buf(&buf, &len);
    len = ame_ctx->ame_temp_len;

    FDS_PLOG(ame_get_log()) << "GetObject resume status " << ame_resp_status
	<< ", obj: " << get_object_id() << ", buf len " << len;

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
Conn_GetObject::ame_request_handler()
{
    static int buf_req_len = (6 << 20); // 6MB
    int           len;
    char          *adr;
    ame_buf_t     *buf;
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    buf = ame_ctx->ame_alloc_buf(buf_req_len, &adr, &len);
    ame_ctx->ame_push_output_buf(buf);

    api = ame->ame_fds_hook();
    api->GetObject(&bucket_ctx, get_object_id(), NULL, 0, len, adr, len,
        (void *)ame_ctx, fdsn_getobj_cbfn, (void *)this);
}

// ---------------------------------------------------------------------------
// PutObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_PutObject::Conn_PutObject(AMEngine *eng, AME_HttpReq *req)
    : AME_Request(eng, req)
{
    ame_finalize = true;
}

Conn_PutObject::~Conn_PutObject()
{
}

// put_callback_fn
// ---------------
//
static int
fdsn_putobj_cbfn(void *reqContext, fds_uint64_t bufferSize, char *buffer,
    void *callbackData, FDSN_Status status, ErrorDetails* errDetails)
{
    AME_Ctx        *ctx = (AME_Ctx *)reqContext;
    Conn_PutObject *conn_po = (Conn_PutObject*)callbackData;

    ctx->ame_update_input_buf(bufferSize);
    conn_po->ame_signal_resume(AME_Request::ame_map_fdsn_status(status));
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
    int   len;
    char *buf;

    /* Compute etag to be sent as response.  Ideally this is done by AM */
    buf = ame_ctx->ame_curr_input_buf(&len);

    /* XXX: we're working on the big buffer, not this one. */
    // ame_etag = "\"" + HttpUtils::computeEtag(buf, len) + "\"";
    // Temp. code because we put our data to this buffer.
    //
    len = ame_ctx->ame_temp_len;
    ame_etag = "\"" + HttpUtils::computeEtag(ame_ctx->ame_temp_buf, len) + "\"";
    FDS_PLOG(ame_get_log()) << "PutObject bucket: " << get_bucket_id()
        << " , object: " << get_object_id();

    // Temp. code.  We need to have a proper buffer chunking model here.
    if (ame_ctx->ame_temp_buf != NULL) {
        ngx_pfree(ame_req->pool, ame_ctx->ame_temp_buf);
    }
    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

// ame_request_handler
// -------------------
//
void
Conn_PutObject::ame_request_handler()
{
    int           len;
    char          *buf;
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    buf = ame_reqt_iter_data(&len);
    if (buf == NULL || len == 0) {
        ame_finalize_request(NGX_HTTP_BAD_REQUEST);
        return;
    }
    api = ame->ame_fds_hook();
    api->PutObject(&bucket_ctx, get_object_id(), NULL, (void *)ame_ctx,
                   buf, len, fdsn_putobj_cbfn, (void *)this);
}

// ---------------------------------------------------------------------------
// DelObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_DelObject::Conn_DelObject(AMEngine *eng, AME_HttpReq *req)
    : AME_Request(eng, req)
{
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
    Conn_DelObject *conn_do = (Conn_DelObject *)arg;
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
    Conn_PutBucket *conn_pb = (Conn_PutBucket *)arg;
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
    Conn_GetBucket *gbucket = (Conn_GetBucket *)cbarg;

    if (status != FDSN_StatusOK) {
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

    buf = ame_ctx->ame_alloc_buf(NGX_RESP_CHUNK_SIZE*4, &cur, &got);
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
    int        len;
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
                   fdsn_getbucket_cbfn, (void *)this);
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
    Conn_PutBucketParams *conn_pbp = (Conn_PutBucketParams *)callbackData;
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

    int   len;
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
        api->ModifyBucket(&bucket_ctx, qos_params,
                           NULL, fdsn_putbucket_param_cb, (void *)this);
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
    Conn_DelBucket *conn_pb = (Conn_DelBucket *)arg;
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
    Conn_GetBucketStats *gbstats = (Conn_GetBucketStats *)callback_data;

    if (status != FDSN_StatusOK) {
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
        int content_count, const BucketStatsContent *contents)
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
		    "\"%s\": \"%s\",\n"
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
			"    {\"%s\": \"%s\", \"%s\": %d, \"%s\": %ld, \"%s\": %ld, "
            "\"%s\": %ld}",
			sgt_AMEKey[RESP_STATS_ID].u.kv_key,
			(contents[i].bucket_name).c_str(),

			sgt_AMEKey[RESP_QOS_PRIORITY].u.kv_key,
			contents[i].priority,

			sgt_AMEKey[RESP_QOS_PERFORMANCE].u.kv_key,
			(long)contents[i].performance,

			sgt_AMEKey[RESP_QOS_SLA].u.kv_key,
			(long)contents[i].sla,

			sgt_AMEKey[RESP_QOS_LIMIT].u.kv_key,
			(long)contents[i].limit);

        if (used == got) {
            // XXX: not yet handle!
            fds_assert(!"Increase bigger buffer size!");
        }
        sent += used;
        cur  += used;
        got  -= used;

        if (i < (content_count-1)) {
            used = snprintf(cur, got, ",\n");
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
    int        len;
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
    api->GetBucketStats(NULL, fdsn_getbucket_stat_cb, (void *)this);
}

int
Conn_GetBucketStats::ame_format_response_hdr()
{
    return NGX_OK;
}
} // namespace fds
