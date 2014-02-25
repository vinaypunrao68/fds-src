topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    :=
user_ragel       := am-urls.rl
user_clean       := am-urls.cpp
user_cpp         :=               \
    am-engine.cpp \
    am-plugin.cpp \
    s3connector.cpp \
    atmos-connector.cpp \
    http_utils.cpp \
    am-urls.cpp

user_no_style    := am-urls.cpp
user_ar_libs     := fds-am-engine
fds-am-engine    := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
