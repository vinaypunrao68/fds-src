topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cpp         := $(wildcard *.cpp)

user_cpp_flags   := -Wall -Werror -Wno-reorder
user_incl_dir    := $(topdir)
user_ar_libs     := fds-lib
fds-lib          := $(user_cpp)


include $(topdir)/Makefile.incl
