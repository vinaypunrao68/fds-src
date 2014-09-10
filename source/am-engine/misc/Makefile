topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_incl_dir    := ../include  $(topdir)/stor-hvisor $(topdir)/fds_client $(topdir)/fds_client/include $(topdir)
user_ragel       := am-urls.rl
user_clean       := am-urls.cpp
user_cpp         :=               \
    am-engine.cpp \
    am-plugin.cpp \
    s3connector.cpp \
    atmos-connector.cpp \
    http_utils.cpp \
    am-urls.cpp \
    fdsn-server.cpp \
    am-probe.cpp

user_no_style    := am-urls.cpp
user_ar_libs     := fds-am-engine
fds-am-engine    := $(user_cpp) $(user_cc)

include $(topdir)/Makefile.incl
