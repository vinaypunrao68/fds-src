probe_topdir      := ..

user_incl_dir     := \
    $(probe_topdir)/../../access-mgr/include

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  :=

user_cc           := ngx_modules.c
user_cpp          := s3-pm-probe.cpp
user_bin_exe      := s3-pm-probe
s3-pm-probe       := s3-pm-probe.cpp $(user_cc)

include $(probe_topdir)/Makefile.s3
