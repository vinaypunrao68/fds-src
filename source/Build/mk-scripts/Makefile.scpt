# Hello emacs, this is a -*-Makefile-*-
#
# Makefile.scpt
# -------------
# Shared scripts for the Makefile system.
#

# ----------------------------------------------------------------------------
# Script do subdir build
#
scpt_build_subdir =                                                          \
    for f in $(user_build_dir); do                                           \
        echo Making $(MAKECMDGOALS) in `pwd`/$$f;                            \
        makearg="$(MAKECMDGOALS) --no-print-directory";                      \
        if [ -e $$f/Makefile.fds ]; then                                     \
            ($(MAKE) -C $$f -f Makefile.fds $$makearg) || exit 1;            \
        else                                                                 \
            ($(MAKE) -C $$f $$makearg) || exit 1;                            \
      fi                                                                     \
    done

# ----------------------------------------------------------------------------
# Script to setup build directory
#
scpt_setup_build_dir :=                                                      \
    if [ "$(topdir)" != "." ]; then                                          \
        [ "$(comm_dir_objs)" != "" ] && mkdir -p $(comm_dir_objs);           \
        [ "$(comm_dir_bin)"  != "" ] && mkdir -p $(comm_dir_bin);            \
        [ "$(comm_dir_lib)"  != "" ] && mkdir -p $(comm_dir_lib);            \
    fi                                                                       \

# ----------------------------------------------------------------------------
# Script to build a static library.
#
#	$(ld) $(lda_flg) $$+ -o $$@
#
define scpt_mk_static_lib
$(call comm_lib_tgt,$(1)): $(call comm_src2obj,$($(1)))
ifdef VERBOSE
	$(ar) $(rule_ar_flags) $$@ $$+
else
	@echo "    [LINK .a]     $$@"
	@$(ar) $(rule_ar_flags) $$@ $$+
endif
endef

# ----------------------------------------------------------------------------
# Script to build a dynamic library.
#
define scpt_mk_dynamic_lib
$(call comm_so_tgt,$(1)): $(call comm_src2obj,$($(1)))
ifdef VERBOSE
	$(cpp) $(rule_so_flags) $$+ -o $$@
else
	@echo "    [LINK .so]    $$@"
	@$(cpp) $(rule_so_flags) $$+ -o $$@
endif
endef

# ----------------------------------------------------------------------------
# Script to build a exec binary.
#
define scrpt_mk_exe
$(comm_dir_bin)/$(1): $(call comm_src2obj,$($(1))) $(comm_lib_dep)
ifdef VERBOSE
	$(cpp) $(call comm_src2obj,$($(1))) $(rule_exe_flags) -o $$@
else
	@echo "    [LINK .exe]   $$@"
	@$(cpp) $(call comm_src2obj,$($(1))) $(rule_exe_flags) -o $$@
endif
endef

# ----------------------------------------------------------------------------
# Script to do installation.
#
define scpt_install
$(comm_install_dir)/$(1): $(1)
ifdef VERBOSE
	cp $(1) $(comm_install_dir)
else
	@echo "    [INSTALL]     $(comm_install_dir)/$(1)"
	@cp $(1) $(comm_install_dir)
endif
endef

# ----------------------------------------------------------------------------
# Script to build a kernel module.
#
define scpt_mk_kmod
$(comm_dir_bin)/$(1): $(call comm_src2obj,$($(1)))
ifdef VERBOSE
	$(ld) $(rule_mod_flags) $$+ -o $$@
else
	@echo "    [LINK kmod]   $$@"
	@export LDFLAGS_MODULE="$(rule_mod_flags)"; echo $$LDFLAGS_MODULE
endif
endef

# ----------------------------------------------------------------------------
# Script to do a clean
#
scpt_make_clean :=                                                            \
    if [ "$(topdir)" != "." ]; then                                           \
        rm -rf $(osdep_user_build) $(osdep_kern_build);                       \
    else                                                                      \
        cd Build/$(osdep_target) && rm -rf bin lib tests;                     \
    fi;                                                                       \
    if [ ! -z "$(comm_rpc_files)" ]; then                                     \
        rm $(comm_rpc_files) > /dev/null 2>&1;                                \
    fi;                                                                       \
    exit 0

# ----------------------------------------------------------------------------
# Script to build multiple OS targets
#
scpt_make_all_oses :=                                                        \
    if [ -f Makefile.user ]; then                                            \
        echo Making user runtime lib in `pwd`;                               \
        ($(MAKE) -f Makefile.user --no-print-directory) || exit 1;           \
    fi;                                                                      \
    if [ -f Makefile.kern ]; then                                            \
        echo Making kernel runtime lib in `pwd`;                             \
        ($(MAKE) -f Makefile.kern --no-print-directory) || exit 1;           \
    fi

# ----------------------------------------------------------------------------
# Script to do rpcgen
#
define scpt_mk_rpc_hdr
$(patsubst %.x,$(user_rpc_hdr_dir)/%.h, $(1)): $(1)
	@mkdir -p $(user_rpc_hdr_dir)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -h $$+ -o $$@
else
	@echo "    [RPCGEN .x]   $$@"
	@$(rpcgen) -M -h $$+ -o $$@
endif
endef

define scpt_mk_rpc_clnt
$(patsubst %.x,%_clnt.c, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -l $$+ -o $$@
else
	@echo "    [RPCGEN .x]   $$@"
	@$(rpcgen) -M -l $$+ -o $$@
endif
endef

define scpt_mk_rpc_xdr
$(patsubst %.x,%_xdr.c, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -c $$+ -o $$@
else
	@echo "    [RPCGEN .x]   $$@"
	@$(rpcgen) -M -c $$+ -o $$@
endif
endef

define scpt_mk_rpc_serv
$(patsubst %.x,%_serv.c, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -m $$+ -o $$@
else
	@echo "    [RPCGEN .x]   $$@"
	@$(rpcgen) -M -m $$+ -o $$@
endif
endef

# ----------------------------------------------------------------------------
# Script to do Thrift gen
#
define scpt_mk_thrift
$(patsubst %.thrift,$(user_thrift_loc)/%_types.h, $(1)): $(1)
ifdef VERBOSE
	@echo $(tool_thrift) $(toolchain_thrf_opts) $$<
	@export LD_LIBRARY_PATH=$(toolchain_lib); \
	$(tool_thrift) $(toolchain_thrf_opts) $$<
else
	@echo "    [GEN .thrift] $$<"
	@export LD_LIBRARY_PATH=$(toolchain_lib); \
	$(tool_thrift) $(toolchain_thrf_opts) $$<
endif
endef

define scpt_mk_gen_thrift
$(patsubst %.thrift,$(2)/%/ttypes.py, $(1)): $(1)
ifdef VERBOSE
	@echo $(tool_thrift) $(3) $$<
	@export LD_LIBRARY_PATH=$(toolchain_lib); \
	$(tool_thrift) $(3) -out $(2) $$<
else
	@echo "    [GEN .thrift] $$<"
	@export LD_LIBRARY_PATH=$(toolchain_lib); \
	$(tool_thrift) $(3) -out $(2) $$<
endif
endef

# ----------------------------------------------------------------------------
# Script to compile ragel files
#
define scpt_mk_ragel
$(patsubst %.rl,%.cpp, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(ragel) $$< -L -G1 -o $$@
else
	@echo "    [RAGEL .rl]   $$@"
	@$(ragel) $$< -L -G1 -o $$@
endif
endef

# ----------------------------------------------------------------------------
# Script to generate .h from .json files.
#
define scpt_mk_json
$(patsubst %.json,%.h, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(fds_probe_gen) $$< > $$@
else
	@echo "    [FDS .json]   $$@"
	@$(fds_probe_gen) $$< > $$@
endif
endef

# ----------------------------------------------------------------------------
# Script to do cpp, hpp style check.
#
define scpt_check_style
	@for f in $(3); do                                                       \
		if [ "`basename $$f`" = "`basename $(4)`" ]; then                    \
			echo " ...skipped!"; touch $(5);                                 \
			exit 0;                                                          \
		fi;                                                                  \
	done;                                                                    \
	echo "";                                                                 \
    $(1) $(2) $(4) && touch $(5);
endef

# ----------------------------------------------------------------------------
# Script to remove a file list
# TODO: should find a better way to pass variables to the shell.
#
scpt_rm_file_list :=                                                         \
    echo $(user_rm_text);                                                    \
    for f in $(user_rm_list); do                                             \
        [ -f $$f ] && rm $$f;                                                \
    done

# ----------------------------------------------------------------------------
# Script to do cscope.
#
scpt_make_cscope :=                                                          \
    cd $(topdir);                                                            \
    find -L . -name "*.[chx]" -xtype f -print > cscope.files;                \
    find -L . -name "*.cpp" -xtype f -print >> cscope.files;                 \
    find -L . -name "*.cc" -xtype f -print >> cscope.files;                  \
    find -L . -name "*.hpp" -xtype f -print >> cscope.files;                 \
    cat cscope.files | xargs ctags;                                          \
	cscope -q

# ----------------------------------------------------------------------------
# Script to update ctags database
#
scpt_make_ctags :=                                                           \
    ctags `cat cscope.files`
