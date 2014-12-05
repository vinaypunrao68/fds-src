/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include <ep-map.h>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
#include <net/PlatNetSvcHandler.h>

#include "node_shared_memory_rw_ctrl.h"
#include "plat_work_flow.h"

#include "disk_plat_module.h"

#include "plat_agent.h"

#include "platform_net_svc.h"


namespace fds
{
    /*
     * -----------------------------------------------------------------------------------
     * Internal module
     * -----------------------------------------------------------------------------------
     */

    extern EpPlatformdMod    gl_PlatformdShmLib;

    PlatformdNetSvc          gl_PlatformdNetSvc("platformd net");

    PlatformdNetSvc::~PlatformdNetSvc()
    {
    }
    PlatformdNetSvc::PlatformdNetSvc(const char *name) : NetPlatSvc(name)
    {
    }

    // mod_init
    // --------
    //
    int PlatformdNetSvc::mod_init(SysParams const *const p)
    {
        static Module   *platd_net_deps[] = {
            &gl_PlatformdShmLib,
            &gl_PlatWorkFlow,
            NULL
        };

        gl_NetPlatSvc   = this;
        gl_EpShmPlatLib = &gl_PlatformdShmLib;

        /* Init the same code as NetPlatSvc::mod_init()  */
        netmgr          = NetMgr::ep_mgr_singleton();
        plat_lib        = Platform::platf_singleton();
        mod_intern      = platd_net_deps;
        return Module::mod_init(p);
    }

    // mod_startup
    // -----------
    //
    void PlatformdNetSvc::mod_startup()
    {
        Platform           *plat;
        NodeWorkFlow       *work;
        fpi::SvcUuid        om;
        fpi::NodeInfoMsg    msg;

        Module::mod_startup();
        plat_recv   = bo::shared_ptr<PlatformEpHandler>(new PlatformEpHandler(this));
        plat_plugin = new PlatformdPlugin(this);
        plat_ep     = new PlatNetEp(
            plat_lib->plf_get_my_node_port(),
            *plat_lib->plf_get_my_node_uuid(),
            NodeUuid(0ULL),
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_recv)), plat_plugin);

        plat_self   = new PlatAgent(*plat_lib->plf_get_my_node_uuid());
        plat_master = new PlatAgent(gl_OmPmUuid);

        plat_self->init_plat_info_msg(&msg);
        nplat_register_node(&msg, plat_self);

        gl_OmUuid.uuid_assign(&om);
        plat = Platform::platf_singleton();
        work = NodeWorkFlow::nd_workflow_sgt();
        work->wrk_item_create(om, plat_self, plat->plf_node_inventory());

        plat_master->init_om_pm_info_msg(&msg);
        nplat_register_node(&msg, plat_master);

        netmgr->ep_register(plat_ep, false);
        LOGNORMAL << "Startup platform specific net svc, port "
                  << plat_lib->plf_get_my_node_port();
    }

    // mod_enable_service
    // ------------------
    //
    void PlatformdNetSvc::mod_enable_service()
    {
        NetPlatSvc::mod_enable_service();
    }

    // mod_lockstep_start_service
    // --------------------------
    //
    void PlatformdNetSvc::mod_lockstep_start_service()
    {
        /* Search for mod_lockstep_done to see this module's lockstep task. */
    }

    // mod_shutdown
    // ------------
    //
    void PlatformdNetSvc::mod_shutdown()
    {
        Module::mod_shutdown();
    }

    // nplat_register_node
    // -------------------
    //
    void PlatformdNetSvc::nplat_register_node(fpi::NodeInfoMsg *msg, NodeAgent::pointer node)
    {
        int                   idx;
        node_data_t           rec;
        ShmObjRWKeyUint64    *shm;
        NodeAgent::pointer    agent;

        node->node_info_msg_to_shm(msg, &rec);
        shm = NodeShmRWCtrl::shm_node_rw_inv();
        idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                                  static_cast<void *>(&rec), sizeof(rec));
        fds_verify(idx != -1);

        agent = node;
        Platform::platf_singleton()->
        plf_node_inventory()->dc_register_node(shm, &agent, idx, idx, 0);
    }

    // plat_update_local_binding
    // -------------------------
    //
    void PlatformdNetSvc::plat_update_local_binding(const ep_map_rec_t *rec)
    {
    }

    // nplat_peer
    // ----------
    //
    EpSvcHandle::pointer PlatformdNetSvc::nplat_peer(const fpi::SvcUuid &uuid)
    {
        return NULL;
    }

    // nplat_peer
    // ----------
    //
    EpSvcHandle::pointer PlatformdNetSvc::nplat_peer(const fpi::DomainID &id,
                                                     const fpi::SvcUuid &uuid)
    {
        return NULL;
    }

    // nplat_register_node
    // -------------------
    // Platform daemon registers this node info from the network packet.
    //
    void PlatformdNetSvc::nplat_register_node(const fpi::NodeInfoMsg *msg)
    {
        node_data_t    rec;

        // Notify all services about this node through shared memory queue.
        NodeInventory::node_info_msg_to_shm(msg, &rec);
        EpPlatformdMod::ep_shm_singleton()->node_reg_notify(&rec);
    }

    // nplat_refresh_shm
    // -----------------
    //
    void PlatformdNetSvc::nplat_refresh_shm()
    {
        std::cout << "Platform daemon refresh shm" << std::endl;
    }

    bo::shared_ptr<PlatNetSvcHandler> PlatformdNetSvc::getPlatNetSvcHandler()
    {
        return plat_recv;
    }
}  // namespace fds
