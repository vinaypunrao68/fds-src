/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_NODE_INFO_ITER_H_
#define SOURCE_PLATFORM_INCLUDE_NODE_INFO_ITER_H_

#include <vector>

#include "fds_resource.h"

namespace fds
{
    class NodeInfoIter : public ResourceIter
    {
        public:
            virtual ~NodeInfoIter()
            {
            }

            explicit NodeInfoIter(std::vector<fpi::NodeInfoMsg> &res) : nd_iter_ret(res)
            {
            }

            virtual bool rs_iter_fn(Resource::pointer curr)
            {
                fpi::NodeInfoMsg      msg;
                NodeAgent::pointer    node;

                if (curr->rs_get_uuid() == gl_OmPmUuid)
                {
                    return true;
                }
                node = agt_cast_ptr<NodeAgent>(curr);
                node->init_node_info_msg(&msg);
                nd_iter_ret.push_back(msg);
                return true;
            }

        protected:
            std::vector<fpi::NodeInfoMsg>    &nd_iter_ret;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_NODE_INFO_ITER_H_
