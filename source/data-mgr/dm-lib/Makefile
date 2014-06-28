topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    := dm-vol-cat

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../include
user_cpp_flags    := -DDMGR_LIVE_DEBUG
user_cpp          := \
	DataMgr.cpp \
	VolumeMeta.cpp \
	qos_ctrl.cpp   \
	CommitLog.cpp \
	DMSvcHandler.cpp \
        CatalogSync.cpp \
        CatalogSyncRecv.cpp

user_ar_libs      := fds-dm-lib
fds-dm-lib        := $(user_cpp)

include $(topdir)/Makefile.incl
