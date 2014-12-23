/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>

#include <net/SvcRequestPool.h>
#include "platform/node_qualify.h"
#include "platform/node_qualify_evt.h"
#include "platform/node_work_item.h"

namespace fds
{
    int NodeQualify::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
    {
        NodeWorkItem::ptr                    wrk;
        bo::intrusive_ptr<NodeQualifyEvt>    arg;

        wrk = state_cast_ptr<NodeWorkItem>(cur);
        arg = evt_cast_ptr<NodeQualifyEvt>(evt);

        wrk->st_trace(arg) << wrk << "\n";

        if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true))
        {
            wrk->act_node_qualify(arg, arg->evt_msg);
        }

        return StateEntry::st_no_change;
    }
}  // namespace fds
