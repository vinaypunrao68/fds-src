thrift_dir        := ../../..
user_taget        := exe
user_rtime_env    := user
user_incl_dir     :=
user_build_dir    := generate parse
user_cc           := md5.c
user_cpp          := \
    thrifty.cc \
    thriftl.cc \
    main.cc

user_non_fds_libs :=  \
    thrift-genlib thrift-parse fl

user_bin_exe      := thrift
thrift            := $(user_cc) $(user_cpp)

# Customize Makefile rules to build .lex and .yy
#
user_predep      := thrifty.cc thriftl.cc
user_clean       := thrifty.h thrifty.cc thriftl.cc

include $(thrift_dir)/Makefile.incl

thrifty.cc: thrifty.yy
	@echo "    [BISON]       $<"
	@$(yacc) -y -d $<
	@mv y.tab.h thrifty.h
	@mv y.tab.c thrifty.cc

thriftl.cc: thriftl.ll
	@echo "    [FLEX]        $<"
	@$(lex) $<
	@mv lex.yy.c thriftl.cc
