topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   := blob net concurrency kvstore crypto native
user_cpp         :=      \
    dlt.cpp \
    FdsRandom.cpp        \
    NetSession.cpp       \
    PerfTrace.cpp        \
    Catalog.cpp          \
    VolumeCatalog.cpp    \
    fds_timer.cpp        \
    LocalClusterMap.cpp  \
    OMgrClient.cpp       \
    QoSWFQDispatcher.cpp \
    qos_htb.cpp          \
    fds_obj_cache.cpp	 \
    qos_min_prio.cpp     \
    PerfStats.cpp        \
    jsoncpp.cpp          \
    fds_process.cpp      \
    serialize.cpp        \
    fds_counters.cpp     \
    fds_types.cpp        \
    ClusterCommMgr.cpp   \
    fds_volume.cpp       \
    TransJournal.cpp     \
    dmt.cpp              \
    fds_error.cpp        \
    PerfHistory.cpp      \
    StatsCollector.cpp   \
    ObjectLogger.cpp

user_incl_dir    := $(topdir)
user_no_style    := \
    PerfTrace.cpp        \
    VolumeCatalog.cpp    \
    OMgrClient.cpp       \
    QoSWFQDispatcher.cpp \
    qos_htb.cpp          \
    fds_obj_cache.cpp	 \
    qos_min_prio.cpp     \
    TransJournal.cpp     \
    PerfStats.cpp        \
    jsoncpp.cpp          \
    serialize.cpp        \
    fds_counters.cpp     \
    $(wildcard *.h)

# TODO: should change this directory and this lib to something more
# specific.
#
user_ar_libs     := fds-lib
fds-lib          := $(user_cpp)

include $(topdir)/Makefile.incl
