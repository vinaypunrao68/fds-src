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
    fds-plat-lib

user_non_fds_libs := \
    leveldb \
    ical \
    hiredis
#user_ld_flags     := -shared

user_cpp          := \
    TestSvcProcess.cpp \
    OMSvcProcess.cpp \
    SvcRequestMgr_gtest.cpp \
    SvcServer_gtest.cpp \
    SvcMgr_gtest.cpp \
    SvcMapChecker.cpp \
    VolumeGrouping_gtest.cpp


user_no_style     :=

user_bin_exe      := \
	testsvc \
	omsvc \
	svcrequestmgr_gtest \
	svcserver_gtest \
	svcmgr_gtest \
	svcmapchecker \
	volumegrouping_gtest 

omsvc := OMSvcProcess.cpp 
testsvc := TestSvcProcess.cpp
svcrequestmgr_gtest := SvcRequestMgr_gtest.cpp
svcserver_gtest := SvcServer_gtest.cpp
svcmgr_gtest := SvcMgr_gtest.cpp
svcmapchecker := SvcMapChecker.cpp
volumegrouping_gtest := VolumeGrouping_gtest.cpp

include $(test_topdir)/Makefile.svc
