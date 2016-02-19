topdir            := $(test_topdir)/..
user_target       := test
user_std_libs     := true
user_rtime_env    := user

user_incl_dir     := \
    $(topdir) ../include \
    $(topdir)/platform/include \
    $(topdir)/data-mgr/include \
    $(topdir)/access-mgr/include \
    $(topdir)/lib \
    $(topdir)/util \
    $(topdir)

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=  \
    fds-qos-lib \
    fds-dm-lib \
    fds-am-engine \
    fds-am-connectors \
    fds-dsk-mgnt \
    fds-svc-request \
    fds-umod \
    fds-lib \
    fds-util \
    fds-fdsp \
    fds-hash \
    fds-thread \
    fds-test \
    fds-net-svc \
    fds-fdsp \
    fds-osdep \
    fds-volume-checker \
    fds-dm-lib

user_non_fds_libs :=  \
    boost_log \
    boost_system \
    boost_thread \
    boost_date_time \
    boost_program_options \
    boost_filesystem \
    boost_regex \
    boost_timer \
    thrift \
    jansson \
    config++ \
    crypt \
    crypto \
    gmock \
    hiredis \
    event \
    statsclient \
    rt \
    fiu \
    dl \
    profiler \
    ssl \
    leveldb \
    z \
    pthread \
    pcrecpp \
    sqlite3  \
    curl \
    jansson \
    tcmalloc

user_static_libs  := \
    ev

include $(topdir)/Makefile.incl
