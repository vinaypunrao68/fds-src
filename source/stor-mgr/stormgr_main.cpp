/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>
#include <sm-platform.h>

int main(int argc, char *argv[])
{
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_SmPlatform,
        &fds::gl_tierPolicy,
        &fds::gl_objStats,
        nullptr
    };
    objStorMgr = new ObjectStorMgr(argc, argv, &fds::gl_SmPlatform, smVec);
    objStorMgr->setup();
    objStorMgr->run();

    delete objStorMgr;
    return 0;
}

