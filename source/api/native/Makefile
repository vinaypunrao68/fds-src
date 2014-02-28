topdir            := ../..
user_target       := bin
user_rtime_env    := user

user_incl_dir     := $(topdir) \
	$(topdir)/fds_client/include \
	$(topdir)/lib \
	$(topdir)/stor-hvisor \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

user_fds_so_libs  :=

user_fds_ar_libs  := \
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

user_ld_flags := -L$(JAVA_HOME)/jre/lib/amd64 -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm
#user_ld_flags := -shared

user_cpp          := JavaContext.cpp fds_java_bindings.cpp 

user_no_style     := $(user_cpp) 
user_bin_exe      := fds_java_bindings
fds_java_bindings	      :=  $(user_cpp) #fds_java_bindings.cpp


include $(topdir)/Makefile.incl

jni_headers: 
	javah -cp ../target/classes com.formationds.nativeapi.NativeApi

