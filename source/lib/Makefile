topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   := unit-test
user_cpp         :=               \
    Catalog.cpp                   \
    VolumeCatalog.cpp             \
    OMgrClient.cpp                \
	QoSWFQDispatcher.cpp	\
	qos_htb.cpp \
	qos_ctrl.cpp

#user_cc          :=               \
#    tvclib.c                      \
#    vvc_db.c                      \
#    vvclib.c                      \
#    vvc_private.c

# TODO: should change this directory and this lib to something more
# specific.
#
user_ar_libs     := fds-lib
fds-lib          := $(user_cpp)

include $(topdir)/Makefile.incl
