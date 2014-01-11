topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../include
user_cpp         := \
    template_probe.cpp \
    pm_probe.cpp \
    sm_probe.cpp

user_ar_libs       := fds-probe-adapters
fds-probe-adapters := $(filter-out template_probe.cpp, $(user_cpp))

include $(topdir)/Makefile.incl
