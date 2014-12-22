/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_SHM_INVENTORY_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_SHM_INVENTORY_H_

namespace fds
{
    /**
     * Node inventory shared memory segment layout
     */
    typedef struct node_shm_inventory
    {
        fds_uint32_t shm_magic;
        fds_uint32_t shm_version;
        node_shm_state_e shm_state;

        fds_uint32_t shm_node_inv_off;
        fds_uint32_t shm_node_inv_key_off;
        fds_uint32_t shm_node_inv_key_siz;
        fds_uint32_t shm_node_inv_obj_siz;

        fds_uint32_t shm_uuid_bind_off;
        fds_uint32_t shm_uuid_bind_key_off;
        fds_uint32_t shm_uuid_bind_key_siz;
        fds_uint32_t shm_uuid_bind_obj_siz;

        fds_uint32_t shm_am_inv_off;
        fds_uint32_t shm_am_inv_key_off;
        fds_uint32_t shm_am_inv_key_siz;
        fds_uint32_t shm_am_inv_obj_siz;

        fds_uint32_t shm_dlt_off;
        fds_uint32_t shm_dlt_key_off;
        fds_uint32_t shm_dlt_key_size;
        fds_uint32_t shm_dlt_size;

        fds_uint32_t shm_dmt_off;
        fds_uint32_t shm_dmt_key_off;
        fds_uint32_t shm_dmt_key_size;
        fds_uint32_t shm_dmt_size;
    } node_shm_inventory_t;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_SHM_INVENTORY_H_
