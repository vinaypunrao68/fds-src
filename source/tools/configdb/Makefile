topdir            := ../..
user_target       := exe
user_std_libs     := true
user_rtime_env    := user

user_incl_dir     := 
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-lib \
	fds-util \
	fds-fdsp \
	fds-umod \
	fds-thread \
	fds-plat-lib \
	fds-xdi	\
        fds-osdep

user_non_fds_libs := \
    cmdconsole

user_cpp          :=   configdbtool.cpp
user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := configdbtool
configdbtool      := configdbtool.cpp


include $(topdir)/Makefile.incl
