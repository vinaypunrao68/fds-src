/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>  // NOLINT

#include <fds_uuid.h>
#include <net/net-service.h>
#include <fds_process.h>
#include <platform/process.h>
#include <util/stringutils.h>
#include <platform/platformmanager.h>
namespace fds {
namespace pm {

PlatformManager::PlatformManager() : Module("pm") {
}

int  PlatformManager::mod_init(SysParams const *const param) {
    conf = new FdsConfigAccessor(g_fdsprocess->get_conf_helper());
    rootDir = g_fdsprocess->proc_fdsroot()->dir_fdsroot();
    db = new kvstore::PlatformDB(conf->get<std::string>("redis_host","localhost"),
                                 conf->get<int>("redis_port", 6379),1);

    if (!db->isConnected()) {
        LOGCRITICAL << "unable to talk to redis @ "
                    << "[" << conf->get<std::string>("redis_host","localhost")
                    << ":" << conf->get<int>("redis_port", 6379)
                    << "]";
    } else {
        db->getNodeInfo(nodeInfo);
        db->getNodeDiskCapability(diskCapability);
    }
    
    if (nodeInfo.uuid <= 0) {
        NodeUuid    uuid(fds_get_uuid64(get_uuid()));
        nodeInfo.uuid = uuid.uuid_get_val();
        LOGNOTIFY << "generated a new uuid for this node : " << nodeInfo.uuid;
        db->setNodeInfo(nodeInfo);
    }

    determineDiskCapability();
    return 0;
}

void PlatformManager::mod_shutdown() {

}


pid_t PlatformManager::startAM() {
    std::vector<std::string>    args;
    pid_t                pid;

    args.push_back(util::strformat("--fds.pm.platform_port=%d",conf->get<int>("platform_port")));
    args.push_back(std::string("--fds.pm.om_ip=") + conf->get<std::string>("om_ip"));
    args.push_back(std::string("--fds.am.om_ip=") + conf->get<std::string>("om_ip"));
    args.push_back(util::strformat("--fds.dm.svc.uuid=%lld", nodeInfo.uuid));
    
    pid = fds_spawn_service("AMAgent", rootDir, args, true);
    if (pid > 0) {
        LOGNOTIFY << "Spawn AM as daemon";
    } else {
        LOGCRITICAL << "error spawning AM";
    }

    return pid;
}

pid_t PlatformManager::startDM() {
    std::vector<std::string>    args;
    pid_t                pid;

    args.push_back(util::strformat("--fds.pm.platform_port=%d",conf->get<int>("platform_port")));
    args.push_back(std::string("--fds.pm.om_ip=") + conf->get<std::string>("om_ip"));
    args.push_back(std::string("--fds.dm.om_ip=") + conf->get<std::string>("om_ip"));
    args.push_back(util::strformat("--fds.dm.svc.uuid=%lld", nodeInfo.uuid));
    
    pid = fds_spawn_service("DataMgr", rootDir, args, true);

    if (pid > 0) {
        LOGNOTIFY << "Spawn DM as daemon";
        db->setNodeInfo(nodeInfo);
    } else {
        LOGCRITICAL << "error spawning DM";
    }

    return pid;
}

pid_t PlatformManager::startSM() {
    std::vector<std::string>    args;
    pid_t                pid;

    args.push_back(util::strformat("--fds.pm.platform_port=%d",conf->get<int>("platform_port")));
    args.push_back(std::string("--fds.pm.om_ip=") + conf->get<std::string>("om_ip"));
    args.push_back(std::string("--fds.sm.om_ip=") + conf->get<std::string>("om_ip"));
    args.push_back(util::strformat("--fds.sm.svc.uuid=%lld", nodeInfo.uuid));
    pid = fds_spawn_service("StorMgr", rootDir, args, true);
    if (pid > 0) {
        LOGNOTIFY << "Spawn SM as daemon";
    } else {
        LOGCRITICAL << "error spawning SM";
    }
    return pid;
}

// plf_start_node_services
// -----------------------
//
void PlatformManager::activateServices(const fpi::FDSP_ActivateNodeTypePtr &msg) {
    pid_t                pid;
    bool                 auto_start;

    if (msg->has_sm_service) {
        nodeInfo.fHasAm = true;
        startSM();
    }

    if (msg->has_dm_service) {
        nodeInfo.fHasDm = true;
        startDM();
    }
    
    if (msg->has_am_service) {
        nodeInfo.fHasAm = true;
        startAM();
    }

    db->setNodeInfo(nodeInfo);
}

void PlatformManager::determineDiskCapability() {
    diskCapability.disk_iops_max    = 10000;
    diskCapability.disk_iops_min    = 4000;
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

bool PlatformManager::sendNodeCapabilityToOM() {
    fpi::FDSP_RegisterNodeTypePtr    pkt(new fpi::FDSP_RegisterNodeType);
    pkt->disk_info = diskCapability;

    if (conf->get<bool>("testing.manual_nodecap")) {
        pkt->disk_info.disk_iops_min = conf->get<int>("testing.disk_iops_min");
        pkt->disk_info.disk_iops_max = conf->get<int>("testing.disk_iops_max");
    }
    // do the send

    return true;
}

int PlatformManager::run() {
    sendNodeCapabilityToOM();
    while (1) {
        sleep(1000);   /* we'll do hotplug uevent thread in here */
    }
    return 0;
}
}  // namespace pm
}  // namespace fds
