topdir			  := ..
user_target		  := tool
user_std_libs     := true
user_rtime_env	  := user

user_incl_dir     := $(topdir) \
    $(topdir)/platform/include \
    $(topdir)/net-service/include \
    $(topdir)/data-mgr/include

user_cpp_flags    := -DLEVELDB_PLATFORM_POSIX
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-dm-lib \
    fds-xdi

user_non_fds_libs := \
    leveldb

user_hh           := $(wildcard *.h)
user_cpp          := \
    shm_dump.cpp \
    journal_dump.cpp \
    ldb-tool.cpp

user_no_style     := \
    journal_dump.cpp

user_bin_exe      := shm_dump journal-dump ldb-tool
shm_dump          := shm_dump.cpp
journal-dump      := journal_dump.cpp
ldb-tool          := ldb-tool.cpp

include $(topdir)/Makefile.incl
