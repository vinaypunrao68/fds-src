topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_python      := true
user_cpp         := \
    buildstr.cpp \
    fds_resource.cpp \
    fds_request.cpp \
    fds_module.cpp \
    fds_stat.cpp \
    fds-shmobj.cpp \
    fds_python_bind.cpp \
    fdsp_utils.cpp \
    fds-fsm.cpp \
    fds-aio.cpp \
    fds_process.cpp \
    PerfTrace.cpp \
    Tracebuffer.cpp

user_no_style    := \
    fds_request.cpp \
    fds_stat.cpp \
    PerfTrace.cpp

user_ar_libs     := fds-umod
fds-umod         := $(user_cpp)

include $(topdir)/Makefile.incl

all: update_buildstr

update_buildstr:
	touch buildstr.cpp
