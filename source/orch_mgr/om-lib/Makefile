topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    := 
user_install      :=

user_fds_so_libs  :=
user_fds_ar_libs  :=
user_non_fds_libs :=
user_incl_dir     := $(topdir) ../include

orch_mgr_cpp      := $(wildcard *.cpp)
user_hh           := $(wildcard *.h)
user_cpp          := $(orch_mgr_cpp)

user_ar_libs      := fds-om-lib
fds-om-lib        := $(orch_mgr_cpp)

include $(topdir)/Makefile.incl
