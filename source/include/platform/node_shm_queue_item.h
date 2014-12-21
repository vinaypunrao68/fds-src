/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_SHM_QUEUE_ITEM_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_SHM_QUEUE_ITEM_H_

namespace fds
{
    typedef struct node_shm_queue_item
    {
        char smq_item[NodeShmCtrl::shm_q_item_size];
    } node_shm_queue_item_t;

    cc_assert(node_inv_shm1, sizeof(node_shm_queue_item_t) <= NodeShmCtrl::shm_q_item_size);

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_SHM_QUEUE_ITEM_H_
