/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>
#include <platform/node-inventory.h>
#include <net/SvcRequestPool.h>

#include "platform/node_started.h"
#include "platform/node_info_evt.h"

#include "platform/node_work_item.h"

namespace fds
{
    class NodeWorkItem;

    int NodeStarted::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
    {
        NodeWorkItem::ptr                 wrk;
        bo::intrusive_ptr<NodeInfoEvt>    arg;

        wrk = state_cast_ptr<NodeWorkItem>(cur);
        arg = evt_cast_ptr<NodeInfoEvt>(evt);

        wrk->st_trace(arg) << wrk << "\n";

        if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true))
        {
            wrk->act_node_started(arg, arg->evt_msg);
        }

        return StateEntry::st_no_change;
    }
}  // namespace fds
