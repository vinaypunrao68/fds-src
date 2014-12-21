/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_SHM_CTRL_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_SHM_CTRL_H_

#include "platform/platform_shm_typedefs.h"

namespace fds
{
    class NodeShmCtrl;
    extern NodeShmCtrl    gl_NodeShmROCtrl;
    extern NodeShmCtrl   *gl_NodeShmCtrl;

    class NodeShmCtrl : public Module
    {
        public:
            virtual ~NodeShmCtrl()
            {
            }

            explicit NodeShmCtrl(const char *name);

            static const int    shm_max_svc   = MAX_DOMAIN_EP_SVC;
            static const int    shm_max_ams   = FDS_MAX_AM_NODES;
            static const int    shm_max_nodes = MAX_DOMAIN_NODES;

            static const int    shm_queue_hdr     = 256;
            static const int    shm_q_item_size   = 128;
            static const int    shm_q_item_count  = 1016;             /** 64K segment */
            static const int    shm_svc_consumers = 8;

            static NodeShmCtrl       *shm_ctrl_singleton()
            {
                return gl_NodeShmCtrl;
            }

            static ShmObjRO          *shm_uuid_binding()
            {
                return gl_NodeShmCtrl->shm_uuid_bind;
            }

            static ShmObjROKeyUint64 *shm_am_inventory()
            {
                return gl_NodeShmCtrl->shm_am_inv;
            }

            static ShmObjRO          *shm_dlt_inv()
            {
                return gl_NodeShmCtrl->shm_dlt;
            }

            static ShmObjRO          *shm_dmt_inv()
            {
                return gl_NodeShmCtrl->shm_dmt;
            }

            static ShmObjROKeyUint64 *shm_node_inventory()
            {
                return gl_NodeShmCtrl->shm_node_inv;
            }

            static ShmObjROKeyUint64 *shm_node_inventory(FdspNodeType type)
            {
                if (type == fpi::FDSP_STOR_HVISOR)
                {
                    return gl_NodeShmCtrl->shm_am_inv;
                }
                return gl_NodeShmCtrl->shm_node_inv;
            }

            static ShmConPrdQueue *shm_consumer()
            {
                return gl_NodeShmCtrl->shm_cons_q;
            }

            static ShmConPrdQueue *shm_producer()
            {
                return gl_NodeShmCtrl->shm_prod_q;
            }

            /**
             * Start a thread to consume at the unique index.
             */
            virtual void shm_start_consumer_thr(int cons_idx)
            {
                shm_cons_q->shm_activate_consumer(cons_idx);
                shm_cons_thr =
                    new std::thread(&ShmConPrdQueue::shm_consume_loop, shm_cons_q, cons_idx);
            }

            /**
             * Module methods
             */
            virtual int  mod_init(SysParams const *const p) override;
            virtual void mod_startup() override;
            virtual void mod_shutdown() override;

        protected:
            /* Shared memory command queue, RW in lib. */
            char                     shm_rw_name[FDS_MAX_UUID_STR];
            FdsShmem                *shm_rw;
            ShmObjRW                *shm_rw_data;
            ShmConPrdQueue          *shm_cons_q;
            ShmConPrdQueue          *shm_prod_q;
            struct node_shm_queue   *shm_queue;
            std::thread             *shm_cons_thr;

            size_t       shm_cmd_queue_size();
            virtual void shm_setup_queue();

            /* Node inventory and uuid binding, RO in plat lib, RW in platform. */
            char                         shm_name[FDS_MAX_UUID_STR];
            FdsShmem                    *shm_ctrl;
            ShmObjROKeyUint64           *shm_am_inv;
            ShmObjROKeyUint64           *shm_node_inv;
            ShmObjRO                    *shm_uuid_bind;
            ShmObjRO                    *shm_dlt;
            ShmObjRO                    *shm_dmt;
            struct node_shm_inventory   *shm_node_hdr;
            size_t                       shm_node_off;
            size_t                       shm_node_siz;
            size_t                       shm_uuid_off;
            size_t                       shm_uuid_siz;
            size_t                       shm_am_off;
            size_t                       shm_am_siz;
            size_t                       shm_dlt_off;
            size_t                       shm_dlt_size;
            size_t                       shm_dmt_off;
            size_t                       shm_dmt_size;
            size_t                       shm_total_siz;

            FdsShmem *shm_create_mgr(const char *fmt, char *name, int size);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_SHM_CTRL_H_
