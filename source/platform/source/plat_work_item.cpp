/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "plat_work_item.h"

namespace fds
{
    /**
     * act_node_down
     * -------------
     */
    void PlatWorkItem::act_node_down(NodeWrkEvent::ptr evt, fpi::NodeDownPtr &msg)
    {
        NodeWorkItem::act_node_down(evt, msg);
    }

    /**
     * act_node_started
     * ----------------
     */
    void PlatWorkItem::act_node_started(NodeWrkEvent::ptr evt, fpi::NodeInfoMsgPtr &msg)
    {
        NodeWorkItem::act_node_started(evt, msg);
    }

    /**
     * act_node_qualify
     * ----------------
     */
    void PlatWorkItem::act_node_qualify(NodeWrkEvent::ptr evt, fpi::NodeQualifyPtr &msg)
    {
        NodeWorkItem::act_node_qualify(evt, msg);
    }

    /**
     * act_node_upgrade
     * ----------------
     */
    void PlatWorkItem::act_node_upgrade(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
    {
        NodeWorkItem::act_node_upgrade(evt, msg);
    }

    /**
     * act_node_rollback
     * -----------------
     */
    void PlatWorkItem::act_node_rollback(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
    {
        NodeWorkItem::act_node_rollback(evt, msg);
    }

    /**
     * act_node_integrate
     * ------------------
     */
    void PlatWorkItem::act_node_integrate(NodeWrkEvent::ptr evt, fpi::NodeIntegratePtr &msg)
    {
        NodeWorkItem::act_node_integrate(evt, msg);
    }

    /**
     * act_node_deploy
     * ---------------
     */
    void PlatWorkItem::act_node_deploy(NodeWrkEvent::ptr evt, fpi::NodeDeployPtr &msg)
    {
        NodeWorkItem::act_node_deploy(evt, msg);
    }

    /**
     * act_node_functional
     * -------------------
     */
    void PlatWorkItem::act_node_functional(NodeWrkEvent::ptr evt, fpi::NodeFunctionalPtr &msg)
    {
        NodeWorkItem::act_node_functional(evt, msg);
    }
}  // namespace fds
