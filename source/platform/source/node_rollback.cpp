/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>
#include <platform/node-inventory.h>
#include <net/SvcRequestPool.h>

#include "platform/node_rollback.h"
#include "platform/node_upgrade_evt.h"

#include "platform/node_work_item.h"

namespace fds
{
    int NodeRollback::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
    {
        NodeWorkItem::ptr                    wrk;
        bo::intrusive_ptr<NodeUpgradeEvt>    arg;

        wrk = state_cast_ptr<NodeWorkItem>(cur);
        arg = evt_cast_ptr<NodeUpgradeEvt>(evt);

        wrk->st_trace(arg) << wrk << "\n";

        if ((arg->evt_run_act == true)|| (wrk->wrk_is_in_om() == true))
        {
            wrk->act_node_rollback(arg, arg->evt_msg);
        }

        return StateEntry::st_no_change;
    }
}  // namespace fds
