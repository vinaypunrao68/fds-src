/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <iostream>  // NOLINT
#include <disk.h>
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
    plf_node_data.nd_flag_run_sm = msg->has_sm_service;
    plf_node_data.nd_flag_run_dm = msg->has_dm_service;
    plf_node_data.nd_flag_run_am = msg->has_am_service;
    plf_db->set(plf_db_key,
                std::string((const char *)&plf_node_data, sizeof(plf_node_data)));

    if (auto_start == true) {
        if (msg->has_sm_service) {
            pid = fds_spawn_service("StorMgr",
                    (std::string("--fds.sm.om_ip=") +
                            conf.get<std::string>("om_ip")).c_str(),
                    proc_root->dir_fdsroot().c_str(), true);
            LOGNOTIFY << "Spawn SM as daemon";

            // TODO(Vy): must fix the transport stuff!
            sleep(1);
        }
        if (msg->has_dm_service) {
            pid = fds_spawn_service("DataMgr",
                    (std::string("--fds.dm.om_ip=") +
                            conf.get<std::string>("om_ip")).c_str(),
                    proc_root->dir_fdsroot().c_str(), true);
            LOGNOTIFY << "Spawn DM as daemon";
        }
        if (msg->has_am_service) {
            sleep(5);
            pid = fds_spawn_service("AMAgent",
                    (std::string("--fds.am.om_ip=") +
                            conf.get<std::string>("om_ip")).c_str(),
                    proc_root->dir_fdsroot().c_str(), true);
            LOGNOTIFY << "Spawn AM as daemon";
        }
    } else {
        LOGNOTIFY << "Auto start services is off, wait for manual start...";
        std::cout << "Auto start services is off, wait for manual start..." << std::endl;
    }
}

// plf_fill_disk_capacity
// ----------------------
//
void
NodePlatformProc::plf_fill_disk_capacity_pkt(fpi::FDSP_RegisterNodeTypePtr pkt)
{
    // for disk_iops_max -- report a little less than it could do
    // to reserve some iops for local background tasks (such as migration, etc)
    LOGNOTIFY << "Send system capacity to OM";

    pkt->disk_info.disk_iops_max    = 3600;
    pkt->disk_info.disk_iops_min    = 1000;
    pkt->disk_info.disk_capacity    = 0x7ffff;
    pkt->disk_info.disk_latency_max = 1000000 / pkt->disk_info.disk_iops_min;
    pkt->disk_info.disk_latency_min = 1000000 / pkt->disk_info.disk_iops_max;
    pkt->disk_info.ssd_iops_max     = 300000;
    pkt->disk_info.ssd_iops_min     = 1000;
    pkt->disk_info.ssd_capacity     = 0x10000;
    pkt->disk_info.ssd_latency_max  = 1000000 / pkt->disk_info.ssd_iops_min;
    pkt->disk_info.ssd_latency_min  = 1000000 / pkt->disk_info.ssd_iops_max;
    pkt->disk_info.disk_type        = FDS_DISK_SATA;
}

void
NodePlatformProc::proc_pre_startup()
{
    NodePlatform *plat = static_cast<NodePlatform *>(plf_mgr);

    plat->mod_load_from_config();
    PlatformProcess::proc_pre_startup();
    plat->plf_bind_process(this);
}

int
NodePlatformProc::run()
{
    fpi::FDSP_RegisterNodeTypePtr pkt(new FDSP_RegisterNodeType);

    plf_mgr->plf_run_server(true);

    plf_fill_disk_capacity_pkt(pkt);
    plf_mgr->plf_rpc_om_handshake(pkt);

    while (1) {
        sleep(1000);   /* we'll do hotplug uevent thread in here */
    }
    return 0;
}

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *plat_vec[] = {
        &fds::gl_NodePlatform,
        &fds::gl_DiskPlatMod,
        NULL
    };
    fds::NodePlatformProc plat(argc, argv, plat_vec);
    int ret = plat.main();
    return ret;
}
