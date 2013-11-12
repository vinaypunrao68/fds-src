topdir           := ..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := $(topdir)/include/net-proxies
user_rpc_hdr_dir := $(topdir)/include/net-proxies
user_rpc         := iomsg.x

user_cc          :=               \
    iomsg_clnt.c                  \
    iomsg_serv.c                  \
    iomsg_xdr.c                   \

user_cpp         :=               \
    vol_policy_clnt.cpp           \
    vol_policy_serv.cpp           \
    vol_io_clnt.cpp               \
    vol_io_serv.cpp

user_ar_libs     := fds-netproxy
fds-netproxy     := $(user_cpp)

include $(topdir)/Makefile.incl
