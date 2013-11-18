nginx_src_dir    := ..
nginx_dir        := $(nginx_src_dir)/..
user_target      := lib
user_rtime_env   := user
user_cc          := \
    nginx.c \
    ngx_log.c \
    ngx_palloc.c \
    ngx_array.c \
    ngx_list.c \
    ngx_hash.c \
    ngx_buf.c \
    ngx_queue.c \
    ngx_output_chain.c \
    ngx_string.c \
    ngx_parse.c \
    ngx_inet.c \
    ngx_file.c \
    ngx_crc32.c \
    ngx_murmurhash.c \
    ngx_md5.c \
    ngx_rbtree.c \
    ngx_radix_tree.c \
    ngx_slab.c \
    ngx_times.c \
    ngx_shmtx.c \
    ngx_connection.c \
    ngx_cycle.c \
    ngx_spinlock.c \
    ngx_cpuinfo.c \
    ngx_conf_file.c \
    ngx_resolver.c \
    ngx_open_file_cache.c \
    ngx_crypt.c \
    ngx_regex.c

user_ar_libs     := ngx-core
ngx-core         := $(user_cc)

include $(nginx_dir)/Makefile.incl
