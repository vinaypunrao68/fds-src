/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <platform.h>
#include <kvstore/configdbmodule.h>

namespace fds {

NodePlatformProc::NodePlatformProc(int argc, char **argv, Module **vec)
    : PlatformProcess(argc, argv, "fds.plat.", &gl_NodePlatform, vec) {}

// plf_start_node_services
// -----------------------
//
void
NodePlatformProc::plf_start_node_services()
{
}

void
NodePlatformProc::setup()
{
    PlatformProcess::setup();
}

void
NodePlatformProc::run()
{
    plf_mgr->plf_run_server(true);
    plf_mgr->plf_rpc_om_handshake();

    while (1) {
        sleep(1000);   /* we'll do hotplug uevent thread in here */
    }
}

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *plat_vec[] = {
        &fds::gl_NodePlatform,
        NULL
    };
    fds::NodePlatformProc plat(argc, argv, plat_vec);

    plat.setup();
    plat.run();
    return 0;
}
