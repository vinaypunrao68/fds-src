/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_NODE_SVC_ITER_H_
#define SOURCE_PLATFORM_INCLUDE_NODE_SVC_ITER_H_

#include "fds_resource.h"
#include "platform/pm_agent.h"

namespace fds
{
    class NodeSvcIter : public ResourceIter
    {
        public:
            explicit NodeSvcIter(fpi::DomainNodes &ret) : iter_ret(ret)
            {
            }

            bool rs_iter_fn(Resource::pointer curr)
            {
                PmAgent::pointer    pm;
                fpi::NodeSvcInfo    info;

                pm = agt_cast_ptr<PmAgent>(curr);
                pm->agent_svc_info(&info);
                iter_ret.dom_nodes.push_back(info);
                return true;
            }

        protected:
            fpi::DomainNodes    &iter_ret;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_NODE_SVC_ITER_H_
