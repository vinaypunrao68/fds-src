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

    fds_verify(shm_ctrl == NULL);
    shm_ctrl = shm_create_mgr(SHM_INV_FMT, shm_name, shm_total_siz);

    shm = shm_ctrl->shm_attach(PROT_READ | PROT_WRITE);
    if (shm == NULL) {
        shm = shm_ctrl->shm_alloc();
        fds_verify(shm != NULL);

        shm_node_hdr = static_cast<node_shm_inventory_t *>(shm);
        shm_init_header(shm_node_hdr);
    } else {
        shm_node_hdr = static_cast<node_shm_inventory_t *>(shm);
    }
    NodeShmCtrl::mod_startup();
    fds_verify(shm_node_hdr != NULL);

    shm_node_rw = new ShmObjRWKeyUint64(shm_ctrl, shm_node_off,
                            shm_node_hdr->shm_node_inv_key_off,
                            shm_node_hdr->shm_node_inv_obj_siz, shm_max_nodes);

    shm_uuid_rw = new ShmObjRW(shm_ctrl, shm_uuid_off,
                            shm_node_hdr->shm_uuid_bind_key_siz,
                            shm_node_hdr->shm_uuid_bind_key_off,
                            shm_node_hdr->shm_uuid_bind_obj_siz, shm_max_svc);

    shm_dlt_rw      = new ShmObjRW(shm_ctrl, shm_dlt_off, shm_node_hdr->shm_dlt_key_size,
                            shm_node_hdr->shm_dlt_key_off, shm_node_hdr->shm_dlt_size, 1);

    shm_dmt_rw      = new ShmObjRW(shm_ctrl, shm_dmt_off, shm_node_hdr->shm_dmt_key_size,
                            shm_node_hdr->shm_dmt_key_off, shm_node_hdr->shm_dmt_size, 1);

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
    hdr->shm_uuid_bind_key_siz = EP_MAP_REC_KEY_SZ;
    hdr->shm_uuid_bind_obj_siz = sizeof(ep_map_rec_t);

    hdr->shm_am_inv_off     = shm_am_off;
    hdr->shm_am_inv_key_off = offsetof(node_data_t, nd_node_uuid);
    hdr->shm_am_inv_key_siz = sizeof(fds_uint64_t);
    hdr->shm_am_inv_obj_siz = sizeof(node_data_t);

    hdr->shm_dlt_off        = shm_dlt_off;
    hdr->shm_dlt_key_off    = 10;  // not used
    hdr->shm_dlt_key_size   = 10;  // not used
    hdr->shm_dlt_size       = FDS_MAX_DLT_BYTES;

    hdr->shm_dmt_off        = shm_dmt_off;
    hdr->shm_dmt_key_off    = 10;  // not used
    hdr->shm_dmt_key_size   = 10;  // not used
    hdr->shm_dmt_size       = FDS_MAX_DMT_BYTES;
}

void
NodeShmRWCtrl::shm_init_queue(node_shm_queue_t *queue)
{
}

void
NodeShmRWCtrl::shm_setup_queue()
{
    size_t  size;
    void   *area;

    size   = shm_cmd_queue_size();
    shm_rw = shm_create_mgr(SHM_QUEUE_FMT, shm_rw_name, size);
    area   = shm_rw->shm_attach(PROT_READ | PROT_WRITE);
    if (area == NULL) {
        area = shm_rw->shm_alloc();
        fds_assert(area != NULL);
    }
    shm_queue = static_cast<node_shm_queue_t *>(area);
    fds_assert(shm_queue != NULL);

    // --------------------------------
    // INIT SHM CONSTRUCTS
    // --------------------------------
    //
    // SHM CTRL STRUCTURES (INDEXES, # of CONSUMERS)
    // Set the number of consumers to 8
    shm_queue->smq_plat2svc.shm_ncon_cnt = 8;

    // Initialize all consumers index to -1
    for (int i = 0; i < shm_queue->smq_plat2svc.shm_ncon_cnt; ++i) {
        shm_queue->smq_plat2svc.shm_ncon_idx[i] = -1;
    }
    shm_queue->smq_plat2svc.shm_1prd_idx = 0;

    shm_queue->smq_svc2plat.shm_1con_idx = 0;
    shm_queue->smq_svc2plat.shm_nprd_idx = 0;

    // SHM SYNC STRUCTURES (MUTEX & CONDITION VARIABLES)
    int ret = -1;
    // Mutex stuffs
    pthread_mutexattr_t mtx_attr;
    ret = pthread_mutexattr_init(&mtx_attr);
    fds_assert(0 == ret);
    ret = pthread_mutexattr_setpshared(&mtx_attr, PTHREAD_PROCESS_SHARED);
    fds_assert(0 == ret);
    ret = pthread_mutex_init(&shm_queue->smq_sync.shm_mtx, &mtx_attr);
    fds_assert(0 == ret);
    // Condition var stuffs
    pthread_condattr_t cond_attr;
    ret = pthread_condattr_init(&cond_attr);
    fds_assert(0 == ret);
    ret = pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    fds_assert(0 == ret);
    ret = pthread_cond_init(&shm_queue->smq_sync.shm_con_cv, &cond_attr);
    fds_assert(0 == ret);
    ret = pthread_cond_init(&shm_queue->smq_sync.shm_prd_cv, &cond_attr);
    fds_assert(0 == ret);

    ret = pthread_condattr_destroy(&cond_attr);
    fds_assert(0 == ret);
    ret = pthread_mutexattr_destroy(&mtx_attr);
    fds_assert(0 == ret);

    shm_rw_data = new ShmObjRW(shm_rw,
                               NodeShmCtrl::shm_queue_hdr, 4, 0,
                               NodeShmCtrl::shm_q_item_size,
                               NodeShmCtrl::shm_q_item_count);

    /* Platform deamon is the single consumer to n number of FDS service daemons. */
    shm_cons_q = new Shm_nPrd_1Con(&shm_queue->smq_sync,
                                   &shm_queue->smq_svc2plat, shm_rw_data);

    /* Platform daemon is the single producer to n number of consumers. */
    shm_prod_q = new Shm_1Prd_nCon(&shm_queue->smq_sync,
                                   &shm_queue->smq_plat2svc, shm_rw_data);
}

}  // namespace fds
