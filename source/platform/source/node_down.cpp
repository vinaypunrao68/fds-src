/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <net/SvcRequestPool.h>

#include "platform/node_down.h"
#include "platform/node_down_evt.h"

#include "platform/node_work_item.h"

namespace fds
{
    int NodeDown::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
    {
        NodeWorkItem::ptr                 wrk;
        bo::intrusive_ptr<NodeDownEvt>    arg;

        wrk = state_cast_ptr<NodeWorkItem>(cur);
        arg = evt_cast_ptr<NodeDownEvt>(evt);

        wrk->st_trace(arg) << wrk << "\n";

        if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true))
        {
            wrk->act_node_down(arg, arg->evt_msg);
        }
        /* No more parent state; we have to handle all events here. */

        return StateEntry::st_no_change;
    }
}  // namespace fds
