/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_DEPLOY_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_DEPLOY_EVT_H_

namespace fds
{
    struct NodeDeployEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeDeploy> evt_msg;

        NodeDeployEvt(fpi::AsyncHdrPtr hdr,
                      bo::shared_ptr<fpi::NodeDeploy> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeDeployTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeDeploy " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_DEPLOY_EVT_H_
