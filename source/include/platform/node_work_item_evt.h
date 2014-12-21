/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_WORK_ITEM_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_WORK_ITEM_EVT_H_

namespace fds
{
    struct NodeWorkItemEvt : public    NodeWrkEvent
    {
        bo::shared_ptr<fpi::NodeWorkItem> evt_msg;

        NodeWorkItemEvt(fpi::AsyncHdrPtr hdr,
                        fpi::NodeWorkItemPtr msg, bool act = true)
            : NodeWrkEvent(fpi::NodeWorkItemTypeId, hdr, act), evt_msg(msg)
        {
        }

        virtual void evt_name(std::ostream &os) const
        {
            os << " NodeWrkItem " << evt_msg;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_WORK_ITEM_EVT_H_
