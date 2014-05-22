/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>
#include <sm-platform.h>
#include <net/net-service.h>

int main(int argc, char *argv[])
{
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        &fds::gl_SmPlatform,
        &fds::gl_NetService,
        &fds::gl_tierPolicy,
        &fds::gl_objStats,
        nullptr
    };
    objStorMgr = new ObjectStorMgr(argc, argv, &fds::gl_SmPlatform, smVec);
    int ret = objStorMgr->main();
    delete objStorMgr;
    return ret;
}

