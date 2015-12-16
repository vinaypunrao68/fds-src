topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    := object-store

# TODO: avoid include across components, put in include dir instead.
#
user_incl_dir     := \
    $(topdir) \
	../include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib \
	$(topdir)/persistent-layer/include

user_cpp_flags    := -DOBJCACHE_ENABLE
user_cpp          := $(wildcard *.cpp)

user_ar_libs      := fds-smlib
user_so_libs      := fds-smlib
fds-smlib         := $(user_cpp)

include $(topdir)/Makefile.incl
