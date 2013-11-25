topdir            := ..
user_target       := exe
user_rtime_env    := user
user_build_dir    :=

# TODO: remove the topdir path in the include search
#
user_incl_dir     := $(topdir) $(topdir)/fds_client/include $(topdir)/lib
user_fds_ar_libs  := fds-umod 
StorHvisor_cpp    :=                   \
    StorHvisorNet.cpp                  \
    RPC_EndPoint.cpp                   \
    StorHvisorTx.cpp                   \
    StorHvisorTbl.cpp                  \
    StorHvisorRx.cpp                   \
    VolumeCatalogCache.cpp             \
    StorHvJournal.cpp                  \
    qos_ctrl.cpp	               \
    StorHvQosCtrl.cpp                  \
    StorHvVolumes.cpp		       \
    StorHvNative.cpp	

user_cpp          :=                   \
    $(StorHvisor_cpp)

user_ar_libs      := fds-StorHvisor
fds-StorHvisor    := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
