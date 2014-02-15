topdir            := ../..
user_target       := lib
user_rtime_env    := user
user_build_dir    :=
user_incl_dir     := ../include $(topdir)
user_hh           := $(wildcard *.h)
user_cpp          := \
    OmResources.cpp \
    OmNodeResources.cpp \
    OmDeploy.cpp \
    OmClusMap.cpp \
    OmDataPlacement.cpp \
    OmPlacementAlgorithms.cpp \
	OmVolume.cpp

user_ar_libs      := fds-om-dlt
fds-om-dlt        := $(user_cpp)

include $(topdir)/Makefile.incl
