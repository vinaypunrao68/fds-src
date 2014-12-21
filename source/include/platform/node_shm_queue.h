/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_SHM_QUEUE_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_SHM_QUEUE_H_

namespace fds
{
    /**
     * Producer/consumer queue in shared memory of a node.
     */
    typedef struct node_shm_queue
    {
        fds_uint32_t smq_hdr_size;
        fds_uint32_t smq_item_size;
        shm_con_prd_sync_t smq_sync;

        /*   Must be together for proper memory layout */
        shm_nprd_1con_q_t smq_svc2plat;        /* *< multiple svcs to 1 platformd. */
        int smq_svcq_pad[NodeShmCtrl::shm_svc_consumers];

        shm_1prd_ncon_q_t smq_plat2svc;        /* *< 1 platformd to multiple svcs. */
        int smq_plat_pad[NodeShmCtrl::shm_svc_consumers];
    } node_shm_queue_t;

    cc_assert(node_inv_shm0, sizeof(node_shm_queue_t) <= NodeShmCtrl::shm_queue_hdr);

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_SHM_QUEUE_H_
