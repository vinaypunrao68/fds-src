topdir         := ..
speedup        = -j $(threads)

ifdef NOTESTS
user_build_dir :=
else
user_build_dir :=      \
    lib                \
    orch-mgr           \
    persistent-layer   \
    umod-lib           \
    util               \
    stor-mgr           \
    data-mgr           \
    volume-checker     \
    svclayer
endif

include $(topdir)/Makefile.incl
