/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <dlt.h>
#include <fds_assert.h>
#include <platform/fds-shmem.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
#include <fds-shmobj.h>
#include <ep-map.h>

namespace fds {

NodeShmCtrl                  gl_NodeShmROCtrl("NodeShm");
NodeShmCtrl                 *gl_NodeShmCtrl = &gl_NodeShmROCtrl;

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
 * --------------------------------------------------------------------------------------
 */
NodeShmCtrl::NodeShmCtrl(const char *name) : Module(name)
{
    shm_flags     = PROT_READ;
    shm_node_off  = sizeof(*shm_node_hdr);
    shm_node_siz  = sizeof(node_data_t) * shm_max_nodes;
    shm_total_siz = sizeof(*shm_node_hdr) + shm_node_siz;

    shm_uuid_off  = shm_node_off + shm_node_siz;
    shm_uuid_siz  = sizeof(ep_map_rec_t) * shm_max_svc;
    shm_total_siz = shm_total_siz + shm_uuid_siz;

    shm_am_off    = shm_uuid_off + shm_uuid_siz;
    shm_am_siz    = sizeof(node_data_t) * shm_max_ams;
    shm_total_siz = shm_total_siz + shm_am_siz;

    shm_am_inv    = NULL;
    shm_node_inv  = NULL;
    shm_uuid_bind = NULL;
    shm_node_hdr  = NULL;
    shm_ctrl      = NULL;
}

// shm_init_header
// ---------------
//
void
NodeShmCtrl::shm_init_header(node_shm_inventory_t *hdr)
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

// mod_init
// --------
//
int
NodeShmCtrl::mod_init(SysParams const *const p)
{
    return Module::mod_init(p);
}

// mod_startup
// -----------
//
void
NodeShmCtrl::mod_startup()
{
    void  *shm;

    Module::mod_startup();
    fds_verify(shm_total_siz > 0);
    snprintf(shm_name, FDS_MAX_UUID_STR, "/0x%llx-%d",
             Platform::platf_singleton()->plf_my_node_uuid().uuid_get_val(), 0);
    shm_ctrl = new FdsShmem(shm_name, shm_total_siz);

    shm = shm_ctrl->shm_attach(shm_flags);
    if (shm == NULL) {
        shm = shm_ctrl->shm_alloc(shm_total_siz);
        fds_verify(shm != NULL);

        std::cout << "Alloc " << shm_total_siz << " bytes in shmem" << std::endl;
        shm_node_hdr = static_cast<node_shm_inventory_t *>(shm);
        shm_init_header(shm_node_hdr);
    } else {
        shm_node_hdr = static_cast<node_shm_inventory_t *>(shm);
        fds_verify(shm_node_hdr->shm_node_inv_off == shm_node_off);
        fds_verify(shm_node_hdr->shm_node_inv_obj_siz == sizeof(node_data_t));

        fds_verify(shm_node_hdr->shm_uuid_bind_off == shm_uuid_off);
        fds_verify(shm_node_hdr->shm_uuid_bind_obj_siz == sizeof(ep_map_rec_t));

        fds_verify(shm_node_hdr->shm_am_inv_off == shm_am_off);
        fds_verify(shm_node_hdr->shm_am_inv_obj_siz == sizeof(node_data_t));
    }
    shm_node_inv  = new ShmObjROKeyUint64(shm_ctrl, shm_node_off,
                            shm_node_hdr->shm_node_inv_key_off,
                            shm_node_hdr->shm_node_inv_obj_siz, shm_max_nodes);

    shm_uuid_bind = new ShmObjROKeyUint64(shm_ctrl, shm_uuid_off,
                            shm_node_hdr->shm_uuid_bind_key_off,
                            shm_node_hdr->shm_uuid_bind_obj_siz, shm_max_svc);

    shm_am_inv    = new ShmObjROKeyUint64(shm_ctrl, shm_am_off,
                            shm_node_hdr->shm_am_inv_key_off,
                            shm_node_hdr->shm_am_inv_obj_siz, shm_max_ams);
}

// mod_shutdown
// ------------
//
void
NodeShmCtrl::mod_shutdown()
{
    Module::mod_shutdown();
}

#if 0
ShmNodeInv *gl_ShmNodeInv;

NodeInvData::~NodeInvData() {}
NodeInvData::NodeInvData(const node_data_t *shm) : nd_shm(shm)
{
    nd_my_gb_cap      = 0;
    nd_my_dlt_version = DLT_VER_INVALID;
    nd_my_node_state  = fpi::FDS_Node_Down;
}

// Nothing for obj accessing the RO shared mem segment.
//
void
NodeInvData::node_update_inventory(const FdspNodeRegPtr msg) {}

// nd_node_state
// -------------
//
FdspNodeState
NodeInvData::nd_node_state() const
{
    if (nd_my_node_state == fpi::FDS_Node_Down) {
        return nd_shm->nd_node_state;
    }
    return nd_my_node_state;
}

// nd_dlt_version
// --------------
//
fds_uint64_t
NodeInvData::nd_dlt_version() const
{
    if (nd_my_dlt_version == DLT_VER_INVALID) {
        return nd_shm->nd_dlt_version;
    }
    return nd_my_dlt_version;
}

// nd_gbyte_cap
// ------------
//
fds_uint64_t
NodeInvData::nd_gbyte_cap() const
{
    if (nd_my_gb_cap == 0) {
        // Vy: still hacking the capacity size for now.
        return nd_shm->nd_capability.ssd_capacity;
    }
    return nd_my_gb_cap;
}

// node_set_gbyte_cap
// ------------------
//
void
NodeInvData::node_set_gbyte_cap(fds_uint64_t cap)
{
    nd_my_gb_cap = cap;
}

OwnerNodeInvData::~OwnerNodeInvData() {}
OwnerNodeInvData::OwnerNodeInvData(node_data_t *shm)
    : NodeInvData(shm), nd_rw_shm(shm) {}

// node_update_inventory
// ---------------------
//
void
OwnerNodeInvData::node_update_inventory(const FdspNodeRegPtr msg)
{
    node_data_t       *cur;
    node_capability_t *ncap;

    cur                    = nd_rw_shm;
    cur->nd_ip_addr        = msg->ip_lo_addr;
    cur->nd_data_port      = msg->data_port;
    cur->nd_ctrl_port      = msg->control_port;
    cur->nd_migration_port = msg->migration_port;
    cur->nd_node_type      = msg->node_type;
    cur->nd_node_state     = fpi::FDS_Node_Discovered;
    cur->nd_disk_type      = msg->disk_info.disk_type;
    cur->nd_dlt_version    = DLT_VER_INVALID;

    ncap                   = &nd_rw_shm->nd_capability;
    ncap->disk_capacity    = msg->disk_info.disk_capacity;
    ncap->disk_iops_max    = msg->disk_info.disk_iops_max;
    ncap->disk_iops_min    = msg->disk_info.disk_iops_min;
    ncap->disk_latency_max = msg->disk_info.disk_latency_max;
    ncap->disk_latency_min = msg->disk_info.disk_latency_min;
    ncap->ssd_iops_max     = msg->disk_info.ssd_iops_max;
    ncap->ssd_iops_min     = msg->disk_info.ssd_iops_min;
    ncap->ssd_capacity     = msg->disk_info.ssd_capacity;
    ncap->ssd_latency_max  = msg->disk_info.ssd_latency_max;
    ncap->ssd_latency_min  = msg->disk_info.ssd_latency_min;
    strncpy(cur->nd_node_name, msg->node_name.c_str(), FDS_MAX_NODE_NAME);
}

#endif
}  // namespace fds
