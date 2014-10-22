thrift_dir        := ../../..
user_taget        := exe
user_rtime_env    := user
user_incl_dir     :=
user_build_dir    :=
user_cc           := md5.c
user_cpp          := \
    thrifty.cc \
    thriftl.cc \
    main.cc \
    generate/t_generator.cc \
    generate/t_c_glib_generator.cc \
    generate/t_cpp_generator.cc \
    generate/t_java_generator.cc \
    generate/t_as3_generator.cc \
    generate/t_csharp_generator.cc \
    generate/t_py_generator.cc \
    generate/t_rb_generator.cc \
    generate/t_perl_generator.cc \
    generate/t_php_generator.cc \
    generate/t_erl_generator.cc \
    generate/t_cocoa_generator.cc \
    generate/t_st_generator.cc \
    generate/t_ocaml_generator.cc \
    generate/t_hs_generator.cc \
    generate/t_xsd_generator.cc \
    generate/t_html_generator.cc \
    generate/t_javame_generator.cc \
    generate/t_delphi_generator.cc \
    generate/t_go_generator.cc \
    generate/t_d_generator.cc \
    parse/parse.cc

user_non_fds_libs :=  fl
user_bin_exe      := thrift
thrift            := $(user_cc) $(user_cpp)

# Customize Makefile rules to build .lex and .yy
#
user_predep      := thrift_dir thrifty.cc thriftl.cc
user_clean       := thrifty.h thrifty.cc thriftl.cc

include $(thrift_dir)/Makefile.incl

thrift_dir:
	@mkdir -p $(comm_dir_objs)/generate
	@mkdir -p $(comm_dir_objs)/parse

thrifty.cc: thrifty.yy
	@echo "    [BISON]       $<"
	@$(yacc) -y -d $<
	@mv y.tab.h thrifty.h
	@mv y.tab.c thrifty.cc

thriftl.cc: thriftl.ll
	@echo "    [FLEX]        $<"
	@$(lex) $<
	@mv lex.yy.c thriftl.cc
