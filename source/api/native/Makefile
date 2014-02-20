topdir            := ../..
user_target       := bin
user_rtime_env    := user

java_home = "/usr/java/jdk/"

user_incl_dir     := $(topdir) \
	$(topdir)/fds_client/include \
	$(topdir)/lib \
	$(topdir)/stor-hvisor \
	$(java_home)/include \
	$(java_home)/include/linux

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

user_ld_flags := -shared 

user_cpp          := fds_java_bindings.cpp

user_no_style     := $(user_cpp) 
user_bin_exe      := fds_java_bindings
fds_java_bindings	      := fds_java_bindings.cpp


include $(topdir)/Makefile.incl

jni_headers: 
	javah -cp ../target/classes com.formationds.nativeapi.NativeApi


