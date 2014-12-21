/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_DOWN_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_DOWN_EVT_H_

namespace fds
{
    struct NodeDownEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeDown> evt_msg;

        NodeDownEvt(fpi::AsyncHdrPtr hdr,
                    bo::shared_ptr<fpi::NodeDown> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeDownTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeDown " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_DOWN_EVT_H_
