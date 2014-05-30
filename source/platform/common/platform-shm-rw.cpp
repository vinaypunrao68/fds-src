/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
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
    shm_flags   = PROT_READ | PROT_WRITE;
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

    NodeShmCtrl::mod_startup();
    fds_verify(shm_ctrl != NULL);
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

}  // namespace fds
