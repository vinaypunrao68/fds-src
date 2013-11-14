nginx_src_dir    := ..
nginx_dir        := $(nginx_src_dir)/..
user_target      := lib
user_rtime_env   := user
user_build_dir   := modules
user_cc          := \
    ngx_http.c \
    ngx_http_core_module.c \
    ngx_http_special_response.c \
    ngx_http_request.c \
    ngx_http_parse.c \
    ngx_http_header_filter_module.c \
    ngx_http_write_filter_module.c \
    ngx_http_copy_filter_module.c \
    ngx_http_request_body.c \
    ngx_http_variables.c \
    ngx_http_script.c \
    ngx_http_upstream.c \
    ngx_http_upstream_round_robin.c \
    ngx_http_parse_time.c \
    ngx_http_busy_lock.c \
    ngx_http_file_cache.c \
    ngx_http_postpone_filter_module.c

user_ar_libs  := ngx-http
ngx-http      := $(user_cc)

include $(nginx_dir)/Makefile.incl
