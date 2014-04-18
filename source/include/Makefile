topdir         := ..
user_target    := lib
user_rtime_env := user
user_build_dir := \
    fdsp \
    xdi \
    am-engine \
    concurrency \
    kvstore \
    fds-probe \
    hash \
    persistent_layer \
    shared \
    orch-mgr \
    util

user_hh        := $(wildcard *.h)
user_hpp       := $(wildcard *.h)
user_no_style  := \
	data_mgr.h            \
	fds_commons_rm.h      \
	fds_config.hpp        \
	fds_err.h             \
	fds_obj_cache.h       \
	fds_placement_table.h \
	fds_qos.h             \
	fds_volume.h          \
	native_api.h          \
	odb.h                 \
	PerfTrace.h           \
	qos_ctrl.h            \
	rdtsc.h               \
	TransJournal.h        \
	tvclib.h              \
	vvclib.h              \
	fds_timer.h           \
	NetSessRespSvr.h      \
	NetSessRespClient.h  \
	NetSession.h        \
    fds_counters.h	    \
	fds_stopwatch.h     \
	graphite_client.h   \
	debughelpers.h      \
	net/socket.h        \
	serialize.h

include $(topdir)/Makefile.incl
