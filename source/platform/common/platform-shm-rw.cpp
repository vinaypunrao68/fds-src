/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform.h>
#include <fds-shmobj.h>
#include <platform/fds-shmem.h>

namespace fds {

NodeShmRWCtrl   gl_NodeShmRWCtrl("NodeShmRW");

/*
 * --------------------------------------------------------------------------------------
 * Node Shared Memory Controller
 * --------------------------------------------------------------------------------------
 */
NodeShmRWCtrl::NodeShmRWCtrl(const char *name) : NodeShmCtrl(name)
{
    shm_am_rw   = NULL;
    shm_node_rw = NULL;
    shm_uuid_rw = NULL;
}

// mod_init
// --------
//
int
NodeShmRWCtrl::mod_init(SysParams const *const p)
{
    gl_NodeShmCtrl = &gl_NodeShmRWCtrl;
    return NodeShmCtrl::mod_init(p);
}

// mod_startup
// -----------
//
void
NodeShmRWCtrl::mod_startup()
{
    void  *shm;

    fds_verify(shm_ctrl != NULL);
    shm = shm_ctrl->shm_attach(PROT_READ | PROT_WRITE);
    if (shm == NULL) {
        shm = shm_ctrl->shm_alloc(shm_total_siz);
        fds_verify(shm != NULL);
        shm_node_hdr = static_cast<node_shm_inventory_t *>(shm);
        shm_init_header(shm_node_hdr);
    }
    NodeShmCtrl::mod_startup();
    fds_verify(shm_node_hdr != NULL);

    shm_node_rw = new ShmObjRWKeyUint64(shm_ctrl, shm_node_off,
                            shm_node_hdr->shm_node_inv_key_off,
                            shm_node_hdr->shm_node_inv_obj_siz, shm_max_nodes);

    shm_uuid_rw = new ShmObjRWKeyUint64(shm_ctrl, shm_uuid_off,
                            shm_node_hdr->shm_uuid_bind_key_off,
                            shm_node_hdr->shm_uuid_bind_obj_siz, shm_max_svc);

    shm_am_rw   = new ShmObjRWKeyUint64(shm_ctrl, shm_am_off,
                            shm_node_hdr->shm_am_inv_key_off,
                            shm_node_hdr->shm_am_inv_obj_siz, shm_max_ams);
}

// mod_shutdown
// ------------
//
void
NodeShmRWCtrl::mod_shutdown()
{
    NodeShmCtrl::mod_shutdown();
}

// shm_init_header
// ---------------
//
void
NodeShmRWCtrl::shm_init_header(node_shm_inventory_t *hdr)
{
    hdr->shm_magic   = 0xFEEDCAFE;
    hdr->shm_version = 0;
    hdr->shm_state   = NODE_SHM_ACTIVE;

    hdr->shm_node_inv_off     = shm_node_off;
    hdr->shm_node_inv_key_off = offsetof(node_data_t, nd_node_uuid);
    hdr->shm_node_inv_key_siz = sizeof(fds_uint64_t);
    hdr->shm_node_inv_obj_siz = sizeof(node_data_t);

    hdr->shm_uuid_bind_off     = shm_uuid_off;
    hdr->shm_uuid_bind_key_off = offsetof(ep_map_rec_t, rmp_uuid);
    hdr->shm_uuid_bind_key_siz = sizeof(fds_uint64_t);
    hdr->shm_uuid_bind_obj_siz = sizeof(ep_map_rec_t);

    hdr->shm_am_inv_off     = shm_am_off;
    hdr->shm_am_inv_key_off = offsetof(node_data_t, nd_node_uuid);
    hdr->shm_am_inv_key_siz = sizeof(fds_uint64_t);
    hdr->shm_am_inv_obj_siz = sizeof(node_data_t);
}

void
NodeShmRWCtrl::shm_init_queue(node_shm_queue_t *queue)
{
}

void
NodeShmRWCtrl::shm_setup_queue()
{
}

}  // namespace fds
