/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_UPGRADE_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_UPGRADE_EVT_H_

namespace fds
{
    struct NodeUpgradeEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeUpgrade> evt_msg;

        NodeUpgradeEvt(fpi::AsyncHdrPtr hdr,
                       bo::shared_ptr<fpi::NodeUpgrade> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeUpgradeTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeUpgrade " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_UPGRADE_EVT_H_
