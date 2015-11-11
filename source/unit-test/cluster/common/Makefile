topdir            := ../../..
user_target       := lib
user_rtime_env    := user

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) .
user_cpp_flags    :=
#user_cpp          := $(wildcard *.cpp)
user_cpp          := volumegroup_extensions.cpp \
	VolumeGroupHandle.cpp \
	Volume.cpp \
	Dm.cpp \
	Om.cpp \
	Am.cpp

user_ar_libs      := dm-clus-lib
dm-clus-lib       := $(user_cpp)

include $(topdir)/Makefile.incl
