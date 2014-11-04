topdir            := ../../..
user_target       := lib
user_rtime_env    := user

# TODO: remove the topdir path in the include search
#
user_incl_dir     := .. \
    $(topdir) \
    $(topdir)/lib \
    $(topdir)/access-mgr/include \
    $(topdir)/access-mgr/stor-hvisor \
		/usr/include/cryptopp

# user_cpp_flags    := -DVVC_CACHE_DISABLE
user_cpp_flags    :=

StorHvisor_cpp    :=                   \
    StorHvisorNet.cpp                  \
    StorHvisorS3.cpp                  \
    StorHvisorSvc.cpp                  \
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
    StorHvQosCtrl.h \
    StorHvVolumes.h \
    StorHvisorNet.cpp \
    StorHvisorTest.cpp \
    StorHvisorS3.cpp \
    StorHvisorSvc.cpp \
    StorHvisorTx.cpp \
    StorHvisorTbl.cpp \
    StorHvisorRx.cpp \
    qos_ctrl.cpp \
    StorHvQosCtrl.cpp \
    StorHvVolumes.cpp \
    StorHvNative.cpp \
    FDSNStat.cpp

#user_no_style     :=
user_ar_libs      := fds-am-engine
fds-am-engine     := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
