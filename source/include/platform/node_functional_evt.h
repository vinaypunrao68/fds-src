/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_FUNCTIONAL_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_FUNCTIONAL_EVT_H_

#include "node_work_event.h"

namespace fds
{
    struct NodeFunctionalEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeFunctional> evt_msg;

        NodeFunctionalEvt(fpi::AsyncHdrPtr hdr,
                          bo::shared_ptr<fpi::NodeFunctional> msg, bool act = true)
            : NodeWrkEvent(fpi::NodeFunctionalTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeFunct " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_FUNCTIONAL_EVT_H_
