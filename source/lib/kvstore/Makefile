topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cpp         :=  $(wildcard *.cpp)
user_incl_dir    := $(topdir)/../hiredis
user_ar_libs     := fds-lib
user_non_fds_libs:= hiredis thrift rt
fds-lib          := $(user_cpp)

user_no_style    :=  $(wildcard *.cpp)  $(wildcard *.h)

include $(topdir)/Makefile.incl
