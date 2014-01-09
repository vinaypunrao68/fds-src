/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>
#include <fds_process.h>

int main(int argc, char *argv[]) {
    boost::shared_ptr<FdsConfig> config(new FdsConfig("fds.conf"));
    objStorMgr = new ObjectStorMgr(config);

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

