/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INFO_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INFO_EVT_H_

#include "node_work_event.h"

namespace fds
{
    struct NodeInfoEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeInfoMsg> evt_msg;

        NodeInfoEvt(fpi::AsyncHdrPtr hdr,
                    bo::shared_ptr<fpi::NodeInfoMsg> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeInfoMsgTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeInfo " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_INFO_EVT_H_
