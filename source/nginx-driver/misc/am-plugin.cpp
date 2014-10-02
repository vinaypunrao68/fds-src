/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-plugin.h>
#include <sys/eventfd.h>
#include <string>
#include <vector>
#include <atomic>
#include <boost/tokenizer.hpp>

#include <fds_assert.h>
#include <util/fds_stat.h>
#include <nginx-driver/s3connector.h>
#include <nginx-driver/http_utils.h>

/* The factory creating objects to handle supported commands. */
static fds::AMEngine  *sgt_ame_plugin[fds::FDS_NativeAPI::FDSN_CLIENT_TYPE_MAX];

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *
ngx_http_fds_data_connector(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char *
ngx_http_fds_data_connector_atmos(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *ngx_http_fds_data_create_loc_conf(ngx_conf_t *cf);

static char *
ngx_http_fds_data_merge_loc_conf(ngx_conf_t *cf, void *prev, void *conf);

static std::vector<std::string>
ngx_parse_uri_parts(ngx_http_request_t *r);

static void ngx_http_fds_read_body(ngx_http_request_t *r);
static void ngx_http_fds_read_body_atmos(ngx_http_request_t *r);
static ngx_int_t ngx_http_fds_data_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_fds_data_handler_atmos(ngx_http_request_t *r);
static ngx_int_t ngx_http_fds_route_request(ngx_http_request_t *r);

static ngx_command_t  ngx_http_fds_data_commands[] =
{
    {
        .name   = ngx_string("fds_connector"),
        .type   = NGX_HTTP_LOC_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_NOARGS,
        .set    = ngx_http_fds_data_connector,
        .conf   = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = 0,
        .post   = NULL
    },
    {
        .name   = ngx_string("fds_connector_atmos"),
        .type   = NGX_HTTP_LOC_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_NOARGS,
        .set    = ngx_http_fds_data_connector_atmos,
        .conf   = NGX_HTTP_LOC_CONF_OFFSET,
        .offset = 0,
        .post   = NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_fds_data_module_ctx =
{
    .preconfiguration   = NULL,
    .postconfiguration  = NULL,
    .create_main_conf   = NULL,
    .init_main_conf     = NULL,
    .create_srv_conf    = NULL,
    .merge_srv_conf     = NULL,
    .create_loc_conf    = ngx_http_fds_data_create_loc_conf,
    .merge_loc_conf     = ngx_http_fds_data_merge_loc_conf
};

ngx_module_t ngx_http_fds_data_module =
{
    NGX_MODULE_V1,
    .ctx                = &ngx_http_fds_data_module_ctx,
    .commands           = ngx_http_fds_data_commands,
    .type               = NGX_HTTP_MODULE,
    .init_master        = NULL,
    .init_module        = NULL,
    .init_process       = NULL,
    .init_thread        = NULL,
    .exit_thread        = NULL,
    .exit_process       = NULL,
    .exit_master        = NULL,
    NGX_MODULE_V1_PADDING
};

typedef struct ngx_http_fds_loc_conf ngx_http_fds_loc_conf_t;
struct ngx_http_fds_loc_conf
{
    ngx_flag_t               fds_enable;
    fds::AME_CtxList        *fds_context;
};

static ngx_http_fds_loc_conf_t *sgt_fds_data_cfg;

/*
 * ngx_register_plugin
 * -------------------
 * Register the plugin having factory methods to create objects that will
 * handle supported APIs.
 */
void
ngx_register_plugin(fds::FDS_NativeAPI::FDSN_ClientType clientType, fds::AMEngine *engine)
{
    fds_assert(clientType < FDS_ARRAY_ELEM(sgt_ame_plugin));
    sgt_ame_plugin[clientType] = engine;
}

/**
 * Parse uri into its individual parts and return those parts
 */
static std::vector<std::string>
ngx_parse_uri_parts(ngx_http_request_t *r) {
    std::vector<std::string> uri_parts;
    std::string uri((const char*) r->uri.data, r->uri.len);
    boost::char_separator<char> sep("/");
    boost::tokenizer<boost::char_separator<char>> tokens(uri, sep);
    for (const auto& t : tokens) {
        uri_parts.push_back(t);
    }
    return uri_parts;
}

/*
 * ngx_http_fds_read_body
 * ----------------------
 */
static void
ngx_http_fds_read_body(ngx_http_request_t *r)
{
    fds::AMEngine            *ame_plugin;
    fds::AME_Ctx             *am_ctx;
    fds::AME_Request         *am_req;
    fds::HttpRequest          http_req(r);
    std::vector<std::string>  uri_parts = http_req.getURIParts();
    ngx_http_fds_loc_conf_t  *fdcf;
    std::string               value;
    // fds_bool_t                keyExists;

    ame_plugin = sgt_ame_plugin[fds::FDS_NativeAPI::FDSN_AWS_S3];
    fds_assert(ame_plugin);
    ame_http_parse_url(ame_plugin, r);
    am_req = NULL;
    switch (r->method) {
    case NGX_HTTP_GET:
        if (uri_parts.size() == 1) {
            am_req = ame_plugin->ame_getbucket_hdler(r);
        } else if (uri_parts.size() == 2) {
            am_req = ame_plugin->ame_getobj_hdler(r);
        } else if (uri_parts.size() == 0) {
          /*
           * We're assuming any host request with no URI
           * parts is a getStats request.
           */
          am_req = ame_plugin->ame_getbucketstats_hdler(r);
        }
        break;

    case NGX_HTTP_POST:
    case NGX_HTTP_PUT:
        if (uri_parts.size() == 1) {
          am_req = ame_plugin->ame_putbucket_hdler(r);
        } else if (uri_parts.size() == 2) {
          if (uri_parts[1] == "modPolicy") {
            am_req = ame_plugin->ame_putbucketparams_hdler(r);
          } else {
            am_req = ame_plugin->ame_putobj_hdler(r);
          }
        }
        break;

    case NGX_HTTP_DELETE:
        if (uri_parts.size() == 1) {
            am_req = ame_plugin->ame_delbucket_hdler(r);
        } else if (uri_parts.size() == 2) {
            am_req = ame_plugin->ame_delobj_hdler(r);
        }
        break;
    }
    if (am_req == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }
    fdcf = reinterpret_cast<ngx_http_fds_loc_conf_t *>
        (ngx_http_get_module_loc_conf(r, ngx_http_fds_data_module));

    am_ctx = fdcf->fds_context->ame_get_ctx(am_req);
    am_ctx->ame_register_ctx();
    am_req->ame_add_context(am_ctx);
    am_req->ame_request_handler();
}

/*
 * FIXME:
 * This solution is temporary.  We will need change when SM/DM support returning
 * object ID on create.
 */
std::atomic_int atmos_obj_id;

/*
 * ngx_http_fds_read_body_atmos
 * ----------------------------
 */
static void
ngx_http_fds_read_body_atmos(ngx_http_request_t *r)
{
    fds::AMEngine            *ame_plugin;
    fds::AME_Ctx             *am_ctx;
    fds::AME_Request         *am_req;
    fds::HttpRequest          http_req(r);
    std::vector<std::string>  uri_parts = http_req.getURIParts();
    std::string               obj_id;
    char                      buf[45];
    ngx_http_fds_loc_conf_t  *fdcf;
    std::string               value;
    fds_bool_t                keyExists;

    ame_plugin = sgt_ame_plugin[fds::FDS_NativeAPI::FDSN_EMC_ATMOS];
    fds_assert(ame_plugin);
    ame_http_parse_url(ame_plugin, r);
    am_req = NULL;
    switch (r->method) {
    case NGX_HTTP_GET:
        if (uri_parts.size() == 3) {
            // read object, obj_id is 3rd argument
            am_req = ame_plugin->ame_getobj_hdler(r);
        }
        break;

    case NGX_HTTP_POST:
    case NGX_HTTP_PUT:
        if (uri_parts.size() == 2) {
            // new object, object data in body, expect obj_id in response
            snprintf(buf, FDS_ARRAY_ELEM(buf), "%044d", atmos_obj_id++);
            obj_id = buf;

            // http_req.appendURIPart(obj_id);
            am_req = ame_plugin->ame_putobj_hdler(r);
            am_req->appendURIPart(obj_id);
        } else if (uri_parts.size() == 3) {
            // update object, object_id is 3rd argument
            am_req = ame_plugin->ame_putobj_hdler(r);
        }
        break;

    case NGX_HTTP_DELETE:
        if (uri_parts.size() == 3) {
            // delete object, obj_id is 3rd argument
            am_req = ame_plugin->ame_delobj_hdler(r);
        }
        break;
    }
    if (am_req == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
        return;
    }
    fdcf = reinterpret_cast<ngx_http_fds_loc_conf_t *>
        (ngx_http_get_module_loc_conf(r, ngx_http_fds_data_module));

    am_ctx = fdcf->fds_context->ame_get_ctx(am_req);
    am_ctx->ame_register_ctx();
    am_req->ame_add_context(am_ctx);
    am_req->ame_request_handler();
}

/*
 * ngx_http_fds_data_handler
 * -------------------------
 */
static ngx_int_t
ngx_http_fds_data_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;

    rc = ngx_http_read_client_request_body(r, ngx_http_fds_read_body);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }
    return NGX_DONE;
}

/*
 * ngx_http_fds_data_handler_atmos
 * -------------------------------
 */
static ngx_int_t
ngx_http_fds_data_handler_atmos(ngx_http_request_t *r)
{
    ngx_int_t rc;

    rc = ngx_http_read_client_request_body(r, ngx_http_fds_read_body_atmos);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }
    return NGX_DONE;
}

/*
 * ngx_http_fds_data_connector
 * ---------------------------
 */
static char *
ngx_http_fds_data_connector(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;
    ngx_http_fds_loc_conf_t   *fdscf;

    clcf = reinterpret_cast<ngx_http_core_loc_conf_t *>
        (ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module));
    clcf->handler = ngx_http_fds_data_handler;

    fdscf = reinterpret_cast<ngx_http_fds_loc_conf_t *>(conf);
    fdscf->fds_enable = 1;

    return NGX_CONF_OK;
}

/*
 * ngx_http_fds_data_connector_atmos
 * ---------------------------------
 */
static char *
ngx_http_fds_data_connector_atmos(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;
    ngx_http_fds_loc_conf_t   *fdscf;

    clcf = reinterpret_cast<ngx_http_core_loc_conf_t *>
        (ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module));
    clcf->handler = ngx_http_fds_data_handler_atmos;

    fdscf = reinterpret_cast<ngx_http_fds_loc_conf_t *>(conf);
    fdscf->fds_enable = 1;

    return NGX_CONF_OK;
}

/*
 * ngx_http_fds_data_create_loc_conf
 * ---------------------------------
 */
static void *
ngx_http_fds_data_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_fds_loc_conf_t *fdcf;

    if (sgt_fds_data_cfg == NULL) {
        fdcf = reinterpret_cast<ngx_http_fds_loc_conf_t *>
            (ngx_pcalloc(cf->pool, sizeof(*fdcf)));

        if (fdcf == NULL) {
            return NGX_CONF_ERROR;
        }
        sgt_fds_data_cfg  = fdcf;
        fdcf->fds_context = new fds::AME_CtxList(cf->cycle->connection_n);
        fdcf->fds_enable  = NGX_CONF_UNSET;
    }
    return sgt_fds_data_cfg;
}

/*
 * ngx_http_fds_data_merge_loc_conf
 * --------------------------------
 */
static char *
ngx_http_fds_data_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    // ngx_http_fds_loc_conf_t *prev;
    // ngx_http_fds_loc_conf_t *self;

    static_cast<ngx_http_fds_loc_conf_t *>(parent);
    static_cast<ngx_http_fds_loc_conf_t *>(child);
    return NGX_CONF_OK;
}

/*
 * ame_context_handler
 * -------------------
 */
static void
ame_context_handler(ngx_event_t *evt)
{
    fds::AME_Ctx     *ctx;
    ngx_connection_t *c;

    c   = static_cast<ngx_connection_t *>(evt->data);
    ctx = static_cast<fds::AME_Ctx *>(c->data);
    ctx->ame_invoke_handler();
}
} /* extern "C" */

namespace fds {

/*
 * AME_Ctx
 * -------
 * Constructor for the context object that acts as "upstream" module to nginx.
 */
AME_Ctx::AME_Ctx() : ame_req(NULL), ame_next(NULL), ame_temp_buf(NULL)
{
    // Start the ctx stream at the beginning
    ame_buf_off          = 0;
    ame_ack_count        = 0;
    ame_received_get_len = 0;
    ame_map_lock         = new fds_mutex("ame ctx lock");
    ame_header_sent      = false;
    ame_init_ngx_ctx();
}

// ame_init_ngx_ctx
// ----------------
//
void
AME_Ctx::ame_init_ngx_ctx()
{
    ame_epoll_fd  = 0;
    ame_connect   = NULL;
    ame_out_buf   = NULL;
    ame_in_chain  = NULL;

    ame_output.buf    = NULL;
    ame_output.next   = NULL;
    ame_free_buf.buf  = NULL;
    ame_free_buf.next = NULL;
}

/*
 * ame_register_handler
 * --------------------
 * Register for the handler to be called from nginx when an "upstream" thread
 * notified it with ame_notify_handler() call.
 */
void
AME_Ctx::ame_invoke_handler()
{
    if (ame_req->ame_request_resume() == NGX_DONE) {
        ame_free_context();
    }
}

/*
 * ame_alloc_buf
 * -------------
 */
ngx_buf_t *
AME_Ctx::ame_alloc_buf(int len, char **buf, int *got)
{
    ngx_buf_t          *b;
    ngx_http_request_t *r;

    if (len > 0) {
        r = ame_req->ame_req;
        b = static_cast<ngx_buf_t *>(ngx_calloc_buf(r->pool));
        ngx_memset(b, 0, sizeof(*b));

        b->memory = 1;
        b->start  = static_cast<u_char *>(ngx_palloc(r->pool, len));
        b->end    = b->start + len;
        b->pos    = b->start;
        b->last   = b->end;
        *buf      = reinterpret_cast<char *>(b->pos);
        *got      = len;
        return b;
    }
    *buf = NULL;
    *got = 0;
    return NULL;
}

/*
 * ame_pop_output_buf
 * ------------------
 */
void
AME_Ctx::ame_pop_output_buf()
{
    // XXX: not now.
}

/*
 * ame_free_context
 * ----------------
 * Call to cleanup resources used to handle the request.
 */
void
AME_Ctx::ame_free_context()
{
    delete ame_req;
    ame_unregister_ctx();
}

fds_off_t
AME_Ctx::ame_get_offset() {
    return ame_buf_off;
}

/** We expect this function to be called by a
 * single nginx thread, so doesn't need locking
 */
void
AME_Ctx::ame_add_alloc_buf(char *buf,
                           fds_uint32_t len) {
    ame_alloc_bufs.push_back(AME_Buf_Desc(buf, len));
}

/**
 * We expect this function to be called by a single
 * nginx thread, so doesn't need locking
 */
void
AME_Ctx::ame_free_bufs() {
    while (ame_alloc_bufs.empty() != true) {
        char *buf = ame_alloc_bufs.front().first;
        ame_alloc_bufs.pop_front();
        fds_verify(buf != NULL);
        ngx_pfree(ame_req->ame_req->pool, buf);
    }
}

/**
 * Adds a pending offset request to the
 * context.

 * @param[in] The stream offset being modified
 * @return The result of the update
 */
Error
AME_Ctx::ame_add_ctx_req(fds_off_t offset) {
    ame_map_lock->lock();
    if (ame_req_map.count(offset) != 0) {
        ame_map_lock->unlock();
        return Error(ERR_DUPLICATE);
    }
    ame_req_map[offset] = WAITING;
    ame_map_lock->unlock();
    return Error(ERR_OK);
}

/**
 * Expects lock to be held by caller
 */
Error
AME_Ctx::ame_upd_ctx_req_locked(fds_off_t offset,
                                AME_Ctx_Ack ack_status) {
    if (ame_req_map.count(offset) == 0) {
        return Error(ERR_NOT_FOUND);
    }
    ame_req_map[offset] = ack_status;
    return Error(ERR_OK);
}

/**
 * Updates the state of a pending offset
 * request
 */
Error
AME_Ctx::ame_upd_ctx_req(fds_off_t offset,
                         AME_Ctx_Ack ack_status) {
    ame_map_lock->lock();
    Error err = ame_upd_ctx_req_locked(offset, ack_status);
    ame_map_lock->unlock();
    return err;
}

/**
 * Sets the number of acks expected to
 * be received in order to call the request
 * complete.
 * We do not lock because we expect to be
 * called in a single threaded env.
 */
void
AME_Ctx::set_ack_count(fds_uint32_t count) {
    ame_ack_count = count;
}

/**
 * Expects lock to be held by caller
 */
AME_Ctx::AME_Ctx_Ack
AME_Ctx::ame_check_status_locked() {
    if (ame_req_map.size() < ame_ack_count) {
        // We haven't sent all of the requests
        // that we're going to send.
        return WAITING;
    }

    // Check the status of each request that
    // we did send to see if
    // the request passed or failed or is pending
    fds_bool_t failed = false;
    for (AME_Ack_Map::const_iterator it = ame_req_map.cbegin();
         it != ame_req_map.cend();
         it++) {
        if (it->second == WAITING) {
            return WAITING;
        } else if (it->second == FAILED) {
            failed = true;
        }
    }

    if (failed == true) {
        return FAILED;
    }
    return OK;
}

/**
 * Returns the status of all of the
 * currently received acks.
 */
AME_Ctx::AME_Ctx_Ack
AME_Ctx::ame_check_status() {
    ame_map_lock->lock();
    AME_Ctx::AME_Ctx_Ack ack = ame_check_status_locked();
    ame_map_lock->unlock();
    return ack;
}

/**
 * Locks the context
 */
void
AME_Ctx::ame_ctx_lock() {
    ame_map_lock->lock();
}

/**
 * Unlocks the context
 */
void
AME_Ctx::ame_ctx_unlock() {
    ame_map_lock->unlock();
}

/**
 * Returns the next acked but unsent offset in the
 * request stream. The offset is only returned if all of
 * the previous offsets have been acked and sent.
 * If this is not the case, an error is returned as there
 * is no offset that is ready to be sent next.
 */
Error
AME_Ctx::ame_get_unsent_offset(fds_off_t  *offset,
                               fds_bool_t *last) {
    *last = false;
    ame_map_lock->lock();
    Error err = ame_get_unsent_offset_locked(offset, last);
    ame_map_lock->unlock();

    return err;
}

/**
 * Expects lock to be held by caller
 */
Error
AME_Ctx::ame_get_unsent_offset_locked(fds_off_t  *offset,
                                      fds_bool_t *last) {
    Error      err(ERR_OK);
    *last = false;

    for (AME_Ack_Map::const_iterator it = ame_req_map.cbegin();
         it != ame_req_map.cend();
         it++) {
        // TODO(Andrew): We need to actually handle
        // stream errors...not sure what to do when
        // they occur mid-stream
        fds_verify(it->second != FAILED);

        if (it->second == WAITING) {
            // The offset's response hasn't been received yet
            err = ERR_NOT_FOUND;
            return err;
        } else if (it->second == OK) {
            // The offset was ack'd OK, but not sent yet
            // so it's OK to send this one
            *offset = it->first;
            if (std::next(it) == ame_req_map.cend()) {
                *last = true;
            }
            return err;
        } else {
            fds_verify(it->second == SENT);
        }
    }

    err = ERR_NOT_FOUND;
    return err;
}

/**
 * Moves the context offset by len
 * Note, no lock is acquired because we're assuming
 * the caller will not call from multi-threaded envs
 */
void
AME_Ctx::ame_mv_off(fds_uint32_t len) {
    ame_buf_off += len;
}

fds_off_t
AME_Ctx::ame_get_off() const {
    return ame_buf_off;
}

ngx_md5_t *
AME_Ctx::ame_get_etag() {
    return &ame_etag_ctx;
}

/**
 * Sets the requested get range to the
 * entire blob. The proper blob size will
 * be set later.
 * Note, we don't lock here because expect
 * it to only be called by a single nginx
 * request thread.
 */
void
AME_Ctx::ame_set_request_all() {
    ame_req_get_range = ALL;
    ame_req_get_len   = 0;
}

/**
 * Sets the specific length being requested.
 * Only has an effect if a specific length wasn't
 * already set, making it idempotent
 */
fds_bool_t
AME_Ctx::ame_set_specific(fds_uint64_t len) {
    fds_bool_t allSet = false;
    ame_map_lock->lock();
    if (ame_req_get_range == ALL) {
        allSet            = true;
        ame_req_get_range = SPECIFIC;
        ame_req_get_len   = len;
    }
    ame_map_lock->unlock();
    return allSet;
}

/**
 * Expects lock to be held by caller
 */
fds_uint64_t
AME_Ctx::ame_get_requested_len_locked() {
    fds_verify(ame_req_get_range == SPECIFIC);
    return ame_req_get_len;
}

/**
 * Returns the requested length.
 * Assumes a specific length has already been set
 */
fds_uint64_t
AME_Ctx::ame_get_requested_len() {
    fds_uint64_t len;
    ame_map_lock->lock();
    len = ame_get_requested_len_locked();
    ame_map_lock->unlock();
    return len;
}

/**
 * Stores to allocated buffer for an offset
 */
void
AME_Ctx::ame_add_get_buf(fds_off_t offset,
                         ame_buf_t *ame_buf) {
    ame_map_lock->lock();
    // Set a 0 len for now and we'll set it
    // on the callback based on how much data
    // was read
    fds_uint32_t len = 0;
    fds_verify(ame_get_bufs.count(offset) == 0);
    ame_get_bufs[offset] = AME_Get_Desc(ame_buf, len);
    ame_map_lock->unlock();
}

/**
 * Sets the length actually received from FDSN
 * for the given offset we read. Also, tracks
 * total bytes received for the entire request.
 */
void
AME_Ctx::ame_set_get_len(fds_off_t offset,
                         fds_uint32_t len) {
    ame_map_lock->lock();
    fds_verify(ame_get_bufs.count(offset) > 0);
    fds_verify(ame_get_bufs[offset].second == 0);
    ame_get_bufs[offset].second = len;
    ame_received_get_len += len;
    ame_map_lock->unlock();
}

/**
 * Note we expect this to only be called when the
 * request is being finalized, so don't need to lock.
 */
fds_uint64_t
AME_Ctx::ame_get_received_len() const {
    return ame_received_get_len;
}

/**
 * Expects lock to be held by caller
 */
void
AME_Ctx::ame_get_get_info_locked(fds_off_t    offset,
                                 ame_buf_t    **ame_buf,
                                 fds_uint32_t *len) {
    fds_verify(ame_get_bufs.count(offset) > 0);
    *ame_buf = ame_get_bufs[offset].first;
    *len     = ame_get_bufs[offset].second;
}

/**
 * Returns the buffer and received length in
 * the buffer for a given offset.
 */
void
AME_Ctx::ame_get_get_info(fds_off_t    offset,
                          ame_buf_t    **ame_buf,
                          fds_uint32_t *len) {
    ame_map_lock->lock();
    ame_get_get_info_locked(offset, ame_buf, len);
    ame_map_lock->unlock();
}

/**
 * Expects lock to be held by caller
 */
fds_bool_t
AME_Ctx::ame_get_header_sent_locked() const {
    return ame_header_sent;
}

/**
 * Returns true of we sent the header already
 */
fds_bool_t
AME_Ctx::ame_get_header_sent() const {
    fds_bool_t sent;
    ame_map_lock->lock();
    sent = ame_get_header_sent_locked();
    ame_map_lock->unlock();
    return sent;
}

/**
 * Expects lock to be held by caller
 */
void
AME_Ctx::ame_set_header_sent_locked(fds_bool_t sent) {
    ame_header_sent = sent;
}

/**
 * Set whether or not the header was sent
 */
void
AME_Ctx::ame_set_header_sent(fds_bool_t sent) {
    ame_map_lock->lock();
    ame_header_sent = sent;
    ame_map_lock->unlock();
}

/**
 * Sets the complete etag string
 */
void
AME_Ctx::ame_set_etag_result(const std::string &etag) {
    ame_map_lock->lock();
    ame_etag_result = etag;
    ame_map_lock->unlock();
}

/**
 * Expects lock to be held by caller
 */
std::string
AME_Ctx::ame_get_etag_result_locked() {
    return ame_etag_result;
}

/**
 * Gets the final etag string
 */
std::string
AME_Ctx::ame_get_etag_result() {
    std::string etag;
    ame_map_lock->lock();
    etag  = ame_get_etag_result_locked();
    ame_map_lock->unlock();
    return etag;
}

/*
 * ame_notify_handler
 * ------------------
 * Called by the "upstream" handler to notify the handler run by nginx's event
 * thread to handle the event associated with the context.
 */
void
AME_Ctx::ame_notify_handler()
{
    int rc;

    /* XXX: TODO must have timeout if fails here. */
    rc = eventfd_write(ame_epoll_fd, 0xfddf);
    fds_verify(rc == 0);
}

/*
 * ame_register_ctx
 * ----------------
 * Register the context obj with nginx's event loop so that it could invoke
 * the handler when service thread notifies it with ame_notify_handler().
 *
 * Note: only nginx thread calls register/unregister method so that we
 * can add/remove ctx obj w/out lock.
 */
int
AME_Ctx::ame_register_ctx()
{
    ngx_int_t           evt;
    ngx_event_t        *rev, *wev;
    ngx_connection_t   *c;
    ngx_http_request_t *r;

    fds_verify(ame_epoll_fd == 0);
    ame_epoll_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (ame_epoll_fd <= 0) {
        ame_epoll_fd = 0;
        return NGX_ERROR;
    }
    ngx_http_set_ctx(ame_req->ame_req, this, ngx_http_fds_data_module);

    r = ame_req->ame_req;
    c = r->connection;
    ame_connect = ngx_get_connection(ame_epoll_fd, r->connection->log);
    if (ame_connect == NULL) {
        ngx_log_error(NGX_LOG_ALERT, c->log, NGX_ERROR, "no more connection");
        return NGX_ERROR;
    }
    if (ngx_add_conn(ame_connect) == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, NGX_ERROR, "failed to add conn");
        goto failed;
        return NGX_ERROR;
    }
    rev = ame_connect->read;
    wev = ame_connect->write;
    rev->log = c->log;
    wev->log = c->log;

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) {
        evt = NGX_CLEAR_EVENT;
    } else {
        evt = NGX_LEVEL_EVENT;
    }
    if (ngx_add_event(ame_connect->read, NGX_READ_EVENT, evt) != NGX_OK) {
        goto failed;
    }
    wev->active       = 0;
    rev->ready        = 1;
    rev->handler      = ame_context_handler;
    ame_connect->data = static_cast<void *>(this);

    return NGX_OK;

 failed:
    ngx_free_connection(ame_connect);
    printf("Failed to add event loop\n");
    return NGX_ERROR;
}

// ame_unregister_ctx
// ------------------
//
void
AME_Ctx::ame_unregister_ctx()
{
    // LOGDEBUG << "Unregistering ngx ctx";
    ngx_close_connection(ame_connect);

    if (ame_epoll_fd != 0) {
        close(ame_epoll_fd);
    }
    sgt_fds_data_cfg->fds_context->ame_put_ctx(this);
}

/*
 * AME_CtxList
 * -----------
 * Manager to manage free list of ctx objs used to keep track of requests
 * submitted to FDS upstream module.
 */
AME_CtxList::AME_CtxList(int elm) : ame_free_ctx_cnt(elm)
{
    int i;

    ame_arr_ctx  = new AME_Ctx [elm];
    ame_free_ctx = &ame_arr_ctx[0];
    for (i = 1; i < elm; i++) {
        fds_verify(ame_arr_ctx[i].ame_next == NULL);
        ame_arr_ctx[i - 1].ame_next = &ame_arr_ctx[i];
    }
}

/*
 * ame_get_ctx
 * -----------
 * Only ngx can call ame_get/put_ctx methods so that we can get/put ctx obj
 * without lock.
 */
AME_Ctx *
AME_CtxList::ame_get_ctx(AME_Request *req)
{
    if (ame_free_ctx != NULL) {
        AME_Ctx *ctx = ame_free_ctx;
        ame_free_ctx = ctx->ame_next;
        ame_free_ctx_cnt--;
        fds_verify(ame_free_ctx_cnt >= 0);

        ctx->ame_next  = NULL;
        ctx->ame_req   = req;
        ctx->ame_state = 0;

        ctx->ame_buf_off = 0;
        ctx->ame_req_map.clear();
        ctx->ame_ack_count = 0;

        fds_verify(ctx->ame_alloc_bufs.empty() == true);
        HttpUtils::initEtag(&(ctx->ame_etag_ctx));

        ctx->ame_header_sent = false;
        ctx->ame_get_bufs.clear();
        ctx->ame_received_get_len = 0;

        return ctx;
    }
    return NULL;
}

/*
 * ame_put_ctx
 * -----------
 * Only ngx can call ame_get/put_ctx methods so that we can get/put ctx obj
 * without lock.
 */
void
AME_CtxList::ame_put_ctx(AME_Ctx *ctx)
{
    fds_verify((ctx != NULL) && (ctx->ame_next == NULL));

    ctx->ame_init_ngx_ctx();
    ctx->ame_next = ame_free_ctx;
    ame_free_ctx  = ctx;
    ame_free_ctx_cnt++;
}

}   // namespace fds
