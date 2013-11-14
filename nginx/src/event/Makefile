nginx_src_dir    := ..
nginx_dir        := $(nginx_src_dir)/..
user_target      := lib
user_build_dir   := modules
user_rtime_env   := user
user_cc          := \
    ngx_event.c \
    ngx_event_timer.c \
    ngx_event_posted.c \
    ngx_event_busy_lock.c \
    ngx_event_accept.c \
    ngx_event_connect.c \
    ngx_event_pipe.c

user_ar_libs     := ngx-event
ngx-event        := $(user_cc)

include $(nginx_dir)/Makefile.incl
