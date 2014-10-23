topdir           := ../
include $(topdir)/Makefile.cpus
user_std_libs    := true
user_target      := exe
user_rtime_env   := user
speedup          := -j $(parallel_build_count)
user_build_dir   := include shared common disk net-svc python

user_incl_dir    := include ../net-service/include
user_fds_ar_libs := \
    fds-plat-daemon \
    fds-plat-disk \
    fds-net-svc \
    fds-xdi

user_non_fds_libs := \
	udev \
	rt \
	parted

user_cpp         := platform-main.cpp
user_bin_exe     := platformd
platformd        := $(user_cpp)

include $(topdir)/Makefile.incl
