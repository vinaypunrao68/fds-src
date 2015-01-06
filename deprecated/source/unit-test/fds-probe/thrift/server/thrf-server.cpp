/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <util/fds_stat.h>
#include <fds-probe/s3-probe.h>
#include <endpoint-test.h>
#include <net/net-service.h>
#include <platform.h>
#include <net-platform.h>

int main(int argc, char **argv)
{
    fds::Module *srv_vec[] = {
        &fds::gl_NodePlatform,
        &fds::gl_NetService,
        &fds::gl_PlatformdNetSvc,
        &fds::gl_ProbeTestSM,
        NULL
    };
    fds::NodePlatformProc server(argc, argv, srv_vec);
    return server.main();
}
