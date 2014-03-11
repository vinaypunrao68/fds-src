topdir            := ..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

# TODO: remove the topdir path in the include search
#
user_incl_dir     := \
    $(topdir) \
    $(topdir)/fds_client/include \
    $(topdir)/lib \
    $(topdir)/am-app/include

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
