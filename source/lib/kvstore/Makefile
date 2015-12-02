topdir           := ../..
user_target      := lib
user_rtime_env   := user
user_cpp         :=  $(wildcard *.cpp)
user_so_libs     := fds-lib
user_non_fds_libs:= hiredis thrift rt
fds-lib          := $(user_cpp)

user_no_style    := configdb.cpp

include $(topdir)/Makefile.incl
