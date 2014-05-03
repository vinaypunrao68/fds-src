/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <endpoint-test.h>

namespace fds {

ProbeEpTest   gl_ProbeTest("Probe EP");

ProbeEpTest::ProbeEpTest(const char *name) : Module(name) {}

int
ProbeEpTest::mod_init(SysParams const *const p)
{
    Module::mod_init(p);

    return 0;
}

void
ProbeEpTest::mod_startup()
{
}

void
ProbeEpTest::mod_shutdown()
{
}

/**
 * ProbeTest EndPoint error handling path
 */
void
ProbeEpPlugin::ep_connected()
{
}

void
ProbeEpPlugin::ep_down()
{
    ep_cleanup_start();
    ep_cleanup_finish();
}

void
ProbeEpPlugin::ep_svc_down()
{
    svc_cleanup_start();
    svc_cleanup_finish();
}

/**
 * Service handlers.
 */
void
ProbeHelloSvc::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

void
ProbeByeSvc::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

void
ProbePokeSvc::svc_receive_msg(const fpi::AsyncHdr &msg)
{
}

}  // namespace fds
