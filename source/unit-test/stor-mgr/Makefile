topdir            := ../..
user_target       := test
user_std_libs     := true
user_rtime_env    := user
user_install_loc  := test

user_incl_dir     := \
	$(topdir) \
	$(topdir)/stor-mgr/include \
	$(topdir)/stor-mgr/migrations \
	$(topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib     \
    fds-am-engine \
    fds-dsk-io

user_cpp          := \
	object_store_unit_test.cpp

user_no_style     := sm_unit_test.cpp
user_bin_exe      := \
    ObjectStoreUnitTest

ObjectStoreUnitTest := object_store_unit_test.cpp

include $(topdir)/Makefile.incl
