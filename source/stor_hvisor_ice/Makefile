topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    :=

# TODO: remove the topdir path in the include search
#
user_incl_dir     := $(topdir) $(topdir)/fds_client/include $(topdir)/lib $(topdir)/fds_client/blk_dev/tools
user_fds_ar_libs  := fds-umod 
StorHvisor_cpp    :=                   \
    StorHvisorNet.cpp                  \
    RPC_EndPoint.cpp                   \
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

user_no_style      := $(user_cpp) $(user_cc) $(wildcard *.h)
user_ar_libs       := fds-StorHvisor-ice
fds-StorHvisor-ice := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
