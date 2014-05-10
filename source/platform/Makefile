topdir           := ../
user_std_libs    := true
user_target      := exe
user_rtime_env   := user
user_build_dir   := include shared common disk python

user_incl_dir    := include
user_fds_ar_libs := \
    fds-plat-daemon \
	fds-plat-disk

user_non_fds_libs := \
	udev \
	parted

user_cpp         := platform-main.cpp
user_bin_exe     := platformd
platformd        := $(user_cpp)

include $(topdir)/Makefile.incl
