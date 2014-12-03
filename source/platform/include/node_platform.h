/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_NODE_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_NODE_PLATFORM_H_

#include <string>
#include <fds-shmobj.h>
#include <fds_process.h>
#include <kvstore/platformdb.h>
#include <shared/fds-constants.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>

namespace fds
{
    class NodePlatformProc;
    class DiskPlatModule;

    class NodePlatform : public Platform
    {
        public:
            NodePlatform();
            virtual ~NodePlatform()
            {
            }

            /**
             * Module methods
             */
            virtual int  mod_init(SysParams const *const param) override;
            virtual void mod_startup() override;
            virtual void mod_enable_service() override;
            virtual void mod_shutdown() override;

            void plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg);

            inline void plf_bind_process(NodePlatformProc *ptr)
            {
                plf_process = ptr;
            }

            virtual boost::intrusive_ptr<PmSvcEp> plat_new_pm_svc(NodeAgent::pointer,
                                                                  fds_uint32_t maj,
                                                                  fds_uint32_t min) override;

            void setBaseAsyncSvcHandler(boost::shared_ptr<BaseAsyncSvcHandler> handler);
            virtual boost::shared_ptr<BaseAsyncSvcHandler> getBaseAsyncSvcHandler() override;

        protected:
            NodePlatformProc                         *plf_process;
            DiskPlatModule                           *disk_ctrl;
            boost::shared_ptr<BaseAsyncSvcHandler>    async_svc_handler;

            virtual void plf_bind_om_node();
    };

    extern NodePlatform    gl_NodePlatform;

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_NODE_PLATFORM_H_
