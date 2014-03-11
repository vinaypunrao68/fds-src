/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <platform.h>
#include <platform/fds-osdep.h>

namespace fds {

NodePlatformProc::NodePlatformProc(int argc, char **argv, Module **vec)
    : PlatformProcess(argc, argv, "fds.plat.", "platform.log", &gl_NodePlatform, vec) {}

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
NodePlatformProc::plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg)
{
    pid_t             pid;
    bool              auto_start;
    FdsConfigAccessor conf(get_conf_helper());

    auto_start = conf.get<bool>("auto_start_services", true);
    if (auto_start == true) {
        if (plf_node_data.nd_flag_run_sm) {
            pid = fds_spawn_service("StorMgr", proc_root->dir_fdsroot().c_str());
            LOGNOTIFY << "Spawn SM with pid " << pid;

            // TODO(Vy): must fix the transport stuff!
            sleep(1);
        }
        if (plf_node_data.nd_flag_run_dm) {
            pid = fds_spawn_service("DataMgr", proc_root->dir_fdsroot().c_str());
            LOGNOTIFY << "Spawn DM with pid " << pid;
        }
        if (plf_node_data.nd_flag_run_am) {
            sleep(5);
            pid = fds_spawn_service("AMAgent", proc_root->dir_fdsroot().c_str());
            LOGNOTIFY << "Spawn AM with pid " << pid;
        }
    } else {
        LOGNOTIFY << "Auto start services is off, wait for manual start...";
        std::cout << "Auto start services is off, wait for manual start..." << std::endl;
    }
}

void
NodePlatformProc::setup()
{
    PlatformProcess::setup();
    NodePlatform *plat = static_cast<NodePlatform *>(plf_mgr);
    plat->plf_bind_process(this);
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
