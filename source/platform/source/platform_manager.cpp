/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>  // NOLINT

#include <fds_uuid.h>
#include <fdsp/svc_types_types.h>
#include <fds_process.h>
#include "fds_resource.h"
#include <platform/process.h>
#include "disk_plat_module.h"
#include <util/stringutils.h>

#include "platform/platform_manager.h"
#include "platform/disk_capabilities.h"

namespace fds
{
    namespace pm
    {
        PlatformManager::PlatformManager() : Module("pm")
        {
        }

        int PlatformManager::mod_init(SysParams const *const param)
        {
            conf = new FdsConfigAccessor(g_fdsprocess->get_conf_helper());
            rootDir = g_fdsprocess->proc_fdsroot()->dir_fdsroot();
            db = new kvstore::PlatformDB(rootDir, conf->get<std::string>("redis_host","localhost"), conf->get<int>("redis_port", 6379), 1);

            if (!db->isConnected())
            { 
                LOGCRITICAL << "unable to talk to platformdb @ [" << conf->get<std::string>("redis_host","localhost") << ":" << conf->get<int>("redis_port", 6379) << "]";
            } else {
                db->getNodeInfo(nodeInfo);
                db->getNodeDiskCapability(diskCapability);
            }

            if (nodeInfo.uuid <= 0)
            {
                NodeUuid    uuid(fds_get_uuid64(get_uuid()));

                nodeInfo.uuid = uuid.uuid_get_val();
                nodeInfo.uuid = getNodeUUID(fpi::FDSP_PLATFORM);
                LOGNOTIFY << "generated a new uuid for this node : " << nodeInfo.uuid;
                db->setNodeInfo(nodeInfo);
            } else {
                LOGNOTIFY << "Using stored uuid for this node : " << nodeInfo.uuid;
            }

            determineDiskCapability();

            return 0;
        }

        void PlatformManager::mod_shutdown()
        {

        }

        pid_t PlatformManager::startAM()
        {
            std::vector<std::string>    args;
            pid_t                       pid;

            args.push_back(util::strformat("--fds.common.om_ip_list=%s",
                                           conf->get_abs<std::string>("fds.common.om_ip_list").c_str()));
            args.push_back(util::strformat("--fds.pm.platform_uuid=%lld", getNodeUUID(fpi::FDSP_PLATFORM)));
            args.push_back(util::strformat("--fds.pm.platform_port=%d", conf->get<int>("platform_port")));

            pid = fds_spawn_service("AMAgent", rootDir, args, true);

            if (pid > 0)
            {
                LOGNOTIFY << "Spawn AM as daemon";
            } else {
                LOGCRITICAL << "error spawning AM";
            }

            return pid;
        }

        pid_t PlatformManager::startDM()
        {
            std::vector<std::string>    args;
            pid_t                       pid;

            args.push_back(util::strformat("--fds.common.om_ip_list=%s",
                                           conf->get_abs<std::string>("fds.common.om_ip_list").c_str()));
            args.push_back(util::strformat("--fds.pm.platform_uuid=%lld", getNodeUUID(fpi::FDSP_PLATFORM)));
            args.push_back(util::strformat("--fds.pm.platform_port=%d", conf->get<int>("platform_port")));

            pid = fds_spawn_service("DataMgr", rootDir, args, true);

            if (pid > 0)
            {
                LOGNOTIFY << "Spawn DM as daemon";
                db->setNodeInfo(nodeInfo);
            } else {
                LOGCRITICAL << "error spawning DM";
            }

            return pid;
        }

        pid_t PlatformManager::startSM()
        {
            std::vector<std::string>    args;
            pid_t                       pid;

            args.push_back(util::strformat("--fds.common.om_ip_list=%s",
                                           conf->get_abs<std::string>("fds.common.om_ip_list").c_str()));
            args.push_back(util::strformat("--fds.pm.platform_uuid=%lld", getNodeUUID(fpi::FDSP_PLATFORM)));
            args.push_back(util::strformat("--fds.pm.platform_port=%d", conf->get<int>("platform_port")));

            pid = fds_spawn_service("StorMgr", rootDir, args, true);

            if (pid > 0)
            {
                LOGNOTIFY << "Spawn SM as daemon";
            } else {
                LOGCRITICAL << "error spawning SM";
            }

            return pid;
        }

        // plf_start_node_services
        // -----------------------
        //
        void PlatformManager::activateServices(const fpi::ActivateServicesMsgPtr &activateMsg)
        {
            pid_t    pid;
            bool     auto_start;
            auto     &info = activateMsg->info;

            if (info.has_sm_service)
            {
                nodeInfo.fHasAm = true;
                startSM();
            }

            if (info.has_dm_service)
            {
                nodeInfo.fHasDm = true;
                startDM();
            }

            if (info.has_am_service)
            {
                nodeInfo.fHasAm = true;
                startAM();
            }

            db->setNodeInfo(nodeInfo);
        }

        void PlatformManager::updateServiceInfoProperties(std::map<std::string, std::string> *data)
        {
            determineDiskCapability();
            util::Properties props = util::Properties(data);
            props.set("fds_root", rootDir);
            props.setInt("uuid", nodeInfo.uuid);
            props.setInt("node_iops_max", diskCapability.node_iops_max);
            props.setInt("node_iops_min", diskCapability.node_iops_min);
            props.setDouble("disk_capacity", diskCapability.disk_capacity);
            props.setDouble("ssd_capacity", diskCapability.ssd_capacity);
            props.setInt("disk_type", diskCapability.disk_type);
        }

        void PlatformManager::determineDiskCapability()
        {
            auto ssd_iops_max = conf->get<uint32_t>("capabilities.disk.ssd.iops_max");
            auto ssd_iops_min = conf->get<uint32_t>("capabilities.disk.ssd.iops_min");
            auto hdd_iops_max = conf->get<uint32_t>("capabilities.disk.hdd.iops_max");
            auto hdd_iops_min = conf->get<uint32_t>("capabilities.disk.hdd.iops_min");
            auto space_reserve = conf->get<float>("capabilities.disk.reserved_space");

            DiskPlatModule* dpm = DiskPlatModule::dsk_plat_singleton();
            auto disk_counts = dpm->disk_counts();

            if (0 == (disk_counts.first + disk_counts.second)) {
                // We don't have real disks
                diskCapability.disk_capacity = 0x7ffff;
                diskCapability.ssd_capacity = 0x10000;
            } else {

                // Calculate aggregate iops with both HDD and SDD
                diskCapability.node_iops_max  = (hdd_iops_max * disk_counts.first);
                diskCapability.node_iops_max += (ssd_iops_max * disk_counts.second);

                diskCapability.node_iops_min  = (hdd_iops_min * disk_counts.first);
                diskCapability.node_iops_min += (ssd_iops_min * disk_counts.second);

                // We assume all disks are connected identically atm
                diskCapability.disk_type = dpm->disk_type() ? FDS_DISK_SAS : FDS_DISK_SATA;

                auto disk_capacities = dpm->disk_capacities();
                diskCapability.disk_capacity = (1.0 - space_reserve) * disk_capacities.first;
                diskCapability.ssd_capacity = (1.0 - space_reserve) * disk_capacities.second;
            }

            if (conf->get<bool>("testing.manual_nodecap",false))
            {
                diskCapability.node_iops_max    = conf->get<int>("testing.node_iops_max", 100000);
                diskCapability.node_iops_min    = conf->get<int>("testing.node_iops_min", 6000);
            }
            LOGDEBUG << "Set node iops max to: " << diskCapability.node_iops_max;
            LOGDEBUG << "Set node iops min to: " << diskCapability.node_iops_min;

            db->setNodeDiskCapability(diskCapability);
        }

        fds_int64_t PlatformManager::getNodeUUID(fpi::FDSP_MgrIdType svcType)
        {
            ResourceUUID    uuid;
            uuid.uuid_set_type(nodeInfo.uuid, svcType);

            return uuid.uuid_get_val();
        }

        int PlatformManager::run()
        {
            while (1)
            {
                sleep(1000);   /* we'll do hotplug uevent thread in here */
            }
            return 0;
        }
    }  // namespace pm
}  // namespace fds
