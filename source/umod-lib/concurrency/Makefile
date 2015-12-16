topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cpp         := \
    ThreadPool.cpp \
    Synchronization.cpp \
    Mutex.cpp \
    fds_actor_request.cpp \
    fds_actor.cpp

user_ar_libs     := fds-thread
user_so_libs     := fds-thread
fds-thread       := $(user_cpp)

include $(topdir)/Makefile.incl
