/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_AGENT_ITER_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_AGENT_ITER_H_

#include "platform/platform.h"

namespace fds
{
    /*
     * ------------------------------------------------------------------------------------
     * Generic container iterator
     * ------------------------------------------------------------------------------------
     */
    class NodeAgentIter : public ResourceIter
    {
        public:
            virtual ~NodeAgentIter()
            {
            }

            NodeAgentIter() : itr_refcnt(0)
            {
                itr_domain = Platform::platf_singleton()->plf_node_inventory();
            }

            inline void foreach_pm()
            {
                itr_domain->dc_foreach_pm(this);
            }

            inline void foreach_am()
            {
                itr_domain->dc_foreach_am(this);
            }

            inline void foreach_sm()
            {
                itr_domain->dc_foreach_sm(this);
            }

            inline void foreach_dm()
            {
                itr_domain->dc_foreach_dm(this);
            }

        protected:
            DomainContainer::pointer    itr_domain;

        private:
            INTRUSIVE_PTR_DEFS(NodeAgentIter, itr_refcnt);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_AGENT_ITER_H_
