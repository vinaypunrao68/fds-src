topdir           := ../..
user_target      := lib
user_rtime_env   := user

local_nocpp      := $(wildcard *.skeleton.*)
user_cpp         := $(filter-out $(local_nocpp),$(wildcard *.cpp))
user_no_style    := $(user_cpp) $(wildcard *.h)
user_ar_libs     := fds-fdsp
user_so_libs     := fds-fdsp
fds-fdsp         := $(filter-out $(local_ice),$(user_cpp))

include $(topdir)/Makefile.incl
