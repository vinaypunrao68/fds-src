topdir            := ..
user_target       := exe
user_rtime_env    := user

user_incl_dir     :=
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-am-engine \
    fds-umod \
    fds-thread

user_nginx_libs   :=  \
    ngx-http-module \
    ngx-http \
    ngx-event-module \
    ngx-event \
    ngx-core \
    ngx-os \
    ngx-core \
    pthread crypt pcre crypto z

user_non_fds_libs :=  \
    boost_system \
    boost_thread \
    boost_program_options \
    $(user_nginx_libs)

user_cc           :=  \
    ngx_modules.c

user_cpp          :=  \
    am-main.cpp

user_bin_exe      := AMAgent
AMAgent           := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
