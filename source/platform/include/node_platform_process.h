/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_NODE_PLATFORM_PROCESS_H_
#define SOURCE_PLATFORM_INCLUDE_NODE_PLATFORM_PROCESS_H_

#include <string>
#include <fds-shmobj.h>
#include <fds_process.h>
#include <kvstore/platformdb.h>
#include <shared/fds-constants.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>

namespace fds
{
    class NodePlatformProc : public PlatformProcess
    {
        public:
            NodePlatformProc(int argc, char **argv, Module **vec);

            void proc_pre_startup() override;
            int  run() override;

            void plf_fill_disk_capacity_pkt(fpi::FDSP_RegisterNodeTypePtr pkt);

        protected:
            friend class NodePlatform;

            void plf_load_node_data();
            void plf_scan_hw_inventory();
            void plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg);

        private:
            /// Count of domain AMs on this platform
            // TODO(Andrew): Should be persisted...
            fds_uint32_t    amInstanceCount;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_NODE_PLATFORM_PROCESS_H_
