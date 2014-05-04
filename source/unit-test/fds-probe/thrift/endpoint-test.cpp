/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <endpoint-test.h>

namespace fds {

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
ProbeEpPlugin::svc_down()
{
    svc_cleanup_start();
    svc_cleanup_finish();
}

}  // namespace fds
