topdir          := ../../../..
user_target     := lib
user_rtime_env  := user

local_nocpp     := $(wildcard *.skeleton.*)
user_cpp        := $(filter-out $(local_nocpp),$(wildcard *.cpp))
user_clean      := $(wildcard *.cpp) $(wildcard *.h)
user_no_style   := $(user_cpp) $(wildcard *.h)
user_ar_libs    := fds-probe-thrift-utest

fds-probe-thrift-utest := $(user_cpp)

include $(topdir)/Makefile.incl
