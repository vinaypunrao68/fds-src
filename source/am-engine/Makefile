topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    :=
user_cpp         :=               \
    am-engine.cpp \
    am-plugin.cpp \
    s3connector.cpp \
    http_utils.cpp

user_no_style    := $(wildcard *.h)
user_ar_libs     := fds-am-engine
fds-am-engine    := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
