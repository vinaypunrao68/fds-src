topdir           := ../..
speedup          = -j $(threads)
user_incl_dir    := $(topdir)/lib
user_predep      := update_buildstr
user_target      := lib
user_rtime_env   := user
user_python      := true
user_cpp         := $(wildcard *.cpp)

user_no_style    := \
    fds_request.cpp \
    fds_stat.cpp \
    PerfTrace.cpp

user_ar_libs      := fds-umod
user_so_libs      := fds-umod
user_non_fds_libs := fdsStatsUtil-debug statsclient
fds-umod          := $(user_cpp)

include $(topdir)/Makefile.incl

# -----------------------------------------------------------------------------
# The rule to generate build string content.
#
dynamics := $(wildcard $(topdir)/include/dynamic_*.h)
.PHONY: update_buildstr
update_buildstr:
ifeq (,$(findstring dynamic_vers.h,$(dynamics)))
	echo "Updating buildstr.cpp, the product version string container."
	touch buildstr.cpp
	git rev-parse --short HEAD | awk '{print "const char * versRev = \"" $$0"\";"}' > $(topdir)/include/dynamic_vers.h
	cat $(topdir)/../omnibus/VERSION | awk '{print "const char * versDate = \"" $$0"\";"}' >> $(topdir)/include/dynamic_vers.h
	uname -srvo | awk '{print "const char * machineArch = \"" $$0"\";"}' > $(topdir)/include/dynamic_machine_arch.h
else
ifdef UPDATEVERS
	echo "Forcing an update of buildstr.cpp, the product version string container."
	touch buildstr.cpp
	git rev-parse --short HEAD | awk '{print "const char * versRev = \"" $$0"\";"}' > $(topdir)/include/dynamic_vers.h
	cat $(topdir)/../omnibus/VERSION | awk '{print "const char * versDate = \"" $$0"\";"}' >> $(topdir)/include/dynamic_vers.h
	uname -srvo | awk '{print "const char * machineArch = \"" $$0"\";"}' > $(topdir)/include/dynamic_machine_arch.h
endif
endif

