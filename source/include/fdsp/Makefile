topdir           := ../..
user_target      := lib
user_rtime_env   := user

#local_ice        := FDSP.cpp orch_proto.cpp
local_nocpp      := $(wildcard *.skeleton.*)
user_cpp         := $(filter-out $(local_nocpp),$(wildcard *.cpp))
user_no_style    := $(user_cpp) $(wildcard *.h)
user_ar_libs     := fds-fdsp #fds-fdsp-ice
fds-fdsp         := $(filter-out $(local_ice),$(user_cpp))
#fds-fdsp-ice     := $(local_ice)

include $(topdir)/Makefile.incl

clean:
	rm -f *.h *.cpp
