/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <disk.h>
#include <platform.h>
#include <platform/fds-osdep.h>
#include <net-platform.h>
#include <net/net-service.h>

int main(int argc, char **argv)
{
    fds::Module *plat_vec[] = { &fds::gl_NodePlatform, &fds::gl_DiskPlatMod, &fds::gl_NetService,
                                &fds::gl_PlatformdNetSvc, NULL };
    fds::NodePlatformProc plat(argc, argv, plat_vec);

    return plat.main();
}
