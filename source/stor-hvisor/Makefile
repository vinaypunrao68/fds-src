topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    :=

# TODO: remove the topdir path in the include search
#
user_incl_dir     := $(topdir) $(topdir)/fds_client/include $(topdir)/lib
user_fds_ar_libs  := fds-umod fds-hash fds_ubd
user_non_fds_libs := cryptopp
StorHvisor_cpp    :=                   \
    StorHvisorNet.cpp                  \
    StorHvisorTest.cpp                  \
    StorHvisorS3.cpp                  \
    StorHvisorTx.cpp                   \
    StorHvisorTbl.cpp                  \
    StorHvisorRx.cpp                   \
    VolumeCatalogCache.cpp             \
    StorHvJournal.cpp                  \
    qos_ctrl.cpp	                   \
    StorHvQosCtrl.cpp                  \
    StorHvVolumes.cpp		           \
    StorHvNative.cpp	               \
    FDSNStat.cpp

user_cpp          :=                   \
    $(StorHvisor_cpp)

user_no_style     := $(user_cpp) $(user_cc) $(wildcard *.h)
user_ar_libs      := fds-StorHvisor
fds-StorHvisor    := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
