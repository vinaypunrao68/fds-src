/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_NODE_SHM_RW_CTRL_H_
#define SOURCE_PLATFORM_INCLUDE_NODE_SHM_RW_CTRL_H_

#include <platform/node-inv-shmem.h>
#include "platform/node_shm_queue.h"
#include "platform/node_shm_inventory.h"

namespace fds
{
    class DiskPlatModule;
    class NodeShmRWCtrl;

    extern NodeShmRWCtrl    gl_NodeShmRWCtrl;

    /**
     * Platform daemon module controls share memory segments.
     */

    class NodeShmRWCtrl : public NodeShmCtrl
    {
        public:
            virtual ~NodeShmRWCtrl()
            {
            }

            explicit NodeShmRWCtrl(const char *name);

            static ShmObjRWKeyUint64 *shm_am_rw_inv()
            {
                return gl_NodeShmRWCtrl.shm_am_rw;
            }

            static ShmObjRWKeyUint64 *shm_node_rw_inv()
            {
                return gl_NodeShmRWCtrl.shm_node_rw;
            }

            static ShmObjRW *shm_uuid_rw_binding()
            {
                return gl_NodeShmRWCtrl.shm_uuid_rw;
            }

            static ShmObjRWKeyUint64 *shm_node_rw_inv(FdspNodeType type)
            {
                if (type == fpi::FDSP_STOR_HVISOR)
                {
                    return gl_NodeShmRWCtrl.shm_am_rw;
                }

                return gl_NodeShmRWCtrl.shm_node_rw;
            }

            static ShmObjRW * shm_dlt_rw_inv()
            {
                return gl_NodeShmRWCtrl.shm_dlt_rw;
            }

            static ShmObjRW * shm_dmt_rw_inv()
            {
                return gl_NodeShmRWCtrl.shm_dmt_rw;
            }

            /**
             * Module methods
             */
            virtual int  mod_init(SysParams const *const p) override;
            virtual void mod_startup() override;
            virtual void mod_shutdown() override;

        protected:
            ShmObjRWKeyUint64   *shm_am_rw;
            ShmObjRWKeyUint64   *shm_node_rw;
            ShmObjRW            *shm_uuid_rw;
            ShmObjRW            *shm_dlt_rw;
            ShmObjRW            *shm_dmt_rw;

            void shm_init_queue(node_shm_queue_t *queue);
            void shm_init_header(node_shm_inventory_t *hdr);

            virtual void shm_setup_queue() override;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_NODE_SHM_RW_CTRL_H_
