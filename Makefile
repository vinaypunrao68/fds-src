topdir         := .
user_ext_build := true

user_build     := \
    Ice-3.5.0 leveldb-1.12.0 source

incl_scripts   := $(topdir)/source/Build/mk-scripts
include $(incl_scripts)/Makefile.start
