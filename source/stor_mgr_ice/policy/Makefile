topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../smlib ../include $(topdir)
user_cpp         :=               \
    policy_rpc.cpp                \
    policy_tier.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_ar_libs      := fds-vpolicy-ice
fds-vpolicy-ice   := $(user_cpp)

include $(topdir)/Makefile.incl
