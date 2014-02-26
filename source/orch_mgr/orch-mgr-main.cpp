/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>
#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <orch-mgr/om-service.h>

namespace fds {

extern OrchMgr *orchMgr;
OM_Module       gl_OMModule("OM");

OM_Module *
OM_Module::om_singleton()
{
    return &gl_OMModule;
}

}  // namespace fds

int main(int argc, char *argv[])
{
    fds::Module *omVec[] = {
        &fds::gl_OMModule,
        NULL
    };
    fds::orchMgr = new fds::OrchMgr(argc, argv, "orch_mgr.conf", "fds.om.", omVec);
    fds::gl_orch_mgr = fds::orchMgr;

    fds::orchMgr->setup();
    fds::orchMgr->run();

    delete fds::orchMgr;

    return 0;
}

