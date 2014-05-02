/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <endpoint-test.h>

namespace fds {

ProbeEpTestAM                 gl_ProbeTestAM("Probe AM EP");

// mode_init
// ---------
//
int
ProbeEpTestAM::mod_init(SysParams const *const p)
{
    EndPointMgr *mgr;

    Module::mod_init(p);

    // Register the endpoint with the local domain.
    mgr = EndPointMgr::ep_mgr_singleton();
    return 0;
}

// mod_startup
// -----------
//
void
ProbeEpTestAM::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
ProbeEpTestAM::mod_shutdown()
{
}

}  // namespace fds
