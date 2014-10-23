topdir            := ../..
include $(topdir)/Makefile.cpus
speedup           := -j $(parallel_build_count)
user_build_dir    := \
    shared       \
    s3           \
    am pm sm     \
    thrift       \
    sm-embed     \
    s3-template  \
    blk-template \
    threadpool   \
    fds-net      \
    stor-mgr     \
    stor-mgr-full\
    data-mgr     \
    orch-mgr     \
    platform     \
    mock-server

include $(topdir)/Makefile.incl
