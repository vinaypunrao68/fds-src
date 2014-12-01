topdir           := ../
user_std_libs    := true
user_target      := exe
user_rtime_env   := user
speedup          = -j $(threads)
user_build_dir   := include source python

user_incl_dir    := include ../net-service/include

user_fds_ar_libs :=                  \
                    platform_core    \
                    fds-net-svc      \
                    fds-xdi

user_non_fds_libs :=           \
                     udev      \
                     rt        \
                     parted

user_cpp         := platform-main.cpp
user_bin_exe     := platformd
platformd        := $(user_cpp)

include $(topdir)/Makefile.incl
