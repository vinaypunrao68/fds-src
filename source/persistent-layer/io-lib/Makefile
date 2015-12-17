topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_build_dir   :=
user_incl_dir    := ../include
user_cpp         :=               \
    dm_read.cpp                   \
    dm_write.cpp                  \
    dm_io_lib.cpp                 \
    dm_index.cpp                  \
    tokFileMgr.cpp                  \
    pdata_request.cpp

user_no_style    := dm_read.cpp tokFileMgr.cpp
user_ar_libs     := fds-dsk-io
user_so_libs     := fds-dsk-io
fds-dsk-io       := $(user_cpp)

include $(topdir)/Makefile.incl
