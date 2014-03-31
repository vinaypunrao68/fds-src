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

user_cpp_flags    := -DVVC_CACHE_DISABLE

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

user_cpp       := \
    $(StorHvisor_cpp)
user_hh        := $(wildcard *.h)
user_hpp       := $(wildcard *.h)

user_no_style     := $(user_cc) \
    list.h \
    StorHvDataPlace.h \
    StorHvisorClient.h \
    StorHvisorCPP.h \
    StorHvisorNet.h \
    StorHvJournal.h \
    StorHvQosCtrl.h \
    StorHvVolumes.h \
    FDSNStat.h \
    StorHvisorNet.cpp \
    StorHvisorTest.cpp \
    StorHvisorS3.cpp \
    StorHvisorTx.cpp \
    StorHvisorTbl.cpp \
    StorHvisorRx.cpp \
    StorHvJournal.cpp \
    qos_ctrl.cpp \
    StorHvQosCtrl.cpp \
    StorHvVolumes.cpp \
    StorHvNative.cpp \
    FDSNStat.cpp

#user_no_style     :=
user_ar_libs      := fds-StorHvisor
fds-StorHvisor    := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
