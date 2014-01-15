topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    :=
user_install_loc  := exe
user_install      :=

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := $(topdir) include
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-lib          \
    fds-umod         \
    fds-thread       \
    fds-fdsp

user_non_fds_libs :=      \
    boost_log             \
    boost_system          \
    boost_filesystem      \
    boost_thread          \
    boost_date_time       \
    boost_program_options \
    ssl                   \
    crypto                \
    leveldb               \
    config++              \
    thrift                \
    rt

example_clnt_cpp  := fds_net_example_client.cpp
example_srvr_cpp  := fds_net_example_server.cpp
user_cpp          := \
    $(example_clnt_cpp) $(example_srvr_cpp)

user_no_style     := 
user_bin_exe      := ExClient ExServer
ExClient          := $(example_clnt_cpp)
ExServer          := $(example_srvr_cpp)

include $(topdir)/Makefile.incl
