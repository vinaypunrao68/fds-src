nginx_src_dir    := ../..
nginx_dir        := $(nginx_src_dir)/..
user_target      := lib
user_rtime_env   := user
user_cc          := \
    ngx_http_static_module.c \
    ngx_http_log_module.c \
    ngx_http_index_module.c \
    ngx_http_chunked_filter_module.c \
    ngx_http_range_filter_module.c \
    ngx_http_headers_filter_module.c \
    ngx_http_not_modified_filter_module.c \
    ngx_http_gzip_filter_module.c \
    ngx_http_ssi_filter_module.c \
    ngx_http_charset_filter_module.c \
    ngx_http_userid_filter_module.c \
    ngx_http_autoindex_module.c \
    ngx_http_auth_basic_module.c \
    ngx_http_access_module.c \
    ngx_http_limit_conn_module.c \
    ngx_http_limit_req_module.c \
    ngx_http_geo_module.c \
    ngx_http_map_module.c \
    ngx_http_split_clients_module.c \
    ngx_http_referer_module.c \
    ngx_http_rewrite_module.c \
    ngx_http_proxy_module.c \
    ngx_http_fastcgi_module.c \
    ngx_http_uwsgi_module.c \
    ngx_http_scgi_module.c \
    ngx_http_memcached_module.c \
    ngx_http_empty_gif_module.c \
    ngx_http_browser_module.c \
    ngx_http_upstream_ip_hash_module.c \
    ngx_http_upstream_least_conn_module.c \
    ngx_http_upstream_keepalive_module.c

user_ar_libs    := ngx-http-module
ngx-http-module := $(user_cc)

include $(nginx_dir)/Makefile.incl
