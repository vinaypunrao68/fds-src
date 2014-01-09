/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>
#include <fds_process.h>

int main(int argc, char *argv[]) {
    objStorMgr = new ObjectStorMgr("fds.conf", "fds.sm.");

    /* Instantiate a DiskManager Module instance */
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_tierPolicy,
        fds::objStorMgr->objStats,
        objStorMgr,
        nullptr
    };
    objStorMgr->setup(argc, argv, smVec);
    objStorMgr->run();

    delete objStorMgr;

    return 0;
}

