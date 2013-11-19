/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-plugin.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *
ngx_http_fds_data_connector(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *
ngx_http_fds_data_create_loc_conf(ngx_conf_t *cf);

static char *
ngx_http_fds_data_merge_loc_conf(ngx_conf_t *cf, void *prev, void *conf);

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
};

static void
ngx_http_fds_read_body(ngx_http_request_t *r)
{
    ngx_int_t                     rc;
    ngx_chain_t                   out;
    ngx_buf_t                    *b;
    ngx_table_elt_t              *h;
    ngx_http_fds_data_loc_conf_t *fdscf;

    h = r->headers_in.host;
    printf("Got request %p, header:\n", r);
    printf("\tHost: key %s, value %s\n", h->key.data, h->value.data);
    printf("\tURI: %s\n", r->request_line.data);
    printf("\tMethod: %s\n", r->method_name.data);
    printf("\tProtocol: %s\n", r->http_protocol.data);
    printf("\tData: %s\n", r->request_body->bufs->buf->start);

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = sizeof("Hello world") - 1;

    rc = ngx_http_send_header(r);
    b = ngx_calloc_buf(r->pool);
    out.buf  = b;
    out.next = NULL;

    b->start = b->pos = "Hello world";
    b->end = b->last = b->start + sizeof("Hello world") - 1;
    b->memory        = 1;
    b->last_buf      = 1;
    b->last_in_chain = 1;

    rc = ngx_http_output_filter(r, &out);
}

/*
 * ngx_http_fds_data_handler
 * -------------------------
 */
static ngx_int_t
ngx_http_fds_data_handler(ngx_http_request_t *r)
{
    ngx_int_t                     rc;
    ngx_http_fds_data_loc_conf_t *fdscf;

    fdscf = ngx_http_get_module_loc_conf(r, ngx_http_fds_data_module);
    /*
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }
    */
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

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_fds_data_handler;

    fdscf = conf;
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

    fdscf = ngx_pcalloc(cf->pool, sizeof(*fdscf));
    if (fdscf == NULL) {
        return NGX_CONF_ERROR;
    }
    fdscf->fds_enable = NGX_CONF_UNSET;
    return fdscf;
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

    prev = parent;
    self = child;
    return NGX_CONF_OK;
}
