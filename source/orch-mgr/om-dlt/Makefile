topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := ../include $(topdir)
user_hh           := $(wildcard *.h)
user_cpp          := $(wildcard *.cpp)
user_cpp_flags    := -DFUSION_MAX_VECTOR_SIZE=20
user_ar_libs      := fds-om-dlt
fds-om-dlt        := $(user_cpp)

include $(topdir)/Makefile.incl
