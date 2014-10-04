topdir            := ../..
user_target       := test
user_rtime_env    := user
user_install      := python_bind_ut.py
user_install_loc  := test

user_python       := true
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-hash \
    fds-lib \
    fds-util \
    fds-fdsp \
    fds-thread \
    fds-odb \
    fds-umod \
    fds-lib \
    fds-util \
    fds-umod \
    fds-osdep

user_non_fds_libs := boost_log \
    boost_thread \
    boost_system \
   	boost_date_time \
    boost_program_options \
    config++ \
    thrift \
    pthread \
    dl \
    util \
    m \
    python2.7 \
    fiu \
    dl

user_cpp          := \
    ThreadPoolTest.cpp \
    conc_unit_test.cpp \
    fds_actor_ut.cpp \
    python_bind_ut.cpp

user_no_style     := $(user_cpp) $(user_cc)
user_bin_exe      := threadtest conc_test fds_actor_ut python_bind_ut
threadtest        := ThreadPoolTest.cpp
conc_test         := conc_unit_test.cpp
fds_actor_ut      := fds_actor_ut.cpp
python_bind_ut    := python_bind_ut.cpp

include $(topdir)/Makefile.incl
