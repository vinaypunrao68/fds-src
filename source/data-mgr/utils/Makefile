topdir            := ../../
user_target       := exe
user_std_libs     := true
user_rtime_env    := user
speedup           = -j $(threads)
user_build_dir    :=

user_fds_ar_libs  := \
    fds-qos-lib \
    fds-dm-lib \
    fds-dsk-mgnt \
    fds-fdsp \
    fds-lib

user_non_fds_libs := \
    jemalloc \
    leveldb \
    ssl \
    crypto \
    pcrecpp \
    sqlite3 \
    boost_system \
    boost_program_options

# TODO: remove this once we have correct placement of header API.
#
user_incl_dir     := $(topdir) ../include
user_cpp_flags    :=
user_cpp          :=  $(wildcard *.cpp)
dmexplorer        := dmexplorer.cpp dmexplorer_driver.cpp

user_bin_exe      := \
    dmexplorer

include $(topdir)/Makefile.incl
