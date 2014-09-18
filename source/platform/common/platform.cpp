/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <disk.h>
#include <platform.h>
#include <fds_process.h>
#include <net-platform.h>
#include <net/net_utils.h>
#include <platform/node-inv-shmem.h>
#include <ep-map.h>
#include <util/timeutils.h>

namespace fds {

NodePlatform    gl_NodePlatform;

// -------------------------------------------------------------------------------------
// Node Specific Platform
// -------------------------------------------------------------------------------------
NodePlatform::NodePlatform()
    : Platform("Node-Platform",
               fpi::FDSP_PLATFORM,
               new DomainNodeInv("Node-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PlatAgentContainer(),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainClusterMap("Node-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PlatAgentContainer(),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainResources("Node-Resources"),
               NULL) {}

int
NodePlatform::mod_init(SysParams const *const param)
{
    plf_node_type = fpi::FDSP_PLATFORM;
    Platform::platf_assign_singleton(&gl_NodePlatform);

    Platform::mod_init(param);
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    disk_ctrl        = DiskPlatModule::dsk_plat_singleton();
    plf_my_ip        = net::get_local_ip(conf.get_abs<std::string>("fds.nic_if"));
    plf_my_node_name = conf.get_abs<std::string>("fds.plat.id", "auto");
    return 0;
}

void
NodePlatform::mod_startup()
{
    Platform::mod_startup();
}

// mod_enable_service
// ------------------
// The Platform::mod_enable_service is common for platform lib.  We are running in
// platform daemon, which required different logic for the same API.
//
void
NodePlatform::mod_enable_service()
{
    /* Create OM Agent and bind it to its known IP. */
    plf_bind_om_node();

    NodeShmCtrl::shm_ctrl_singleton()->shm_start_consumer_thr(plf_node_type);
    Module::mod_enable_service();
}

void
NodePlatform::mod_shutdown()
{
    Platform::mod_shutdown();
}

void
NodePlatform::plf_bind_om_node()
{
    int                 idx;
    node_data_t         rec;
    fpi::NodeInfoMsg    msg;
    ShmObjRWKeyUint64  *shm;
    NodeAgent::pointer  agent;

    if (plf_master != NULL) {
        return;
    }
    /*  Well-known binding for OM services (e.g. logical). */
    plf_master = new OmAgent(gl_OmUuid);
    plf_master->init_om_info_msg(&msg);
    plf_master->node_info_msg_to_shm(&msg, &rec);

    /*  Fix up the record for OM. */
    rec.nd_svc_type = fpi::FDSP_ORCH_MGR;

    shm = NodeShmRWCtrl::shm_node_rw_inv();
    idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                              static_cast<void *>(&rec), sizeof(rec));
    fds_verify(idx != -1);

    agent = plf_master;
    plf_node_inv->dc_register_node(shm, &agent, idx, idx, 0);
}

void
NodePlatform::plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg)
{
    plf_process->plf_start_node_services(msg);
}

/**
 * Required factory methods.
 */

}  // namespace fds
