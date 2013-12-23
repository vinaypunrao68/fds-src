topdir          := ../..
user_target     := lib
user_rtime_env  := user

user_cpp        := \
    Calculator.cpp \
    SharedService.cpp \
    tutorial_constants.cpp \
    tutorial_types.cpp \
    shared_constants.cpp \
    shared_types.cpp

user_clean      := \
    $(user_cpp) \
    Calculator.h \
    SharedService.h \
    tutorial_constants.h \
    tutorial_types.h \
    shared_constants.h \
    shared_types.h \
    $(wildcard *.skeleton.*)

user_no_style   := $(user_cpp)
user_ar_libs    := fds-thrift
fds-thrift      := $(user_cpp)

include $(topdir)/Makefile.incl
