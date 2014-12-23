/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_QUALIFY_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_QUALIFY_EVT_H_

#include "node_work_event.h"

namespace fds
{
    struct NodeQualifyEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeQualify> evt_msg;

        NodeQualifyEvt(fpi::AsyncHdrPtr hdr,
                       bo::shared_ptr<fpi::NodeQualify> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeQualifyTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeQualify " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_QUALIFY_EVT_H_
