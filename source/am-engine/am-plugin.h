/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_AM_ENGINE_AM_PLUGIN_H_
#define INCLUDE_AM_ENGINE_AM_PLUGIN_H_

#include <am-engine/am-engine.h>

extern "C" {
extern void ngx_register_plugin(fds::AMEngine *engine);
extern int  ngx_main(int argc, char const **argv);

/* Methods for ngx module to call out to process http requests. */
//extern ngx_int_t ngx_fds_obj_get(ngx_http_request_t *r);
//extern ngx_int_t ngx_fds_obj_put(ngx_http_request_t *r);
//extern ngx_int_t ngx_fds_obj_delete(ngx_http_request_t *r);
//extern ngx_int_t ngx_fds_bucket_create(ngx_http_request_t *r);
//extern ngx_int_t ngx_fds_bucket_delete(ngx_http_request_t *r);
}

#endif /* INCLUDE_AM_ENGINE_AM_PLUGIN_H_ */
