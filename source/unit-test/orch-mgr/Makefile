test_topdir            := ..
user_fds_so_libs  :=
user_fds_ar_libs  := \
	fds-svc-request \
	fds-lib \
	fds-thread \
	fds-util \
	fds-fdsp \
	fds-umod \
    fds-osdep \
        fds-om-common \
        fds-om-lib \
        fds-om-dlt

user_non_fds_libs :=                                                          \
    boost_log \
	boost_system \
	boost_thread \
	boost_date_time \
	boost_program_options \
    leveldb \
	hiredis \
	thrift \
	rt config++ fiu dl ical

user_cpp          :=                   \
    place_tbl_unit_test.cpp            \
    configservice_t.cpp                \
    dmt_calculation_gtest.cpp          \
    VolumeGroupingOm_gtest.cpp         \
    dlt_calculation_gtest.cpp

user_no_style     := $(user_cc)
user_bin_exe      :=       \
    place_tbl_test         \
    configservice_gtest    \
    dmt_calc_gtest         \
    volume_group_omgtest   \
    dlt_calc_gtest

configservice_gtest := configservice_t.cpp
place_tbl_test      := place_tbl_unit_test.cpp
dmt_calc_gtest      := dmt_calculation_gtest.cpp
dlt_calc_gtest      := dlt_calculation_gtest.cpp
volume_group_omgtest := VolumeGroupingOm_gtest.cpp

include $(test_topdir)/Makefile.om
