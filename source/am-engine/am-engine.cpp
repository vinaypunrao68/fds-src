/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/am-engine.h>
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

AMEngine gl_AMEngine("AM Engine Module");

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

    nginx_start_argv[0] = mod_params->p_argv[0];
    ngx_main(FDS_ARRAY_ELEM(nginx_start_argv), nginx_start_argv);
}

void
AMEngine::mod_shutdown()
{
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
    { { nullptr },               0, AME_HDR_KEY_MAX }
};

int AME_Request::map_fdsn_status(FDSN_Status status)
{
  if (status == FDSN_StatusOK) {
    return NGX_HTTP_OK;
  } else {
    // todo: do better mapping.  Log the error
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
}

AME_Request::AME_Request(HttpRequest &req)
    : fdsio::Request(true), ame_req(req),
      queue(1, 1000)
{
  if (ame_req.getNginxReq()->request_body &&
      ame_req.getNginxReq()->request_body->bufs) {
    post_buf_itr = ame_req.getNginxReq()->request_body->bufs;
  } else {
    post_buf_itr = NULL;
  }
  resp_status = 500;
  resp_buf = NULL;
  resp_buf_len = 0;
  req_completed = false;
  queue.rq_enqueue(this, 0);
}

AME_Request::~AME_Request()
{
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
ame_ret_e
AME_Request::ame_reqt_iter_next()
{
  // todo: rao implement this to the post/put client buffer
    return AME_OK;
}

// ame_reqt_iter_data
// ------------------
//
char*
AME_Request::ame_reqt_iter_data(int *len)
{
  if (post_buf_itr == NULL) {
    *len = 0;
    return NULL;
  }

  char* data = (char*) post_buf_itr->buf->pos;
  *len = post_buf_itr->buf->last - post_buf_itr->buf->pos;
  post_buf_itr = post_buf_itr->next;

  return data;
}

// ame_get_reqt_hdr_val
// --------------------
//
char const *const
AME_Request::ame_get_reqt_hdr_val(char const *const key)
{
  // todo: rao implement this
    return NULL;
}

// ame_set_resp_keyval
// -------------------
// Assume key/value buffers remain valid until this object is destroyed.
//
ame_ret_e
AME_Request::ame_set_resp_keyval(char *k, ngx_int_t klen,
                                 char *v, ngx_int_t vlen)
{
    ngx_table_elt_t    *h;
    ngx_http_request_t *r;

    r = ame_req.getNginxReq();
    h = (ngx_table_elt_t *)ngx_list_push(&r->headers_out.headers);

    h->key.len    = klen;
    h->key.data   = (u_char *)k;
    h->value.len  = vlen;
    h->value.data = (u_char *)v;

    return AME_OK;
}

// ame_set_std_resp
// ----------------
// Common code path to set standard response to client.
//
ame_ret_e
AME_Request::ame_set_std_resp(int status, int len)
{
    int                used;
    char               *buf;
    ngx_http_request_t *r;

    r = ame_req.getNginxReq();
    r->headers_out.status           = status;
    r->headers_out.content_length_n = len;
    if (len == 0) {
        r->header_only = 1;
    }
    return AME_OK;
}

// ame_send_response_hdr
// ---------------------
// Common code path to send the complete response header to client.
//
ame_ret_e
AME_Request::ame_send_response_hdr()
{
    ngx_int_t          rc;
    ngx_http_request_t *r;
    std::string etag_key = "ETag";

    // Any protocol-connector specific response format.
    ame_format_response_hdr();

    // Do actual sending.
    r  = ame_req.getNginxReq();
    if (etag.size() > 0) {
      ame_set_resp_keyval(const_cast<char*>(etag_key.c_str()), etag_key.size(),
          const_cast<char*>(etag.c_str()), etag.size());
    }
    rc = ngx_http_send_header(r);
    return AME_OK;
}

// ame_push_resp_data_buf
// ----------------------
//
void *
AME_Request::ame_push_resp_data_buf(int ask, char **buf, int *got)
{
    ngx_buf_t          *b;
    ngx_http_request_t *r;

    r = ame_req.getNginxReq();
    b = (ngx_buf_t *)ngx_calloc_buf(r->pool);
    memset(b, sizeof(ngx_buf_t), 0);

    b->memory        = 1;
    b->last_buf      = 0;
    b->last_in_chain = 0;

    if (ask > 0) {
      b->start = (u_char *)ngx_palloc(r->pool, ask);
      b->end   = b->start + ask;
      b->pos   = b->start;
      b->last  = b->end;
    }
    *buf = (char *)b->pos;
    *got = ask;
    return (void *)b;
}

// ame_send_resp_data
// ------------------
//
ame_ret_e
AME_Request::ame_send_resp_data(void *buf_cookie, int len, fds_bool_t last)
{
    ngx_buf_t          *buf;
    ngx_int_t          rc;
    ngx_chain_t        out;
    ngx_http_request_t *r;

    r   = ame_req.getNginxReq();

    buf = (ngx_buf_t *)buf_cookie;
    buf->last = buf->pos + len;
    out.buf  = buf;
    out.next = NULL;

    if (last == true) {
        buf->last_buf      = 1;
        buf->last_in_chain = 1;
    }

    rc = ngx_http_output_filter(r, &out);
    return AME_OK;
}

// ---------------------------------------------------------------------------
// GetObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_GetObject::Conn_GetObject(HttpRequest &req)
    : AME_Request(req)
{
}

Conn_GetObject::~Conn_GetObject()
{
}

// get_callback_fn
// ---------------
//
FDSN_Status
Conn_GetObject::cb(void *req, fds_uint64_t bufsize,
                const char *buf, void *cbData,
                FDSN_Status status, ErrorDetails *errdetails)
{
  Conn_GetObject *conn_go = (Conn_GetObject*) cbData;
  conn_go->notify_request_completed(map_fdsn_status(status), buf, (int)bufsize);
  return FDSN_StatusOK;
}

// ame_request_handler
// -------------------
//
void
Conn_GetObject::ame_request_handler()
{
    static int buf_req_len = 3 * (1 << 20); // 3MB
    int buf_len;
    char          *buf;
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    cur_get_buffer  = fdsn_alloc_get_buffer(buf_req_len, &buf, &buf_len);

    api = ame_fds_hook();

    api->GetObject(&bucket_ctx, get_object_id(), NULL, 0, buf_len, buf, buf_len,
        (void *)this, Conn_GetObject::cb, NULL);

    if (!req_completed) {
      req_wait();
    }

    if (resp_status == NGX_HTTP_OK) {
      etag = HttpUtils::computeEtag(resp_buf, resp_buf_len);
      fdsn_send_get_response(resp_status, (int) resp_buf_len);
      fdsn_send_get_buffer(cur_get_buffer, (int) resp_buf_len, true);
    } else {
      fdsn_send_get_response(resp_status, 0);
    }

}

// fdsn_send_get_response
// ----------------------
//
void
Conn_GetObject::fdsn_send_get_response(int status, int get_len)
{
    ame_set_std_resp(status, get_len);
    ame_send_response_hdr();
}

// ---------------------------------------------------------------------------
// PutObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_PutObject::Conn_PutObject(HttpRequest &req)
    : AME_Request(req)
{
}

Conn_PutObject::~Conn_PutObject()
{
}

// put_callback_fn
// ---------------
//
int
Conn_PutObject::cb(void *reqContext, fds_uint64_t bufferSize, char *buffer,
    void *callbackData, FDSN_Status status, ErrorDetails* errDetails)
{
  Conn_PutObject *conn_po = (Conn_PutObject*) callbackData;
  conn_po->notify_request_completed(map_fdsn_status(status), NULL, 0);
  printf("put object cb invoked\n");
}

// ame_request_handler
// -------------------
//
void
Conn_PutObject::ame_request_handler()
{
    fds_uint64_t  len;
    char          *buf;
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");
    char *temp;

    buf = ame_reqt_iter_data((int*) &len);
    if (buf == NULL || len == 0) {
      // todo: instead of assert put a log.  Also think about what needes
      // to be returned to server
      fds_assert(!"no body");
      return ;
    }

    /* compute etag to be sent as response.  Ideally this is done by AM */
    etag = HttpUtils::computeEtag(buf, len);
    printf("len: %d, data: %.4s\n", len, buf);

    api = ame_fds_hook();
    api->PutObject(&bucket_ctx, get_object_id(), NULL, NULL,
                   buf, len, Conn_PutObject::cb, this);

    if (!req_completed) {
      printf("put object waiting\n");
      req_wait();
    }
    printf("put object wait done\n");
    fdsn_send_put_response(resp_status, 0);
}

// fdsn_send_put_response
// ----------------------
//
void
Conn_PutObject::fdsn_send_put_response(int status, int put_len)
{
    ame_set_std_resp(status, put_len);
    ame_send_response_hdr();
}

// ---------------------------------------------------------------------------
// DelObject Connector Adapter
// ---------------------------------------------------------------------------
Conn_DelObject::Conn_DelObject(HttpRequest &req)
    : AME_Request(req)
{
}

Conn_DelObject::~Conn_DelObject()
{
}

// delete callback fn
// ------------------
//
void Conn_DelObject::cb(FDSN_Status status,
    const ErrorDetails *errorDetails,
    void *callbackData)
{
  ((Conn_DelObject*) callbackData)->notify_request_completed(map_fdsn_status(status), NULL, 0);
}

// ame_request_handler
// -------------------
//
void
Conn_DelObject::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame_fds_hook();
    api->DeleteObject(&bucket_ctx, get_object_id(), NULL, Conn_DelObject::cb, this);

    if (!req_completed) {
      req_wait();
    }
    fdsn_send_del_response(resp_status, 0);
}

// fdsn_send_put_response
// ----------------------
//
void
Conn_DelObject::fdsn_send_del_response(int status, int len)
{
    ame_set_std_resp(status, len);
    ame_send_response_hdr();
}

// ---------------------------------------------------------------------------
// PutBucket Connector Adapter
// ---------------------------------------------------------------------------
Conn_PutBucket::Conn_PutBucket(HttpRequest &req)
    : AME_Request(req)
{
}

Conn_PutBucket::~Conn_PutBucket()
{
}

// put_callback_fn
// ---------------
//
void Conn_PutBucket::fdsn_cb(FDSN_Status status,
    const ErrorDetails *errorDetails,
    void *callbackData)
{
  Conn_PutBucket *conn_pb_obj =  (Conn_PutBucket*) callbackData;
  conn_pb_obj->notify_request_completed(map_fdsn_status(status), NULL, 0);
}

// ame_request_handler
// -------------------
//
void
Conn_PutBucket::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame_fds_hook();
    api->CreateBucket(&bucket_ctx, CannedAclPrivate,
                      NULL, fds::Conn_PutBucket::fdsn_cb, this);
    if (!req_completed) {
      req_wait();
    }
    fdsn_send_put_response(resp_status, 0);
}

// fdsn_send_put_response
// ----------------------
//
void
Conn_PutBucket::fdsn_send_put_response(int status, int put_len)
{
    ame_set_std_resp(status, put_len);
    ame_send_response_hdr();
}

// ---------------------------------------------------------------------------
// GetBucket Connector Adapter
// ---------------------------------------------------------------------------
Conn_GetBucket::Conn_GetBucket(HttpRequest &req)
    : AME_Request(req)
{
}

Conn_GetBucket::~Conn_GetBucket()
{
}

// fdsn_getbucket
// --------------
// Callback from FDSN to notify us that they have data to send out.
//
FDSN_Status
Conn_GetBucket::fdsn_getbucket(int isTruncated, const char *nextMarker,
        int contentCount, const ListBucketContents *contents,
        int commPrefixCnt, const char **commPrefixes, void *cbarg)
{
    int            i, got, used, sent;
    void           *resp;
    char           *cur;
    Conn_GetBucket *gbucket = (Conn_GetBucket *)cbarg;

    resp = gbucket->ame_push_resp_data_buf(NGX_RESP_CHUNK_SIZE, &cur, &got);

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
            commPrefixes == nullptr ? "" : commPrefixes[0],
            sgt_AMEKey[REST_PREFIX].u.kv_key,

            sgt_AMEKey[REST_MARKER].u.kv_key,
            nextMarker == nullptr ? "" : nextMarker,
            sgt_AMEKey[REST_MARKER].u.kv_key,

            sgt_AMEKey[REST_MAX_KEYS].u.kv_key, contentCount,
            sgt_AMEKey[REST_MAX_KEYS].u.kv_key,

            sgt_AMEKey[REST_IS_TRUNCATED].u.kv_key,
            isTruncated ? "true" : "false",
            sgt_AMEKey[REST_IS_TRUNCATED].u.kv_key);

    // We shouldn't run out of room here.
    fds_verify(used < got);
    sent += used;
    cur  += used;
    got  -= used;

    for (i = 0; i < contentCount; i++) {
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

    gbucket->fdsn_send_getbucket_response(200, sent);
    gbucket->ame_send_resp_data(resp, sent, true);
}

// ame_request_handler
// -------------------
//
void
Conn_GetBucket::ame_request_handler()
{
    FDS_NativeAPI *api;
    std::string    prefix, marker, deli;
    ListBucketContents content;

    content.objKey = "foo";
    content.lastModified = 10;
    content.eTag = "abc123";
    content.size = 100;
    content.ownerId = "vy";
    content.ownerDisplayName = "Vy Nguyen";

    /*
    api = ame_fds_hook();
    api->GetBucket(NULL, prefix, marker, deli, 1000, NULL,
                   fds::Conn_GetBucket::fdsn_getbucket, (void *)this);
    */
    fdsn_getbucket(false, "marker", 1, &content, 0, NULL, (void *)this);
}

// fdsn_send_getbucket_response
// ----------------------------
//
void
Conn_GetBucket::fdsn_send_getbucket_response(int status, int len)
{
    ame_set_std_resp(status, len);
    ame_send_response_hdr();
}

// ---------------------------------------------------------------------------
// PutBucket Connector Adapter
// ---------------------------------------------------------------------------
Conn_DelBucket::Conn_DelBucket(HttpRequest &req)
    : AME_Request(req)
{
}

Conn_DelBucket::~Conn_DelBucket()
{
}

// delete_callback_fn
// ---------------
//
void Conn_DelBucket::cb(FDSN_Status status,
    const ErrorDetails *errorDetails,
    void *callbackData)
{
  Conn_DelBucket *conn_pb_obj = (Conn_DelBucket*) callbackData;
  conn_pb_obj->notify_request_completed(map_fdsn_status(status), NULL, 0);
}

// ame_request_handler
// -------------------
//
void
Conn_DelBucket::ame_request_handler()
{
    FDS_NativeAPI *api;
    BucketContext bucket_ctx("host", get_bucket_id(), "accessid", "secretkey");

    api = ame_fds_hook();
    api->DeleteBucket(&bucket_ctx, NULL, fds::Conn_DelBucket::cb, this);

    if (!req_completed) {
      req_wait();
    }
    fdsn_send_delbucket_response(resp_status, 0);
}

// fdsn_send_put_response
// ----------------------
//
void
Conn_DelBucket::fdsn_send_delbucket_response(int status, int len)
{
    ame_set_std_resp(status, len);
    ame_send_response_hdr();
}

} // namespace fds
