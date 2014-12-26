/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>

#include <net/SvcRequestPool.h>
#include "platform/node_integrate.h"
#include "platform/node_integrate_evt.h"
#include "platform/node_work_item.h"

namespace fds
{
    int NodeIntegrate::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
    {
        NodeWorkItem::ptr                      wrk;
        bo::intrusive_ptr<NodeIntegrateEvt>    arg;

        wrk = state_cast_ptr<NodeWorkItem>(cur);
        arg = evt_cast_ptr<NodeIntegrateEvt>(evt);

        wrk->st_trace(arg) << wrk << "\n";

        if ((arg->evt_run_act == true)|| (wrk->wrk_is_in_om() == true))
        {
            wrk->act_node_integrate(arg, arg->evt_msg);
        }

        return StateEntry::st_no_change;
    }
}  // namespace fds
