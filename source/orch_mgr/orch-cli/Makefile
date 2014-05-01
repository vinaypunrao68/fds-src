topdir            := ../..
user_target       := exe
user_rtime_env    := user
user_install      :=
user_install_loc  :=

user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-lib \
    fds-umod  \
    fds-util \
    fds-fdsp \
    fds-thread \
    fds-osdep

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    leveldb config++ thrift

user_incl_dir     :=

fds_cli_cpp       :=                   \
    fdscli.cpp cli-policy.cpp

user_cpp          :=                   \
    $(fds_cli_cpp)                     \

user_hh           := fdsCli.h

user_bin_exe      := fdscli
fdscli            := $(fds_cli_cpp)

include $(topdir)/Makefile.incl
