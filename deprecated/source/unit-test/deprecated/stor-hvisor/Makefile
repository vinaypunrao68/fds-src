topdir            := ../..
hvisor_dir        := $(topdir)/stor-hvisor
user_target       := test
user_rtime_env    := user
user_install      := $(topdir)/stor-hvisor/fds.conf
user_install_loc  := test

# TODO: remove the topdir from include search path.
#
user_incl_dir     :=                          \
    $(hvisor_dir)                             \
    $(topdir)/fds_client/include              \
    $(topdir)                                 \
    $(topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine \
    fds-lib \
    fds-umod \
    fds-thread \
    fds-util \
    fds-fdsp \
    fds-hash

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    leveldb \
    ssl \
    crypto \
    leveldb \
    config++ \
    thrift

user_cpp          :=                   \
    vcc_unit_test.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := vcc_unit_test
vcc_unit_test     := vcc_unit_test.cpp

include $(topdir)/Makefile.incl
