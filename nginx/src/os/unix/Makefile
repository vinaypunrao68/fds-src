nginx_src_dir    := ../..
nginx_dir        := $(nginx_src_dir)/..
user_target      := lib
user_rtime_env   := user
user_cc          := \
    ngx_time.c \
    ngx_errno.c \
    ngx_alloc.c \
    ngx_files.c \
    ngx_socket.c \
    ngx_recv.c \
    ngx_readv_chain.c \
    ngx_udp_recv.c \
    ngx_send.c \
    ngx_writev_chain.c \
    ngx_channel.c \
    ngx_shmem.c \
    ngx_process.c \
    ngx_daemon.c \
    ngx_setaffinity.c \
    ngx_setproctitle.c \
    ngx_posix_init.c \
    ngx_user.c \
    ngx_process_cycle.c \
    ngx_linux_init.c \
    ngx_linux_sendfile_chain.c

user_ar_libs     := ngx-os
ngx-os           := $(user_cc)

include $(nginx_dir)/Makefile.incl
