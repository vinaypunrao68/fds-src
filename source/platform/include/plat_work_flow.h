/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_WORK_FLOW_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_WORK_FLOW_H_

#include <platform/node-workflow.h>

#include "platform/node_work_flow.h"

namespace fds
{
    class PlatWorkFlow;
    extern PlatWorkFlow    gl_PlatWorkFlow;

    class PlatWorkFlow : public NodeWorkFlow
    {
        public:
            /* Singleton access. */
            static PlatWorkFlow *nd_workflow_sgt()
            {
                return &gl_PlatWorkFlow;
            }

            /* Module methods. */
            int  mod_init(SysParams const *const param) override;
            void mod_startup() override;
            void mod_enable_service() override;
            void mod_shutdown() override;

        protected:
            /* Factory method. */
            virtual NodeWorkItem::ptr wrk_item_alloc(fpi::SvcUuid               &peer,
                                                     bo::intrusive_ptr<PmAgent>  owner,
                                                     bo::intrusive_ptr<DomainContainer> domain);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_WORK_FLOW_H_
