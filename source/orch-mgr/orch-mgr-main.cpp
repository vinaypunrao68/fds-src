/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>
#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <orch-mgr/om-service.h>
#include <OmResources.h>

namespace fds {

extern OrchMgr *orchMgr;
OM_Module *omModule;

OM_Module *OM_Module::om_singleton()
{
    return omModule;
}

}  // namespace fds

int om_gdb = 0;

int main(int argc, char *argv[])
{
    omModule = new OM_Module("OM");

    while (om_gdb == 1) { sleep(1); }

    fds::orchMgr = new fds::OrchMgr(argc, argv, omModule);

    int ret = fds::orchMgr->main();

    delete omModule;
    omModule = nullptr;
    delete fds::orchMgr;
    fds::orchMgr = nullptr;

    return ret;
}
