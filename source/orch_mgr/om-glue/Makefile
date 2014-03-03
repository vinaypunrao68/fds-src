topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := ../include \
	$(JAVA_HOME)/include \
	$(JAVA_HOME)/include/linux

user_hh           := $(wildcard *.h)
user_cpp          := $(wildcard *.cpp)
user_no_style     := $(wildcard com_*)

user_ld_flags     := -L$(JAVA_HOME)/jre/lib/amd64 -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm 
user_so_libs      := fds-om-glue
user_ar_libs      := fds-om-glue

fds-om-glue       := $(user_cpp)

include $(topdir)/Makefile.incl
