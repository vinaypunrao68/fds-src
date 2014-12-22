/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <ep-map.h>

#include "platform/platform.h"
#include "platform/node_agent.h"
#include "platform/domain_container.h"

#include "plat_uuid_bind.h"
#include "node_shm_rw_ctrl.h"

namespace fds
{
    /* static  */ EpPlatformdMod    gl_PlatformdShmLib("Platformd Shm Lib");

    static PlatUuidBind   *platform_uuid_bind;

    // ep_shm_singleton
    // ----------------
    //
    /* static */ EpPlatformdMod *EpPlatformdMod::ep_shm_singleton()
    {
        return &gl_PlatformdShmLib;
    }

    /*
     * -------------------------------------------------------------------------------------
     * Platform NetEndPoint services
     * -------------------------------------------------------------------------------------
     */
    EpPlatformdMod::EpPlatformdMod(const char *name) : EpPlatLibMod(name)
    {
    }

    // mod_init
    // --------
    //
    int EpPlatformdMod::mod_init(SysParams const *const arg)
    {
        static Module   *ep_plat_dep_mods[] = {
            &gl_NodeShmRWCtrl,
            NULL
        };
        mod_intern = ep_plat_dep_mods;
        platform_uuid_bind = new PlatUuidBind(this);
        return EpPlatLibMod::mod_init(arg);
    }

    // mod_startup
    // -----------
    //
    void EpPlatformdMod::mod_startup()
    {
        Module::mod_startup();
        ep_uuid_rw   = NodeShmRWCtrl::shm_uuid_rw_binding();
        ep_uuid_bind = NodeShmRWCtrl::shm_uuid_binding();
        ep_node_rw   = NodeShmRWCtrl::shm_node_rw_inv();
        ep_node_bind = NodeShmRWCtrl::shm_node_inventory();
    }

    // mod_enable_service
    // ------------------
    //
    void EpPlatformdMod::mod_enable_service()
    {
        ShmConPrdQueue   *consumer;

        consumer = NodeShmCtrl::shm_consumer();
        consumer->shm_register_handler(SHMQ_REQ_UUID_BIND, platform_uuid_bind);
        consumer->shm_register_handler(SHMQ_REQ_UUID_UNBIND, platform_uuid_bind);
    }

    // ep_map_record
    // -------------
    //
    int EpPlatformdMod::ep_map_record(const ep_map_rec_t *rec)
    {
        return ep_uuid_rw->shm_insert_rec(static_cast<const void *>(&rec->rmp_uuid),
                                          static_cast<const void *>(rec), sizeof(*rec));
    }

    // ep_unmap_record
    // ---------------
    //
    int EpPlatformdMod::ep_unmap_record(fds_uint64_t uuid, int idx)
    {
        return ep_uuid_rw->shm_remove_rec(idx, static_cast<const void *>(&uuid), NULL, 0);
    }

    // node_reg_notify
    // ---------------
    //
    void EpPlatformdMod::node_reg_notify(const node_data_t *info)
    {
        int                         idx;
        ep_map_rec_t                map;
        ep_shmq_node_req_t          out;
        ShmConPrdQueue             *plat;
        ShmObjRWKeyUint64          *shm;
        NodeAgent::pointer          agent;
        DomainContainer::pointer    local;

        if (info->nd_node_uuid == Platform::plf_get_my_node_uuid()->uuid_get_val())
        {
            LOGDEBUG << "Skip registration notify for my node uuid";
            return;
        }
        /* Save the node_info binding to shared memory. */
        shm = NodeShmRWCtrl::shm_node_rw_inv(info->nd_svc_type);
        idx =
            shm->shm_insert_rec(static_cast<const void *>(&info->nd_node_uuid),
                                reinterpret_cast<const void *>(info), sizeof(*info));

        /* Cache the binding info. */
        fds_verify(idx >= 0);
        EpPlatLibMod::ep_node_info_to_mapping(info, &map);
        NetMgr::ep_mgr_singleton()->ep_register_binding(&map, idx);

        /* Notify all services running in the same node. */
        out.smq_idx          = idx;
        out.smq_type         = info->nd_svc_type;
        out.smq_node_uuid    = info->nd_node_uuid;
        out.smq_svc_uuid     = info->nd_service_uuid;
        out.smq_hdr.smq_code = SHMQ_NODE_REGISTRATION;

        /* Allocate node agents as proxy to this node and its services. */
        agent = NULL;
        local = Platform::platf_singleton()->plf_node_inventory();
        local->dc_register_node(shm, &agent, idx, idx, NODE_DO_PROXY_ALL_SVCS);

        plat = NodeShmCtrl::shm_producer();
        plat->shm_producer(static_cast<void *>(&out), sizeof(out), 0);
    }
}  // namespace fds
