topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cpp         :=      \
    config.cpp           \
    dlt.cpp              \
    FdsRandom.cpp        \
    Catalog.cpp          \
    VolumeCatalog.cpp    \
    fds_timer.cpp        \
    QoSWFQDispatcher.cpp \
    qos_htb.cpp          \
    fds_obj_cache.cpp	 \
    qos_min_prio.cpp     \
    jsoncpp.cpp          \
    serialize.cpp        \
    fds_counters.cpp     \
    fds_types.cpp        \
    fds_volume.cpp       \
    dmt.cpp              \
    fds_error.cpp        \
    PerfHistory.cpp      \
    StatsCollector.cpp   \
    ObjectLogger.cpp 	 \
	Histogram.cpp 	     \
    S3Client.cpp         \
	BitSet.cpp           \
	disk_utils.cpp		 \
	MigrationUtility.cpp \
	fds_subscription.cpp \
	fds_ldomain.cpp

user_incl_dir    := .. $(topdir)
user_no_style    := \
    VolumeCatalog.cpp    \
    qos_htb.cpp          \
    fds_obj_cache.cpp	 \
    qos_min_prio.cpp     \
    jsoncpp.cpp          \
    serialize.cpp        \
    fds_counters.cpp     \
    $(wildcard *.h)

# TODO: should change this directory and this lib to something more
# specific.
#
user_so_libs     := fds-lib
fds-lib          := $(user_cpp)

include $(topdir)/Makefile.incl
