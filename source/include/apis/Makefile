topdir           := ../..
user_target      := lib
user_rtime_env   := user

local_nocpp      := $(wildcard *.skeleton.*)
user_cpp         := $(filter-out $(local_nocpp),$(wildcard *.cpp))
user_no_style    := $(user_cpp) $(wildcard *.h)
user_ar_libs     := fds-xdi
fds-xdi          := $(user_cpp)

include $(topdir)/Makefile.incl

clean:
	rm *.h *.cpp
