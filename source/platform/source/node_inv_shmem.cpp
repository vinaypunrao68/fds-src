/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/platform-lib.h>
#include <platform/fds_shmem.h>
#include <ep-map.h>

#include "platform/platform.h"
#include "platform/node_shm_inventory.h"
#include "platform/node_shm_queue.h"

namespace fds
{

    NodeShmCtrl    gl_NodeShmROCtrl("NodeShm");
    NodeShmCtrl   *gl_NodeShmCtrl = &gl_NodeShmROCtrl;

    /*
     * --------------------------------------------------------------------------------------
     * Node Shared Memory Layout
     *
     *   +----------------------------+
     *   |        shm_node_hdr        |
     *   +----------------------------+
     *   |    Node inventory data     |
     *   +----------------------------+
     *   |    Service UUID binding    |
     *   +----------------------------+
     *   |   AM node inventory data   |
     *   +----------------------------+
     *
     * Shared Memory Queue Segment
     *   +----------------------------+
     *   |      node_shm_queue_t      |
     *   +----------------------------+
     *   |    node_shm_queue_item_t   |
     *   |            ....            |
     *   |    node_shm_queue_item_t   |
     *   +----------------------------+
     *
     * --------------------------------------------------------------------------------------
     */

    NodeShmCtrl::NodeShmCtrl(const char *name) : Module(name)
    {
        shm_node_off  = sizeof(*shm_node_hdr);
        shm_node_siz  = sizeof(node_data_t) * shm_max_nodes;
        shm_total_siz = sizeof(*shm_node_hdr) + shm_node_siz;

        shm_uuid_off  = shm_node_off + shm_node_siz;
        shm_uuid_siz  = sizeof(ep_map_rec_t) * shm_max_svc;
        shm_total_siz += shm_uuid_siz;

        shm_am_off    = shm_uuid_off + shm_uuid_siz;
        shm_am_siz    = sizeof(node_data_t) * shm_max_ams;
        shm_total_siz +=  shm_am_siz;

        shm_dlt_off   = shm_am_off + shm_am_siz;
        shm_dlt_size  = FDS_MAX_DLT_BYTES;
        shm_total_siz += shm_dlt_size;

        shm_dmt_off   = shm_dlt_off + shm_dlt_size;
        shm_dmt_size  = FDS_MAX_DMT_BYTES;
        shm_total_siz += shm_dmt_size;

        shm_rw_name[0] = '\0';
        shm_rw        = NULL;
        shm_rw_data   = NULL;
        shm_cons_q    = NULL;
        shm_prod_q    = NULL;
        shm_queue     = NULL;
        shm_cons_thr  = NULL;

        shm_name[0]   = '\0';
        shm_am_inv    = NULL;
        shm_node_inv  = NULL;
        shm_uuid_bind = NULL;
        shm_node_hdr  = NULL;
        shm_ctrl      = NULL;
    }

    // mod_init
    // --------
    //
    int NodeShmCtrl::mod_init(SysParams const *const p)
    {
        return Module::mod_init(p);
    }

    // mod_startup
    // -----------
    //
    void NodeShmCtrl::mod_startup()
    {
        Module::mod_startup();

        if (shm_ctrl == NULL)
        {
            fds_verify(shm_total_siz > 0);
            shm_ctrl     = shm_create_mgr(SHM_INV_FMT, shm_name, shm_total_siz);
            void   *shm    = shm_ctrl->shm_attach(PROT_READ);
            shm_node_hdr = static_cast<node_shm_inventory_t *>(shm);
        }

        fds_verify(shm_node_hdr != NULL);
        fds_verify(shm_node_hdr->shm_node_inv_off == shm_node_off);
        fds_verify(shm_node_hdr->shm_node_inv_obj_siz == sizeof(node_data_t));

        fds_verify(shm_node_hdr->shm_uuid_bind_off == shm_uuid_off);
        fds_verify(shm_node_hdr->shm_uuid_bind_obj_siz == sizeof(ep_map_rec_t));

        fds_verify(shm_node_hdr->shm_am_inv_off == shm_am_off);
        fds_verify(shm_node_hdr->shm_am_inv_obj_siz == sizeof(node_data_t));

        shm_node_inv  =
            new ShmObjROKeyUint64(shm_ctrl, shm_node_off, shm_node_hdr->shm_node_inv_key_off,
                                  shm_node_hdr->shm_node_inv_obj_siz,
                                  shm_max_nodes);

        shm_uuid_bind =
            new ShmObjRO(shm_ctrl, shm_uuid_off, shm_node_hdr->shm_uuid_bind_key_siz,
                         shm_node_hdr->shm_uuid_bind_key_off, shm_node_hdr->shm_uuid_bind_obj_siz,
                         shm_max_svc);

        shm_dlt      =
            new ShmObjRO(shm_ctrl, shm_dlt_off, shm_node_hdr->shm_dlt_key_size,
                         shm_node_hdr->shm_dlt_key_off, shm_node_hdr->shm_dlt_size,
                         1);

        shm_dmt      =
            new ShmObjRO(shm_ctrl, shm_dmt_off, shm_node_hdr->shm_dmt_key_size,
                         shm_node_hdr->shm_dmt_key_off, shm_node_hdr->shm_dmt_size,
                         1);

        shm_am_inv    =
            new ShmObjROKeyUint64(shm_ctrl, shm_am_off, shm_node_hdr->shm_am_inv_key_off,
                                  shm_node_hdr->shm_am_inv_obj_siz,
                                  shm_max_ams);

        shm_setup_queue();
    }

    // mod_shutdown
    // ------------
    //
    void NodeShmCtrl::mod_shutdown()
    {
        Module::mod_shutdown();
    }

    // shm_create_mgr
    // --------------
    // Common function to create a shared memory segment.
    //
    FdsShmem *NodeShmCtrl::shm_create_mgr(const char *fmt, char *name, int shm_size)
    {
        snprintf(name, FDS_MAX_UUID_STR, fmt,
                 Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_get_val(), 0);

        return new FdsShmem(name, shm_size);
    }

    // shm_cmd_queue_size
    // ------------------
    //
    size_t NodeShmCtrl::shm_cmd_queue_size()
    {
        size_t    shm_size;

        shm_size  = NodeShmCtrl::shm_queue_hdr;
        shm_size += (NodeShmCtrl::shm_q_item_size * NodeShmCtrl::shm_q_item_count);

        return shm_size;
    }

    // shm_setup_queue
    // ---------------
    //
    void NodeShmCtrl::shm_setup_queue()
    {
        void   *shm;

        shm_rw = shm_create_mgr(SHM_QUEUE_FMT, shm_rw_name, shm_cmd_queue_size());
        shm    = shm_rw->shm_attach(PROT_READ | PROT_WRITE);

        shm_queue = static_cast<node_shm_queue_t *>(shm);
        fds_assert(shm_queue != NULL);

        shm_rw_data =
            new ShmObjRW(shm_rw, NodeShmCtrl::shm_queue_hdr, 4, 0, NodeShmCtrl::shm_q_item_size,
                         NodeShmCtrl::shm_q_item_count);

        /* Platform lib linked with n producers to 1 platform daemon consumer. */
        shm_prod_q = new Shm_nPrd_1Con(&shm_queue->smq_sync, &shm_queue->smq_svc2plat, shm_rw_data);

        /* Platform lib is n consumers from 1 platform daemon producer. */
        shm_cons_q = new Shm_1Prd_nCon(&shm_queue->smq_sync, &shm_queue->smq_plat2svc, shm_rw_data);
    }
}  // namespace fds
