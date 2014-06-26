probe_topdir      := ..

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine \
    fds-dsk-io \
    fds-dsk-mgnt

user_hh           := $(wildcard *.h)
user_cpp          := \
    mod-main.cpp \
    template_probe.cpp

user_bin_exe      := example-blk
example-blk       := $(user_cpp)

include $(probe_topdir)/Makefile.blk
