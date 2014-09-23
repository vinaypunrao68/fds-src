/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/platform-lib.h>
#include <platform/node-workflow.h>

namespace fds {

int
NodeDown::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeStarted::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeQualify::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeUpgrade::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeRollback::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeIntegrate::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeDeploy::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

int
NodeFunctional::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    return 0;
}

/*
 * ----------------------------------------------------------------------------------
 * Common Node Workflow Item
 * ----------------------------------------------------------------------------------
 */
NodeWorkItem::~NodeWorkItem() {}
NodeWorkItem::NodeWorkItem(NodeAgent::pointer na, FsmTable::pointer fsm)
    : StateObj(NodeDown::st_index(), fsm), wrk_na(na) {}

void
NodeWorkItem::act_node_down()
{
}

void
NodeWorkItem::act_node_started()
{
}

void
NodeWorkItem::act_node_qualify()
{
}

void
NodeWorkItem::act_node_upgrade()
{
}

void
NodeWorkItem::act_node_rollback()
{
}

void
NodeWorkItem::act_node_integrate()
{
}

void
NodeWorkItem::act_node_deploy()
{
}

void
NodeWorkItem::act_node_functional()
{
}

/*
 * ----------------------------------------------------------------------------------
 * Common Node Workflow
 * ----------------------------------------------------------------------------------
 */
static const NodeDown           sgt_node_down;
static const NodeStarted        sgt_node_started;
static const NodeQualify        sgt_node_qualify;
static const NodeUpgrade        sgt_node_upgrade;
static const NodeRollback       sgt_node_rollback;
static const NodeIntegrate      sgt_node_integrate;
static const NodeDeploy         sgt_node_deploy;
static const NodeFunctional     sgt_node_functional;

/* Index to each state entry. */
static StateEntry const *const  sgt_node_wrk_flow[] =
{
    &sgt_node_down,
    &sgt_node_started,
    &sgt_node_qualify,
    &sgt_node_upgrade,
    &sgt_node_rollback,
    &sgt_node_integrate,
    &sgt_node_deploy,
    &sgt_node_functional,
    NULL
};

NodeWorkFlow::~NodeWorkFlow() {}
NodeWorkFlow::NodeWorkFlow(DomainClusterMap::ptr domain)
    : FsmTable(FDS_ARRAY_ELEM(sgt_node_wrk_flow), sgt_node_wrk_flow)
{
}

static node_wrk_flow_t sgl_node_wrk_flow[] =
{
    { NodeDown::st_index(),       fpi::NodeInfoMsgTypeId, NodeStarted::st_index() },
    { NodeStarted::st_index(),    fpi::NodeInfoMsgTypeId, NodeQualify::st_index() },
    { NodeQualify::st_index(),    fpi::NodeInfoMsgTypeId, NodeUpgrade::st_index() },
    { NodeUpgrade::st_index(),    fpi::NodeInfoMsgTypeId, NodeIntegrate::st_index() },
    { NodeRollback::st_index(),   fpi::NodeInfoMsgTypeId, NodeIntegrate::st_index() },
    { NodeIntegrate::st_index(),  fpi::NodeInfoMsgTypeId, NodeDeploy::st_index() },
    { NodeDeploy::st_index(),     fpi::NodeInfoMsgTypeId, NodeFunctional::st_index() },
    { NodeFunctional::st_index(), fpi::NodeInfoMsgTypeId, NodeDown::st_index() },
    { -1,                         fpi::UnknownMsgTypeId,  -1 }
};

int
NodeWorkFlow::wrk_next_step(fpi::FDSPMsgTypeId input)
{
    return 0;
}

}  // namespace fds
