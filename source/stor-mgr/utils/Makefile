topdir            := ../../
user_target       := exe 
user_std_libs     := true
user_rtime_env    := user
speedup           = -j $(threads)
user_incl_dir     := $(topdir) ../include $(topdir)/unit-test/stor-mgr /usr/include/cryptopp

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-smlib\
    fds-am-engine \
    fds-am-plat \
    fds-dsk-mgnt \
    fds-dsk-io \
    fds-odb \
    fds-fdsp

user_non_fds_libs := \
    boost_timer \
    leveldb \
    ssl \
    cryptopp \
    leveldb

user_ar_libs      := smchk
smchk             := smchk.cpp

user_cpp          := smchk.cpp
user_bin_exe      := smchk
smchk             := smchk.cpp smchk_driver.cpp

include $(topdir)/Makefile.incl
