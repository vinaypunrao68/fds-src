/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <sys/eventfd.h>
#include <am-engine/s3connector.h>
#include <am-plugin.h>
#include <am-engine/http_utils.h>
#include <boost/tokenizer.hpp>
#include <fds_assert.h>

/* The factory creating objects to handle supported commands. */
static fds::AMEngine  *sgt_ame_plugin;

extern "C" {
#include <am-plugin.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *
ngx_http_fds_data_connector(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *ngx_http_fds_data_create_loc_conf(ngx_conf_t *cf);

static char *
ngx_http_fds_data_merge_loc_conf(ngx_conf_t *cf, void *prev, void *conf);

static std::vector<std::string>
ngx_parse_uri_parts(ngx_http_request_t *r);

static void ngx_http_fds_read_body(ngx_http_request_t *r);
static ngx_int_t ngx_http_fds_data_handler(ngx_http_request_t *r);
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

typedef struct ngx_http_fds_data_loc_conf ngx_http_fds_data_loc_conf_t;
struct ngx_http_fds_data_loc_conf
{
    ngx_flag_t               fds_enable;
    fds::AME_CtxList        *fds_context;
};

static ngx_http_fds_data_loc_conf_t *sgt_fds_data_cfg;

/*
 * ngx_register_plugin
 * -------------------
 * Register the plugin having factory methods to create objects that will
 * handle supported APIs.
 */
void
ngx_register_plugin(fds::AMEngine *engine)
{
    sgt_ame_plugin = engine;
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
    fds::AME_Request         *am_req;
    HttpRequest               http_req(r);
    std::vector<std::string>  uri_parts = http_req.getURIParts();
    std::string               value;
    fds_bool_t                keyExists;

    am_req = NULL;
    switch (r->method) {
    case NGX_HTTP_GET:
        if (uri_parts.size() == 1) {
            am_req = sgt_ame_plugin->ame_getbucket_hdler(r);
        } else if (uri_parts.size() == 2) {
            am_req = sgt_ame_plugin->ame_getobj_hdler(r);
        }
	else if (uri_parts.size() == 0) {
	  /* Check if this is a get bucket stats request - 
	   * this one gets stats for all existing buckets */
          keyExists = http_req.getReqHdrVal("FdsReqType", value);
          if ((keyExists == true) && (value == "getStats")) {
	    am_req = sgt_ame_plugin->ame_getbucketstats_hdler(r);
	  }
	}
        break;

    case NGX_HTTP_POST:
    case NGX_HTTP_PUT:
        if (uri_parts.size() == 1) {
          /*
           * Check to see if modify bucket headers exist.
           * If so, call putbucketparams, if not just
           * putbucket.
           */
          keyExists = http_req.getReqHdrVal("FdsReqType", value);
          if ((keyExists == true) && (value == "modPolicy")) {
            am_req = sgt_ame_plugin->ame_putbucketparams_hdler(r);
          } else {
            am_req = sgt_ame_plugin->ame_putbucket_hdler(r);
          }
        } else if (uri_parts.size() == 2) {
            am_req = sgt_ame_plugin->ame_putobj_hdler(r);
        }
        break;

    case NGX_HTTP_DELETE:
        if (uri_parts.size() == 1) {
            am_req = sgt_ame_plugin->ame_delbucket_hdler(r);
        } else if (uri_parts.size() == 2) {
            am_req = sgt_ame_plugin->ame_delobj_hdler(r);
        }
        break;
    }
    if (am_req != NULL) {
        am_req->ame_request_handler();
        delete am_req;
    } else {
        ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);
    }
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
 * ngx_http_fds_data_connector
 * ---------------------------
 */
static char *
ngx_http_fds_data_connector(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t     *clcf;
    ngx_http_fds_data_loc_conf_t *fdscf;

    clcf = (ngx_http_core_loc_conf_t *)
        ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_fds_data_handler;

    fdscf = (ngx_http_fds_data_loc_conf_t *)conf;
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
    ngx_http_fds_data_loc_conf_t *fdscf;

    if (sgt_fds_data_cfg == NULL) {
        fdscf = (ngx_http_fds_data_loc_conf_t *)
            ngx_pcalloc(cf->pool, sizeof(*fdscf));

        if (fdscf == NULL) {
            return NGX_CONF_ERROR;
        }
        sgt_fds_data_cfg   = fdscf;
        fdscf->fds_context = new fds::AME_CtxList(cf->cycle->connection_n);
        fdscf->fds_enable  = NGX_CONF_UNSET;
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
    ngx_http_fds_data_loc_conf_t *prev;
    ngx_http_fds_data_loc_conf_t *self;

    prev = (ngx_http_fds_data_loc_conf_t *)parent;
    self = (ngx_http_fds_data_loc_conf_t *)child;
    return NGX_CONF_OK;
}

} /* extern "C" */

namespace fds {

/*
 * AME_Ctx
 * -------
 * Constructor for the context object that acts as "upstream" module to nginx.
 */
AME_Ctx::AME_Ctx()
    : ame_req(NULL), ame_ngx_req(NULL), ame_handler_fn(NULL), ame_next(NULL)
{
    static int cnt = 0;

    ame_epoll_fd = eventfd(0xfdfd, EFD_CLOEXEC | EFD_NONBLOCK);
    fds_verify(ame_epoll_fd > 0);
}

/*
 * ame_register_handler
 * --------------------
 * Register for the handler to be called from nginx when an "upstream" thread
 * notified it with ame_notify_handler() call.
 */
void
AME_Ctx::ame_register_handler(void (*handler)(AME_Request *req))
{
    fds_verify(ame_handler_fn == NULL);
    ame_handler_fn = handler;
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
void
AME_Ctx::ame_register_ctx()
{
}

void
AME_Ctx::ame_unregister_ctx()
{
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

    // TODO: increase sys limit.
    elm = 512;
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
AME_CtxList::ame_get_ctx()
{
    if (ame_free_ctx != NULL) {
        AME_Ctx *ctx = ame_free_ctx;
        ame_free_ctx = ctx->ame_next;
        ame_free_ctx_cnt--;
        fds_verify(ame_free_ctx_cnt >= 0);

        ctx->ame_next = NULL;
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
    ctx->ame_next = ame_free_ctx;
    ame_free_ctx  = ctx;
    ame_free_ctx_cnt++;
}

} // namespace fds
