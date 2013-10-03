#
# Makefile.scpt
# -------------
# Shared scripts for the Makefile system.
#

# ----------------------------------------------------------------------------
# Script do subdir build
#
scpt_build_subdir =                                                          \
    for f in $(user_build); do                                               \
        echo Making $(MAKECMDGOALS) in `pwd`/$$f;                            \
        (cd $$f && $(MAKE) $(MAKECMDGOALS) --no-print-directory) || exit 1;  \
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
	@echo "\t[LINK .a]\t$$@"
	@$(ar) $(rule_ar_flags) $$@ $$+
endif
endef

# ----------------------------------------------------------------------------
# Script to build a dynamic library.
#
define scpt_mk_dynamic_lib
$(call comm_so_tgt,$(1)): $(call comm_src2obj,$($(1)))
ifdef VERBOSE
	$(ld) $(rule_so_flags) $$+ -o $$@
else
	@echo "\t[LINK .so]\t$$@"
	@$(ld) $(rule_so_flags) $$+ -o $$@
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
	@echo "\t[LINK .exe]\t$$@"
	@$(cpp) $(call comm_src2obj,$($(1))) $(rule_exe_flags) -o $$@
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
	@echo "\t[LINK kmod]\t$$@"
	@export LDFLAGS_MODULE="$(rule_mod_flags)"; echo $$LDFLAGS_MODULE
endif
endef

# ----------------------------------------------------------------------------
# Script to do a clean
#
scpt_make_clean :=                                                            \
    [ "$(topdir)" != "." ] && rm -rf $(osdep_user_build) $(osdep_kern_build); \
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
	@echo "\t[RPCGEN .x]\t$$@"
	@$(rpcgen) -M -h $$+ -o $$@
endif
endef

define scpt_mk_rpc_clnt
$(patsubst %.x,%_clnt.c, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -l $$+ -o $$@
else
	@echo "\t[RPCGEN .x]\t$$@"
	@$(rpcgen) -M -l $$+ -o $$@
endif
endef

define scpt_mk_rpc_xdr
$(patsubst %.x,%_xdr.c, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -c $$+ -o $$@
else
	@echo "\t[RPCGEN .x]\t$$@"
	@$(rpcgen) -M -c $$+ -o $$@
endif
endef

define scpt_mk_rpc_serv
$(patsubst %.x,%_serv.c, $(1)): $(1)
	@rm -f $$@
ifdef VERBOSE
	$(rpcgen) -M -m $$+ -o $$@
else
	@echo "\t[RPCGEN .x]\t$$@"
	@$(rpcgen) -M -m $$+ -o $$@
endif
endef
   
# ----------------------------------------------------------------------------
# Script to do cscope.
#
scpt_make_cscope :=                                                          \
    cd $(topdir);                                                            \
    find -L . -name "*.[chx]" -print > cscope.files;                         \
    cat cscope.files | xargs ctags;                                          \
	cscope -q

# ----------------------------------------------------------------------------
# Script to update ctags database
#
scpt_make_ctags :=                                                           \
    ctags `cat cscope.files`
