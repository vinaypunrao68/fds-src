nginx_src_dir     := ..
nginx_dir         := $(nginx_src_dir)/..
nginx_cfg_dir     := $(nginx_dir)/objs
user_target       := exe
user_rtime_env    := user
user_predep       := ngx_modules.c
user_clean        := $(user_predep)

user_cc           := \
    ngx_modules.c \
    nginx_main.c

user_non_fds_libs := \
    ngx-http-module \
    ngx-http \
    ngx-event-module \
    ngx-event \
    ngx-core \
    ngx-os \
    ngx-core \
    pthread crypt pcre crypto z

user_bin_exe      := nginx
nginx             := $(user_cc)

include $(nginx_dir)/Makefile.incl

ngx_modules.c: $(nginx_cfg_dir)/ngx_modules.c
	@echo "    [SoftLink]"
	@ln -s $(nginx_cfg_dir)/ngx_modules.c $@
