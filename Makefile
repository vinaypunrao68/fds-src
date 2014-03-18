topdir         := .
user_ext_build := true

user_build_dir := \
    thrift-0.9.0 \
    leveldb-1.12.0 \
    nginx \
    libconfig-1.4.9 \
    jansson-2.5 \
    hiredis \
    cryptopp \
    gmock-1.7.0 \
    cmdconsole \
    source

include $(topdir)/Makefile.incl
