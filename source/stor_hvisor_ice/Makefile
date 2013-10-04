topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build        := unit-test

# TODO: remove the topdir path in the include search
#
user_incl_dir     := $(topdir) $(topdir)/fds_client/include
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := fds-thread fds-util fds-fdsp fds-lib
user_non_fds_libs :=                                                          \
    boost_log boost_system boost_thread boost_date_time boost_program_options \
    Ice IceUtil IceStorm leveldb ssl crypto leveldb

StorHvisor_cpp    :=                   \
    StorHvisorNet.cpp                  \
    RPC_EndPoint.cpp                   \
    StorHvisorTx.cpp                   \
    StorHvisorTbl.cpp                  \
    StorHvisorRx.cpp                   \
    VolumeCatalogCache.cpp             \
    StorHvJournal.cpp                  \
    StorHvVolumes.cpp

user_cpp          :=                   \
    $(StorHvisor_cpp)

user_ar_libs      := StorHvisor
StorHvisor        := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
