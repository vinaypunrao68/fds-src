topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := $(topdir) . /usr/include/cryptopp
user_cpp         := \
	Deployer.cpp \
	SvcMsgFactory.cpp \
	TestUtils.cpp \
	TestFixtures.cpp

user_ar_libs     := fds-test
fds-test  	 := $(user_cpp)

include $(topdir)/Makefile.incl
