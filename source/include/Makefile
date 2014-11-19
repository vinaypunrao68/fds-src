topdir         := ..
user_target    := lib
speedup        = -j $(threads)
user_rtime_env := user
user_build_dir := \
    fdsp \
    apis \
    net \
    blob \
    nginx-driver \
    concurrency \
    kvstore \
    fds-probe \
    hash \
    persistent-layer \
    shared \
    orch-mgr \
    access-mgr \
    util \
    leveldb \
    cache

user_hh        := $(wildcard *.h)
user_hpp       := $(wildcard *.h)
user_no_style  := \
	data_mgr.h            \
	fds_commons_rm.h      \
	fds_config.hpp        \
	fds_obj_cache.h       \
	odb.h                 \
	PerfTrace.h           \
	qos_ctrl.h            \
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
