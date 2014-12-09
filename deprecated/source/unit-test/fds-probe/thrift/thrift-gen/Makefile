topdir          := ../../../..
user_target     := lib
user_rtime_env  := user
user_incl_dir   := $(topdir)/include/fdsp

local_nocpp     := $(wildcard *.skeleton.*)
user_cpp        := $(filter-out $(local_nocpp),$(wildcard *.cpp))
user_clean      := $(wildcard *.cpp) $(wildcard *.h)
user_no_style   := $(user_cpp) $(wildcard *.h)
user_ar_libs    := fds-probe-thrift-utest

fds-probe-thrift-utest := $(user_cpp)

include $(topdir)/Makefile.incl
