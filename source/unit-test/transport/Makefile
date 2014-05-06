topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := 
user_install_loc  := test

user_incl_dir     := \
	$(topdir) \
	$(topdir)/lib

user_cpp_flags    :=
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-migrations\
    fds-smlib\
    fds-StorHvisor \
    fds-sm-plat \
    fds-plat-lib \
    fds-lib \
    fds-dsk-mgnt \
    fds-umod \
    fds-dsk-io \
    fds-thread \
    fds-util \
    fds-odb \
    fds-vpolicy \
    fds-fdsp \
    fds-hash \
    fds-osdep

user_non_fds_libs := \
    boost_log \
    boost_system \
    boost_filesystem \
    boost_thread \
    boost_date_time \
    boost_program_options \
    leveldb \
    ssl \
    cryptopp \
    crypto \
    leveldb \
    fuse \
    config++ \
    thrift \
    hiredis \
    gmock \
    pthread

user_cpp          := \
	RpcRequest.cpp \
	RpcRequest_ut.cpp \
	RpcRequestPool.cpp 

user_no_style     := RpcRequest_ut.cpp

user_bin_exe      := \
	RpcRequest_ut 

RpcRequest_ut := RpcRequest.cpp RpcRequestPool.cpp RpcRequest_ut.cpp

include $(topdir)/Makefile.incl
