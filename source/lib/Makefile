topdir           := ..
user_target      := lib
user_rtime_env   := user
user_cpp         :=      \
    dlt.cpp \
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
    NetSession.cpp       \
    serialize.cpp        \
    fds_counters.cpp     \
    fds_types.cpp        \
    ClusterCommMgr.cpp   \
    TransJournal.cpp    

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
    $(wildcard *.h)

# TODO: should change this directory and this lib to something more
# specific.
#
user_ar_libs     := fds-lib fds-thread 
fds-lib          := $(user_cpp)

include $(topdir)/Makefile.incl
