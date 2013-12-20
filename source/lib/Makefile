topdir           := ..
user_target      := lib
user_rtime_env   := user
user_cpp         :=               \
    PerfTrace.cpp        \
    Catalog.cpp                   \
    VolumeCatalog.cpp             \
    OMgrClient.cpp                \
	QoSWFQDispatcher.cpp	\
	qos_htb.cpp  \
	fds_obj_cache.cpp	\
	qos_min_prio.cpp        \
	TransJournal.cpp        \
        PerfStats.cpp        \
	jsoncpp.cpp             \

#user_cc          :=               \
#    tvclib.c                      \
#    vvc_db.c                      \
#    vvclib.c                      \
#    vvc_private.c

user_incl_dir    := $(topdir)
user_no_style    := $(user_cpp)

# TODO: should change this directory and this lib to something more
# specific.
#
user_ar_libs     := fds-lib fds-thread 
fds-lib          := $(user_cpp)

include $(topdir)/Makefile.incl
