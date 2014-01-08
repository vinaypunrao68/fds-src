topdir         := .
user_ext_build := true

user_build_dir := \
    Ice-3.5.0/cpp \
    thrift-0.9.0 \
    leveldb-1.12.0 \
    nginx \
    libconfig-1.4.9 \
    jansson-2.5 \
    source

include $(topdir)/Makefile.incl
