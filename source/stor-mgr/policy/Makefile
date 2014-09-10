topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../smlib ../include $(topdir) $(topdir)/stor-mgr/migrations $(topdir)/lib

user_cpp         :=               \
    policy_rpc.cpp                \
    policy_tier.cpp

user_ar_libs     := fds-vpolicy
fds-vpolicy      := $(user_cpp)

include $(topdir)/Makefile.incl
