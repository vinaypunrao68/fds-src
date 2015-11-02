topdir           := ..
user_target      := lib
user_rtime_env   := user

# We don't use recursive generation to cut down
# on the number of files get touched during build
user_thrift      := \
	FDSP.thrift \
	common.thrift \
	svc_types.thrift \
	node_svc_api.thrift \
	am_types.thrift \
	dm_types.thrift \
	om_types.thrift \
	sm_types.thrift \
	pm_types.thrift \
	config_types.thrift \
	fds_stream.thrift \
	svc_api.thrift \
	pm_service.thrift \
	pm_data.thrift \
	am_api.thrift \
	dm_api.thrift \
	om_api.thrift \
	sm_api.thrift \
	config_api.thrift \
	xdi.thrift \
    health_monitoring_types.thrift \
    health_monitoring_api.thrift \
	event_api.thrift \
	event_types.thrift \
	filetransfer_api.thrift \
	svc_test.thrift

user_thrift_loc  := $(topdir)/include/fdsp
user_thrift_flag := --gen cpp:pure_enums --allow-64bit-consts

user_thrift_py_loc  := $(topdir)/test/fdslib/pyfdsp
user_thrift_py_flag := -gen py

user_clean       := \
    $(wildcard $(user_thrift_loc)/*.h) \
    $(wildcard $(user_thrift_loc)/*.cpp) \
    $(user_thrift_loc)

include $(topdir)/Makefile.incl
