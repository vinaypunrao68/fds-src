test_topdir       := ..
source_dir       := $(test_topdir)/..
user_build_dir    :=
user_incl_dir     := \
	$(source_dir)/orch-mgr/include	
user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-util \
    fds-net-svc \
    fds-svc-request \
    fds-om-common \
    fds-om-lib \
    fds-om-dlt \
    fds-om-discovery \
    fds-om-plat \
    fds-plat-lib \
    fds-xdi

user_non_fds_libs := \
    leveldb \
    ical \
    hiredis
user_ld_flags     := -shared

user_cpp          := \
    OMSvcProcess.cpp \
    TestSvcProcess.cpp

user_no_style     :=

user_bin_exe      := \
	omsvc \
	testsvc

omsvc := OMSvcProcess.cpp 
testsvc := TestSvcProcess.cpp

include $(test_topdir)/Makefile.svc
