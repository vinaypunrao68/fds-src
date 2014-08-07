/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>
#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <orch-mgr/om-service.h>
#include <OmResources.h>
#include <om-platform.h>
#include <net/net-service.h>

namespace fds {

extern OrchMgr *orchMgr;
OM_Module       gl_OMModule("OM");

OM_Module *OM_Module::om_singleton()
{
    return &gl_OMModule;
}

}  // namespace fds

int om_gdb = 0;

int main(int argc, char *argv[])
{
    fds::Module *omVec[] = {
        &fds::gl_OmPlatform,
        &fds::gl_NetService,
        &fds::gl_OMModule,
        NULL
    };
    while (om_gdb == 1) { sleep(1); }

    fds::orchMgr = new fds::OrchMgr(argc, argv, &fds::gl_OmPlatform, omVec);
    fds::gl_orch_mgr = fds::orchMgr;
    int ret = fds::orchMgr->main();
    delete fds::orchMgr;
    return ret;
}
