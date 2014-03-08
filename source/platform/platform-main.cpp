/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <platform.h>
#include <kvstore/configdbmodule.h>

PlatformProcess::PlatformProcess(int argc, char **argv,
                                 const std::string  &cfg,
                                 Module            **vec)
    : FdsProcess(argc, argv, "platform.conf", cfg, "platform.log", vec)
{
    memset(&plf_node_data, 0, sizeof(plf_node_data));
}

void
PlatformProcess::plf_load_node_data()
{
}

void
PlatformProcess::plf_apply_node_data()
{
    char         name[64];
    fds_uint32_t base_port;
    std::string  my_name;
    NodeUuid     my_uuid(plf_node_data.node_uuid);

    base_port = plf_mgr->plf_get_my_ctrl_port() + (plf_node_data.node_number * 1000);
    snprintf(name, sizeof(name), "node-%u", plf_node_data.node_number);
    plf_mgr->plf_change_info(my_uuid, std::string(name), base_port);
}

void
PlatformProcess::plf_start_node_services()
{
}

void
PlatformProcess::setup()
{
    FdsProcess::setup();
    plf_mgr = &fds::gl_NodePlatform;
    plf_db  = fds::gl_configDB.get();

    plf_load_node_data();
    plf_apply_node_data();
}

void
PlatformProcess::run()
{
    plf_mgr->plf_run_server(true);
    plf_mgr->plf_rpc_om_handshake();

    while (1) {
        sleep(1000);   /* we'll do hotplug uevent thread in here */
    }
}

int main(int argc, char **argv)
{
    fds::Module *plat_vec[] = {
        &fds::gl_configDB,
        &fds::gl_NodePlatform,
        NULL
    };
    PlatformProcess plat(argc, argv, "fds.plat.", plat_vec);

    plat.setup();
    plat.run();
    return 0;
}
