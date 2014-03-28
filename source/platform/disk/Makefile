topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_python_bind := true
user_incl_dir    := ../include

user_cpp         := \
    disk-capacity.cpp \
    disk-label.cpp \
	disk-partition.cpp \
    disk-inventory.cpp

user_ar_libs     := fds-plat-disk
fds-plat-disk    := $(user_cpp)

include $(topdir)/Makefile.incl
