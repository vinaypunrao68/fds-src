nginx_src_dir    := ../..
nginx_dir        := $(nginx_src_dir)/..
user_target      := lib
user_rtime_env   := user
user_cc          := \
    ngx_epoll_module.c

user_ar_libs     := ngx-event-module
ngx-event-module := $(user_cc)

include $(nginx_dir)/Makefile.incl
