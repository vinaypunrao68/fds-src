topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_incl_dir    := .. $(topdir) \
    $(topdir)/data-mgr/include
user_cpp_flags    := -DLEVELDB_PLATFORM_POSIX
user_cpp         := \
    copy_env.cpp \
    cat_journal.cpp

user_no_style    := \
    copy_env.cpp

# TODO: should change this directory and this lib to something more
# specific.
#
user_ar_libs     := fds-lib
user_so_libs     := fds-leveldb
fds-lib          := $(user_cpp)
fds-leveldb      := $(user_cpp)

include $(topdir)/Makefile.incl
