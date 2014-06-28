topdir            := ../../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../../include ../../include/dm-vol-cat
user_cpp_flags    :=
user_cpp          := \
	DmVolumeCatalog.cpp \
	DmVcCache.cpp \
	DmPersistVc.cpp

user_ar_libs      := fds-dm-volcat
fds-dm-volcat     := $(user_cpp)

include $(topdir)/Makefile.incl
