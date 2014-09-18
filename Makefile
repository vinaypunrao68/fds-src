# XXX make sure enough space
ifeq ($(wildcard ~/.ihaveaswapfile),)
	aaa := $(shell [ ! -f /swapfile ] && sudo dd if=/dev/zero of=/swapfile bs=1024000 count=6k)
	bbb := $(shell sudo mkswap /swapfile && sudo swapon /swapfile)
endif

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
    cmdconsole

ifndef JENKINS_URL 
    user_build_dir += source
endif

include $(topdir)/Makefile.incl
