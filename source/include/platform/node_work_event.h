/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_WORK_EVENT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_WORK_EVENT_H_

#include "node_work_type.h"

namespace fds
{
    /**
     * Common events to work item.
     */
    struct NodeWrkEvent : public    EventObj
    {
        typedef bo::intrusive_ptr<NodeWrkEvent> ptr;

        bool evt_run_act;
        node_work_type_e evt_op;
        fpi::AsyncHdrPtr evt_pkt_hdr;

        NodeWrkEvent(int evt_id, fpi::AsyncHdrPtr hdr, bool act)
            : EventObj(evt_id), evt_run_act(act), evt_op(NWL_NO_OP), evt_pkt_hdr(hdr)
        {
        }
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_WORK_EVENT_H_
