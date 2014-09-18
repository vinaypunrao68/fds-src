topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := \
	../include /usr/include/fuse

user_cpp         :=               \
    fds-probe.cpp \
    js-object.cpp \
    utest-types.cpp

user_ar_libs     := fds-probe
fds-probe        := $(user_cpp)

include $(topdir)/Makefile.incl
