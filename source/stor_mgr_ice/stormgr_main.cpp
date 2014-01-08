/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>

int main(int argc, char *argv[]) {
    objStorMgr = new ObjectStorMgr();

    /* Instantiate a DiskManager Module instance */
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_tierPolicy,
        fds::objStorMgr->objStats,
        objStorMgr,
        nullptr
    };
    fds::ModuleVector smModVec(argc, argv, smVec);
    smModVec.mod_execute();

    objStorMgr->runServer();

    delete objStorMgr;

    return 0;
}

