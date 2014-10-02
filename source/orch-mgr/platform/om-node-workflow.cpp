/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <om-node-workflow.h>

namespace fds {

OmWorkFlow                 gl_OmWorkFlow;

/**
 * act_node_down
 * -------------
 */
void
OmWorkItem::act_node_down(NodeWrkEvent::ptr evt, fpi::NodeDownPtr &msg)
{
    NodeWorkItem::act_node_down(evt, msg);
}

/**
 * act_node_started
 * ----------------
 */
void
OmWorkItem::act_node_started(NodeWrkEvent::ptr evt, fpi::NodeInfoMsgPtr &msg)
{
    NodeWorkItem::act_node_started(evt, msg);
}

/**
 * act_node_qualify
 * ----------------
 */
void
OmWorkItem::act_node_qualify(NodeWrkEvent::ptr evt, fpi::NodeQualifyPtr &msg)
{
    NodeWorkItem::act_node_qualify(evt, msg);
}

/**
 * act_node_upgrade
 * ----------------
 */
void
OmWorkItem::act_node_upgrade(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
{
    NodeWorkItem::act_node_upgrade(evt, msg);
}

/**
 * act_node_rollback
 * -----------------
 */
void
OmWorkItem::act_node_rollback(NodeWrkEvent::ptr evt, fpi::NodeUpgradePtr &msg)
{
    NodeWorkItem::act_node_rollback(evt, msg);
}

/**
 * act_node_integrate
 * ------------------
 */
void
OmWorkItem::act_node_integrate(NodeWrkEvent::ptr evt, fpi::NodeIntegratePtr &msg)
{
    NodeWorkItem::act_node_integrate(evt, msg);
}

/**
 * act_node_deploy
 * ---------------
 */
void
OmWorkItem::act_node_deploy(NodeWrkEvent::ptr evt, fpi::NodeDeployPtr &msg)
{
    NodeWorkItem::act_node_deploy(evt, msg);
}

/**
 * act_node_functional
 * -------------------
 */
void
OmWorkItem::act_node_functional(NodeWrkEvent::ptr evt, fpi::NodeFunctionalPtr &msg)
{
    NodeWorkItem::act_node_functional(evt, msg);
}

/**
 * mod_init
 * --------
 */
int
OmWorkFlow::mod_init(SysParams const *const param)
{
    gl_NodeWorkFlow = this;
    return NodeWorkFlow::mod_init(param);
}

/**
 * mod_startup
 * -----------
 */
void
OmWorkFlow::mod_startup()
{
    NodeWorkFlow::mod_startup();
}

/**
 * mod_enable_service
 * ------------------
 */
void
OmWorkFlow::mod_enable_service()
{
    NodeWorkFlow::mod_enable_service();
}

/**
 * mod_shutdown
 * ------------
 */
void
OmWorkFlow::mod_shutdown()
{
    NodeWorkFlow::mod_shutdown();
}

/**
 * wrk_item_alloc
 * --------------
 */
NodeWorkItem::ptr
OmWorkFlow::wrk_item_alloc(fpi::SvcUuid               &peer,
                           bo::intrusive_ptr<PmAgent>  owner,
                           bo::intrusive_ptr<DomainContainer> domain)
{
    fpi::DomainID did;
    return new OmWorkItem(peer, did, owner, wrk_fsm, this);
}

}  // namespace fds
