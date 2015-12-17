topdir         := ..
user_target    := tool
user_std_libs  := true
#user_predep    := debugtool
user_rtime_env := user

user_incl_dir     := $(topdir) \
    $(topdir)/platform/include \
    $(topdir)/net-service/include \
    $(topdir)/data-mgr/include

user_cpp_flags    := -DLEVELDB_PLATFORM_POSIX
user_fds_so_libs  :=
user_fds_ar_libs  := \
    fds-lib \
    fds-dm-lib \
    fds-fdsp

user_non_fds_libs := \
    leveldb

user_hh           := $(wildcard *.h)
user_cpp          := \
    shm_dump.cpp \
    journal_dump.cpp \
    ldb-tool.cpp \
    fdsio.cpp

user_no_style     := \
    journal_dump.cpp

user_bin_exe      := shm_dump journal-dump ldb-tool fdsio
shm_dump          := shm_dump.cpp
journal-dump      := journal_dump.cpp
ldb-tool          := ldb-tool.cpp
fdsio             := fdsio.cpp

#user_clean        := /tmp/.___dummy__to_trigger_user_clean

include $(topdir)/Makefile.incl

#user_clean        := $(fds_tool_bin)/debugtool
.PHONY: debugtool
debugtool: $(fds_tool_bin)/debugtool
$(fds_tool_bin)/debugtool: $(topdir)/fdsp/*.thrift $(topdir)/tools/debugtool.spec $(shell find $(topdir)/tools/fdsconsole/ -type f -name '*.py')
	@echo "making debugtool @ $(fds_tool_bin)/debugtool"
	@pyinstaller  \
                --noconfirm \
                --onefile \
                --key=FD5BADA55BABE \
                --log-level=WARN \
                --distpath=$(fds_tool_bin) \
                --workpath=$(osdep_user_build)/debugtool \
                --name=debugtool debugtool.spec
