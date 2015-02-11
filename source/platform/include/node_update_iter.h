/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_NODE_UPDATE_ITER_H_
#define SOURCE_PLATFORM_INCLUDE_NODE_UPDATE_ITER_H_

#include <vector>

#include "platform/node_agent_iter.h"

namespace fds
{
    class NodeUpdateIter : public NodeAgentIter
    {
        public:
            typedef bo::intrusive_ptr<NodeUpdateIter> pointer;

            bool rs_iter_fn(Resource::pointer curr)
            {
                DomainAgent::pointer             agent;
                EpSvcHandle::pointer             eph;
                std::vector<fpi::NodeInfoMsg>    ret;

                agent = agt_cast_ptr<DomainAgent>(curr);
                auto                             rpc = agent->node_svc_rpc(&eph);

                if (rpc != NULL)
                {
                    // NET_SVC_RPC_CALL(eph, rpc, notifyNodeInfo, ret, *(nd_reg_msg.get()), false);
                }

                return true;
            }

            /**
             * Update to platform nodes in a worker thread.
             */
            static void node_reg_update(NodeUpdateIter::pointer itr,
                                        bo::shared_ptr<fpi::NodeInfoMsg> &info)
            {
                itr->nd_reg_msg = info;
                itr->foreach_pm();
            }

        protected:
            bo::shared_ptr<fpi::NodeInfoMsg>    nd_reg_msg;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_NODE_UPDATE_ITER_H_
