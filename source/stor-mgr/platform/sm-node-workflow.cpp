/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sm-node-workflow.h>

namespace fds {

SmWorkFlow                 gl_SmWorkFlow;

/**
 * act_node_down
 * -------------
 */
void
SmWorkItem::act_node_down(NodeWrkEvent::ptr evt, fpi::NodeDownPtr &msg)
{
}

/**
 * act_node_started
 * ----------------
 */
void
SmWorkItem::act_node_started(NodeWrkEvent::ptr evt, fpi::NodeInfoMsgPtr &msg)
{
}

/**
 * act_node_qualify
 * ----------------
 */
void
SmWorkItem::act_node_qualify(NodeWrkEvent::ptr evt, fpi::NodeQualifyPtr &msg)
{
}

/**
 * act_node_upgrade
 * ----------------
 */
void
SmWorkItem::act_node_upgrade(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
{
}

/**
 * act_node_rollback
 * -----------------
 */
void
SmWorkItem::act_node_rollback(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
{
}

/**
 * act_node_integrate
 * ------------------
 */
void
SmWorkItem::act_node_integrate(NodeWrkEvent::ptr evt, fpi::NodeIntegratePtr &msg)
{
}

/**
 * act_node_deploy
 * ---------------
 */
void
SmWorkItem::act_node_deploy(NodeWrkEvent::ptr evt, fpi::NodeDeployPtr &msg)
{
}

/**
 * act_node_functional
 * -------------------
 */
void
SmWorkItem::act_node_functional(NodeWrkEvent::ptr evt, fpi::NodeFunctionalPtr &msg)
{
}

/**
 * mod_init
 * --------
 */
int
SmWorkFlow::mod_init(SysParams const *const param)
{
    gl_NodeWorkFlow = this;
    return NodeWorkFlow::mod_init(param);
}

/**
 * mod_startup
 * -----------
 */
void
SmWorkFlow::mod_startup()
{
    NodeWorkFlow::mod_startup();
}

/**
 * mod_enable_service
 * ------------------
 */
void
SmWorkFlow::mod_enable_service()
{
    NodeWorkFlow::mod_enable_service();
}

/**
 * mod_shutdown
 * ------------
 */
void
SmWorkFlow::mod_shutdown()
{
    NodeWorkFlow::mod_shutdown();
}

/**
 * wrk_item_alloc
 * --------------
 */
NodeWorkItem::ptr
SmWorkFlow::wrk_item_alloc(fpi::SvcUuid               &peer,
                           bo::intrusive_ptr<PmAgent>  owner,
                           bo::intrusive_ptr<DomainContainer> domain)
{
    fpi::DomainID did;
    return new SmWorkItem(peer, did, owner, wrk_fsm, this);
}

}  // namespace fds
