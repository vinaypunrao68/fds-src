topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := $(topdir) . \
        $(topdir)/platform/include \
	    $(topdir)/net-service/include \
        $(topdir)/util/volume-checker/include
user_cpp         := \
	Deployer.cpp \
	SvcMsgFactory.cpp \
	TestUtils.cpp \
	TestFixtures.cpp \
	test-plat-proc.cpp

user_fds_ar_libs := fds-volume-checker

user_ar_libs     := fds-test
fds-test  	 := $(user_cpp)

include $(topdir)/Makefile.incl
