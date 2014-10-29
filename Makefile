topdir         := .
user_ext_build := true

user_build_dir := \
    thrift-0.9.0 \
    leveldb-1.15.0 \
    banana \
    nginx \
    jansson-2.5 \
    hiredis \
    cryptopp \
    gmock-1.7.0 \
    cmdconsole

ifndef JENKINS_URL 
    user_build_dir += source
endif

include $(topdir)/Makefile.incl
