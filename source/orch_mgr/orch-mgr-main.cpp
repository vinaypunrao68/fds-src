/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>

namespace fds {
extern OrchMgr *orchMgr;
}  // namespace fds

int main(int argc, char *argv[]) {
    fds::orchMgr = new fds::OrchMgr(argc, argv, "orch_mgr.conf", "fds.om.");

    fds::gl_orch_mgr = fds::orchMgr;

    fds::Module *omVec[] = {
        fds::orchMgr,
        nullptr};

    fds::orchMgr->setup(argc, argv, omVec);
    fds::orchMgr->run();

    delete fds::orchMgr;

    return 0;
}

