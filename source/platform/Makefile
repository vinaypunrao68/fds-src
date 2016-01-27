topdir           := ../
user_std_libs    := true
user_target      := exe
user_rtime_env   := user
speedup          = -j $(threads)
user_build_dir   := include source python scripts

user_incl_dir    := include $(topdir)

user_fds_ar_libs := fds-pm-lib fds-disk-lib
user_non_fds_libs :=           \
                     udev      \
                     rt        \
                     uuid      \
                     parted

user_cpp         := platform-main.cpp
user_bin_exe     := platformd
platformd        := $(user_cpp)

include $(topdir)/Makefile.incl
