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
    StorHvCtrl.cpp                     \
    StorHvQosCtrl.cpp                  \
    StorHvVolumes.cpp

user_cpp       := \
    $(StorHvisor_cpp)
user_hh        := $(wildcard *.h)
user_hpp       := $(wildcard *.h)

user_no_style     := $(user_cc) \
    StorHvDataPlace.h \
    StorHvQosCtrl.h \
    StorHvVolumes.h \
    StorHvCtrl.cpp    \
    StorHvQosCtrl.cpp \
    StorHvVolumes.cpp

#user_no_style     :=
user_ar_libs      := fds-am-engine
fds-am-engine     := $(StorHvisor_cpp)

include $(topdir)/Makefile.incl
