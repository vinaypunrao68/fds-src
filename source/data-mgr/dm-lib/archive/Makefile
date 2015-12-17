topdir            := ../../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_install_loc  := exe
user_install      := archivehelper.py

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../../include
user_cpp_flags    :=
user_cpp          := \
	ArchiveClient.cpp

user_ar_libs      := fds-dm-lib
user_so_libs      := fds-dm-archive
fds-dm-lib	      := $(user_cpp)
fds-dm-archive    := $(user_cpp)

include $(topdir)/Makefile.incl
