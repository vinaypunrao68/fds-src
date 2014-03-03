topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    := 
user_install      :=

user_fds_so_libs  :=
user_fds_ar_libs  :=
user_non_fds_libs :=
user_incl_dir     := ../include
user_hh           := $(wildcard *.h)
user_cpp          := \
	om-discovery.cpp \
	om-vol-discovery.cpp

user_ar_libs      := fds-om-discovery
fds-om-discovery  := $(user_cpp)

include $(topdir)/Makefile.incl
