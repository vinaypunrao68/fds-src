topdir            := ../..
user_target       := exe
user_rtime_env    := user
user_install      :=
user_install_loc  :=

user_fds_so_libs  :=
user_fds_ar_libs  :=                                                          \
    fds-thread fds-util fds-netproxy fds-fdsp-ice fds-umod fds-lib

user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil leveldb config++

user_incl_dir     :=

fds_cli_cpp       :=                   \
    fdscli.cpp cli-policy.cpp

user_cpp          :=                   \
    $(fds_cli_cpp)                     \

user_hpp          := fdsCli.h

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := fdscli
fdscli            := $(fds_cli_cpp)

include $(topdir)/Makefile.incl
