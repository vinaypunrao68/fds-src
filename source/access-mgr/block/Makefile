topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := \
    ../include \
    ../stor-hvisor \
		$(topdir) \
		/usr/include/cryptopp

user_cpp_flags    :=
user_cpp          := $(wildcard *.cpp)

user_ar_libs      := fds-am-engine
fds-am-engine     := $(user_cpp)

include $(topdir)/Makefile.incl
