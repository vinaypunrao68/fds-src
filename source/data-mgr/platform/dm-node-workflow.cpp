/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <dm-node-workflow.h>

namespace fds {

DmWorkFlow                 gl_DmWorkFlow;

/**
 * act_node_down
 * -------------
 */
void
DmWorkItem::act_node_down(NodeWrkEvent::ptr evt, fpi::NodeDownPtr &msg)
{
}

/**
 * act_node_started
 * ----------------
 */
void
DmWorkItem::act_node_started(NodeWrkEvent::ptr evt, fpi::NodeInfoMsgPtr &msg)
{
}

/**
 * act_node_qualify
 * ----------------
 */
void
DmWorkItem::act_node_qualify(NodeWrkEvent::ptr evt, fpi::NodeQualifyPtr &msg)
{
}

/**
 * act_node_upgrade
 * ----------------
 */
void
DmWorkItem::act_node_upgrade(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
{
}

/**
 * act_node_rollback
 * -----------------
 */
void
DmWorkItem::act_node_rollback(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
{
}

/**
 * act_node_integrate
 * ------------------
 */
void
DmWorkItem::act_node_integrate(NodeWrkEvent::ptr evt, fpi::NodeIntegratePtr &msg)
{
}

/**
 * act_node_deploy
 * ---------------
 */
void
DmWorkItem::act_node_deploy(NodeWrkEvent::ptr evt, fpi::NodeDeployPtr &msg)
{
}

/**
 * act_node_functional
 * -------------------
 */
void
DmWorkItem::act_node_functional(NodeWrkEvent::ptr evt, fpi::NodeFunctionalPtr &msg)
{
}

/**
 * mod_init
 * --------
 */
int
DmWorkFlow::mod_init(SysParams const *const param)
{
    gl_NodeWorkFlow = this;
    return NodeWorkFlow::mod_init(param);
}

/**
 * mod_startup
 * -----------
 */
void
DmWorkFlow::mod_startup()
{
    NodeWorkFlow::mod_startup();
}

/**
 * mod_enable_service
 * ------------------
 */
void
DmWorkFlow::mod_enable_service()
{
    NodeWorkFlow::mod_enable_service();
}

/**
 * mod_shutdown
 * ------------
 */
void
DmWorkFlow::mod_shutdown()
{
    NodeWorkFlow::mod_shutdown();
}

/**
 * wrk_item_alloc
 * --------------
 */
NodeWorkItem::ptr
DmWorkFlow::wrk_item_alloc(fpi::SvcUuid               &peer,
                           bo::intrusive_ptr<PmAgent>  owner,
                           bo::intrusive_ptr<DomainContainer> domain)
{
    fpi::DomainID did;
    return new DmWorkItem(peer, did, owner, wrk_fsm, this);
}

}  // namespace fds
