/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>  // NOLINT
#include <fds_uuid.h>
#include <platform/fds-osdep.h>
#include <net-platform.h>
#include <net/net-service.h>

#include "node_platform.h"
#include "node_shared_memory_rw_ctrl.h"
#include "node_platform_process.h"
#include "disk_plat_module.h"

namespace fds
{
    NodePlatformProc::NodePlatformProc(int argc, char **argv, Module **vec) : 
                                       PlatformProcess(argc, argv, "fds.plat.", "platform.log",
                                       &gl_NodePlatform, vec), amInstanceCount(0)
    {
    }

    // plf_load_node_data
    // ------------------
    //
    void NodePlatformProc::plf_load_node_data()
    {
        PlatformProcess::plf_load_node_data();

        if (plf_node_data.nd_has_data == 0)
        {
            NodeUuid    uuid(fds_get_uuid64(get_uuid()));
            plf_node_data.nd_node_uuid = uuid.uuid_get_val();
            LOGNOTIFY << "NO UUID, generate one " << std::hex << plf_node_data.nd_node_uuid <<
            std::dec;
        }

        // Make up some values for now.
        //
        plf_node_data.nd_has_data    = 1;
        plf_node_data.nd_magic       = 0xcafecaaf;
        plf_node_data.nd_node_number = 0;
        plf_node_data.nd_plat_port   = plf_mgr->plf_get_my_node_port();
        plf_node_data.nd_om_port     = plf_mgr->plf_get_om_ctrl_port();
        plf_node_data.nd_flag_run_sm = 1;
        plf_node_data.nd_flag_run_dm = 1;
        plf_node_data.nd_flag_run_am = 1;
        plf_node_data.nd_flag_run_om = 1;

        // TODO(Vy): deal with error here.
        //
        plf_db->set(plf_db_key, std::string((const char *)&plf_node_data, sizeof(plf_node_data)));
    }

    // plf_start_node_services
    // -----------------------
    //
    void NodePlatformProc::plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg)
    {
        pid_t                pid;
        bool                 auto_start;
        FdsConfigAccessor    conf(get_conf_helper());

        auto_start = conf.get<bool>("auto_start_services", true);
        plf_node_data.nd_flag_run_sm = msg->has_sm_service;
        plf_node_data.nd_flag_run_dm = msg->has_dm_service;
        plf_node_data.nd_flag_run_am = msg->has_am_service;
        plf_db->set(plf_db_key, std::string((const char *)&plf_node_data, sizeof(plf_node_data)));

        sleep(10);

        if (auto_start == true)
        {
            if (msg->has_sm_service)
            {
                std::vector<const char*>    extra_args;
                std::stringstream           arg1stream;
                std::string                 arg1;
                std::string                 arg2;
                std::string                 arg3;
                arg1stream << "--fds.plat.platform_port=" << conf.get<int>("platform_port");
                arg1 = arg1stream.str();
                extra_args.push_back(arg1.c_str());
                arg2 = std::string("--fds.plat.om_ip=") + conf.get<std::string>("om_ip");
                extra_args.push_back(arg2.c_str());
                arg3 = std::string("--fds.sm.om_ip=") + conf.get<std::string>("om_ip");
                extra_args.push_back(arg3.c_str());
                extra_args.push_back(NULL);
                pid = fds_spawn_service("StorMgr",
                                        proc_root->dir_fdsroot().c_str(), &extra_args[0], true);
                LOGNOTIFY << "Spawn SM as daemon";

                // TODO(Vy): must fix the transport stuff!
            }

            if (msg->has_dm_service)
            {
                std::vector<const char*>    extra_args;
                std::stringstream           arg1stream;
                std::string                 arg1;
                std::string                 arg2;
                std::string                 arg3;
                arg1stream << "--fds.plat.platform_port=" << conf.get<int>("platform_port");
                arg1 = arg1stream.str();
                extra_args.push_back(arg1.c_str());
                arg2 = std::string("--fds.plat.om_ip=") + conf.get<std::string>("om_ip");
                extra_args.push_back(arg2.c_str());
                arg3 = std::string("--fds.dm.om_ip=") + conf.get<std::string>("om_ip");
                extra_args.push_back(arg3.c_str());
                extra_args.push_back(NULL);
                pid = fds_spawn_service("DataMgr",
                                        proc_root->dir_fdsroot().c_str(), &extra_args[0], true);
                LOGNOTIFY << "Spawn DM as daemon";
            }

            if (msg->has_am_service)
            {
                std::vector<const char*>    extra_args;
                std::stringstream           arg1stream;
                std::string                 arg1;
                std::string                 arg2;
                std::string                 arg3;
                std::string                 arg4;
                arg1stream << "--fds.plat.platform_port=" << conf.get<int>("platform_port");
                arg1 = arg1stream.str();
                extra_args.push_back(arg1.c_str());
                arg2 = std::string("--fds.plat.om_ip=") + conf.get<std::string>("om_ip");
                extra_args.push_back(arg2.c_str());
                arg3 = std::string("--fds.am.om_ip=") + conf.get<std::string>("om_ip");
                extra_args.push_back(arg3.c_str());
                arg4 = std::string("--fds.am.instanceId=") +
                       std::to_string(amInstanceCount);
                extra_args.push_back(arg4.c_str());
                extra_args.push_back(NULL);
                pid = fds_spawn_service("AMAgent",
                                        proc_root->dir_fdsroot().c_str(), &extra_args[0], true);
                LOGNOTIFY << "Spawn AM as daemon";
                ++amInstanceCount;
            }
        }else {
            LOGNOTIFY << "Auto start services is off, wait for manual start...";
            std::cout << "Auto start services is off, wait for manual start..." << std::endl;
        }
    }

    // plf_fill_disk_capacity
    // ----------------------
    //
    void NodePlatformProc::plf_fill_disk_capacity_pkt(fpi::FDSP_RegisterNodeTypePtr pkt)
    {
        // for disk_iops_max -- report a little less than it could do
        // to reserve some iops for local background tasks (such as migration, etc)
        LOGNOTIFY << "Send system capacity to OM";

        pkt->disk_info.disk_iops_max    = 10000;
        pkt->disk_info.disk_iops_min    = 4000;
        pkt->disk_info.disk_capacity    = 0x7ffff;
        pkt->disk_info.disk_latency_max = 1000000 / pkt->disk_info.disk_iops_min;
        pkt->disk_info.disk_latency_min = 1000000 / pkt->disk_info.disk_iops_max;
        pkt->disk_info.ssd_iops_max     = 300000;
        pkt->disk_info.ssd_iops_min     = 1000;
        pkt->disk_info.ssd_capacity     = 0x10000;
        pkt->disk_info.ssd_latency_max  = 1000000 / pkt->disk_info.ssd_iops_min;
        pkt->disk_info.ssd_latency_min  = 1000000 / pkt->disk_info.ssd_iops_max;
        pkt->disk_info.disk_type        = FDS_DISK_SATA;

        // over-write some value from config if testing perf
        FdsConfigAccessor    conf(get_conf_helper());

        if (conf.get<bool>("testing.manual_nodecap"))
        {
            pkt->disk_info.disk_iops_min = conf.get<int>("testing.disk_iops_min");
            pkt->disk_info.disk_iops_max = conf.get<int>("testing.disk_iops_max");
        }
    }

    void NodePlatformProc::proc_pre_startup()
    {
        NodePlatform   *plat = static_cast<NodePlatform *>(plf_mgr);

        PlatformProcess::proc_pre_startup();
        plat->plf_bind_process(this);
    }

    int NodePlatformProc::run()
    {
        auto                             disk_ctrl = DiskPlatModule::dsk_plat_singleton();
        fpi::FDSP_RegisterNodeTypePtr    pkt(new fpi::FDSP_RegisterNodeType);

        plf_fill_disk_capacity_pkt(pkt);
        plf_mgr->plf_rpc_om_handshake(pkt);

        while (1)
        {
            disk_ctrl->dsk_monitor_hotplug();
            sleep(1000);   /* we'll do hotplug uevent thread in here */
        }
        return 0;
    }
}  // namespace fds
