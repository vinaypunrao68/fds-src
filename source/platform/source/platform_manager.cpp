/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>  // NOLINT

#include <fds_uuid.h>
#include <fdsp/svc_types_types.h>
#include <fds_process.h>
#include <platform/process.h>
#include <util/stringutils.h>

#include "platform/platform_manager.h"

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
            util::Properties props = util::Properties(data);
            props.set("fds_root", rootDir);
            props.setInt("uuid", nodeInfo.uuid);
            props.setInt("disk_iops_max", diskCapability.disk_iops_max);
            props.setInt("disk_iops_min", diskCapability.disk_iops_min);
            props.setDouble("disk_capacity", diskCapability.disk_capacity);
            props.setInt("disk_latency_max", diskCapability.disk_latency_max);
            props.setInt("disk_latency_min", diskCapability.disk_latency_min);
            props.setInt("ssd_iops_max", diskCapability.ssd_iops_max);
            props.setInt("ssd_iops_min", diskCapability.ssd_iops_min);
            props.setDouble("ssd_capacity", diskCapability.ssd_capacity);
            props.setInt("ssd_latency_max", diskCapability.ssd_latency_max);
            props.setInt("ssd_latency_min", diskCapability.ssd_latency_min);
            props.setInt("disk_type", diskCapability.disk_type);
        }

        // TODO: this needs to populate real data from the disk module labels etc.
        // it may want to load the value from the database and validate it against
        // DiskPlatModule data, or just load from the DiskPlatModule and be done
        // with it.  Depends somewhat on how expensive it is to traverse the DiskPlatModule
        // and calculate all the data.
        void PlatformManager::determineDiskCapability()
        {
            diskCapability.disk_iops_max    = 100000;
            diskCapability.disk_iops_min    = 4000;

            if (conf->get<bool>("testing.manual_nodecap",false))
            {
                diskCapability.disk_iops_max    = conf->get<int>("testing.disk_iops_max", 100000);
                diskCapability.disk_iops_min    = conf->get<int>("testing.disk_iops_min", 6000);
            }
            diskCapability.disk_capacity    = 0x7ffff;
            diskCapability.disk_latency_max = 1000000 / diskCapability.disk_iops_min;
            diskCapability.disk_latency_min = 1000000 / diskCapability.disk_iops_max;
            diskCapability.ssd_iops_max     = 300000;
            diskCapability.ssd_iops_min     = 1000;
            diskCapability.ssd_capacity     = 0x10000;
            diskCapability.ssd_latency_max  = 1000000 / diskCapability.ssd_iops_min;
            diskCapability.ssd_latency_min  = 1000000 / diskCapability.ssd_iops_max;
            diskCapability.disk_type        = FDS_DISK_SATA;
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
