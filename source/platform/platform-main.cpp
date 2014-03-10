/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <platform.h>

namespace fds {

NodePlatformProc::NodePlatformProc(int argc, char **argv, Module **vec)
    : PlatformProcess(argc, argv, "fds.plat.", &gl_NodePlatform, vec) {}

// plf_load_node_data
// ------------------
//
void
NodePlatformProc::plf_load_node_data()
{
    PlatformProcess::plf_load_node_data();
    if (plf_node_data.nd_has_data == 0) {
        plf_node_data.nd_node_uuid = fds_get_uuid64(get_uuid());
        std::cout << "NO UUID, generate one " << std::hex
                  << plf_node_data.nd_node_uuid << std::endl;
    }
    // Make up some values for now.
    //
    plf_node_data.nd_has_data    = 1;
    plf_node_data.nd_magic       = 0xcafecaaf;
    plf_node_data.nd_node_number = 0;
    plf_node_data.nd_plat_port   = plf_mgr->plf_get_my_ctrl_port();
    plf_node_data.nd_om_port     = plf_mgr->plf_get_om_ctrl_port();
    plf_node_data.nd_flag_run_sm = 1;
    plf_node_data.nd_flag_run_dm = 1;
    plf_node_data.nd_flag_run_am = 1;
    plf_node_data.nd_flag_run_om = 1;

    // TODO(Vy): deal with error here.
    //
    plf_db->set(plf_db_key,
                std::string((const char *)&plf_node_data, sizeof(plf_node_data)));
}

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
