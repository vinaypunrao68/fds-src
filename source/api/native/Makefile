topdir            := ../..
user_target       := bin
user_rtime_env    := user

user_incl_dir     := $(topdir) $(topdir)/fds_client/include $(topdir)/lib

user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-am-engine \
    fds-StorHvisor \
    fds-lib \
    fds-umod \
    fds-util \
    fds-fdsp \
    fds-hash \
    fds-thread

user_non_fds_libs := \
    boost_log boost_system boost_thread boost_date_time boost_program_options boost_filesystem \
    leveldb thrift rt config++

user_cpp          := smoke.cpp

#user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := smoke
smoke	      := smoke.cpp
#serialize_unit_test := serialize_ut.cpp
#tt := testthrift.cpp


include $(topdir)/Makefile.incl
