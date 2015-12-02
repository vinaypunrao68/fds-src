topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../include
user_cpp         := \
    endpoint.cpp \
    ep-map.cpp \
    ep-attr.cpp \
    net-plat-shared.cpp \
    net-service.cpp

user_so_libs     := fds-net-svc
fds-net-svc      := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
