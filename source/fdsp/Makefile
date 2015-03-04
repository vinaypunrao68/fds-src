topdir           := ..
user_target      := lib
user_rtime_env   := user

# We don't use recursive generation to cut down
# on the number of files get touched during build
user_thrift      := \
	FDSP.thrift \
	common.thrift \
	fds_stream.thrift \
	fds_service.thrift \
	snapshot.thrift \
	pm_service.thrift \
	dm_service.thrift \
	om_service.thrift \
	sm_service.thrift \
	am_service.thrift \
	configuration_service.thrift \
	xdi.thrift

user_thrift_loc  := $(topdir)/include/fdsp
user_thrift_flag := --gen cpp:pure_enums --allow-64bit-consts

user_thrift_py_loc  := $(topdir)/test/fdslib/pyfdsp
user_thrift_py_flag := -gen py

user_clean       := \
    $(wildcard $(user_thrift_loc)/*.h) \
    $(wildcard $(user_thrift_loc)/*.cpp) \
    $(user_thrift_loc)

include $(topdir)/Makefile.incl
