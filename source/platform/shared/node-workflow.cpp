/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <platform/node-workflow.h>

namespace fds {

NodeWorkFlow                 gl_NodeWorkFlow;
static const node_wrk_flow_t sgl_node_wrk_flow[] =
{
    /* Current State            | Message Input           | Next State              */
    { NodeDown::st_index(),       fpi::NodeInfoMsgTypeId,   NodeStarted::st_index() },
    { NodeDown::st_index(),       fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeStarted::st_index(),    fpi::NodeQualifyTypeId,   NodeQualify::st_index() },

    { NodeQualify::st_index(),    fpi::NodeUpgradeTypeId,   NodeUpgrade::st_index() },
    { NodeQualify::st_index(),    fpi::NodeRollbackTypeId,  NodeUpgrade::st_index() },
    { NodeQualify::st_index(),    fpi::NodeIntegrateTypeId, NodeUpgrade::st_index() },

    { NodeUpgrade::st_index(),    fpi::NodeInfoMsgTypeId,   NodeQualify::st_index() },
    { NodeRollback::st_index(),   fpi::NodeInfoMsgTypeId,   NodeQualify::st_index() },

    { NodeIntegrate::st_index(),  fpi::NodeDeployTypeId,    NodeDeploy::st_index() },

    { NodeDeploy::st_index(),     fpi::NodeDeployTypeId,    NodeDeploy::st_index() },
    { NodeDeploy::st_index(),     fpi::NodeFuncTypeId,      NodeFunctional::st_index() },

    { NodeFunctional::st_index(), fpi::NodeDeployTypeId,    NodeFunctional::st_index() },
    { NodeFunctional::st_index(), fpi::NodeFuncTypeId,      NodeFunctional::st_index() },

    { -1,                         fpi::UnknownMsgTypeId,    -1 }
    /* Current State            | Message Input           | Next State              */
};

/**
 * wrk_next_step
 * -------------
 */
int
NodeStarted::wrk_next_step(const StateEntry *entry,
                           EventObj::pointer evt,
                           StateObj::pointer state)
{
    int                    i, cur_st;
    fpi::FDSPMsgTypeId     input;
    const node_wrk_flow_t *cur;

    cur_st = state->st_current();
    input  = static_cast<fpi::FDSPMsgTypeId>(evt->evt_current());

    for (i = 0; sgl_node_wrk_flow[i].wrk_cur_item != -1; i++) {
        cur = sgl_node_wrk_flow + i;
        if ((cur->wrk_cur_item == cur_st) && (cur->wrk_msg_in == input)) {
            return cur->wrk_nxt_item;
        }
    }
    /* Try the parent state. */
    return entry->st_handle(evt, state);
}


int
NodeDown::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr  wrk;
    bo::intrusive_ptr<NodeDownEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeDownEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_down(arg->evt_msg);
    }
    /* No more parent state; we have to handle all events here. */
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeStarted::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeInfoEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeInfoEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_started(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeQualify::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeQualifyEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeQualifyEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_qualify(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeUpgrade::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeUpgradeEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeUpgradeEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_upgrade(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeRollback::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeUpgradeEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeUpgradeEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_rollback(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeIntegrate::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeIntegrateEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeIntegrateEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_integrate(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeDeploy::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeDeployEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeDeployEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_deploy(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

int
NodeFunctional::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr wrk;
    bo::intrusive_ptr<NodeFunctionalEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeFunctionalEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_functional(arg->evt_msg);
    }
    return NodeStarted::wrk_next_step(this, evt, cur);
}

/*
 * ----------------------------------------------------------------------------------
 * Common Node Workflow Item
 * ----------------------------------------------------------------------------------
 */
NodeWorkItem::~NodeWorkItem() {}
NodeWorkItem::NodeWorkItem(fpi::SvcUuid      &peer,
                           fpi::DomainID     &did,
                           PmAgent::pointer   owner,
                           FsmTable::pointer  fsm,
                           NodeWorkFlow      *mod)
    : StateObj(NodeDown::st_index(), fsm), wrk_peer_uuid(peer),
      wrk_peer_did(did), wrk_owner(owner), wrk_module(mod) {}

void
NodeWorkItem::act_node_down(bo::shared_ptr<fpi::NodeDown> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_started(bo::shared_ptr<fpi::NodeInfoMsg> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_info(msg);
}

void
NodeWorkItem::act_node_qualify(bo::shared_ptr<fpi::NodeQualify> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_integrate(NULL);
}

void
NodeWorkItem::act_node_upgrade(bo::shared_ptr<fpi::NodeUpgrade> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_info(NULL);
}

void
NodeWorkItem::act_node_rollback(bo::shared_ptr<fpi::NodeUpgrade> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_info(NULL);
}

void
NodeWorkItem::act_node_integrate(bo::shared_ptr<fpi::NodeIntegrate> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_deploy(NULL);
}

void
NodeWorkItem::act_node_deploy(bo::shared_ptr<fpi::NodeDeploy> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_functional(NULL);
}

void
NodeWorkItem::act_node_functional(bo::shared_ptr<fpi::NodeFunctional> msg)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

/**
 * wrk_send_node_info
 * ------------------
 */
void
NodeWorkItem::wrk_send_node_info(bo::shared_ptr<fpi::NodeInfoMsg> msg)
{
    /* Send to the peer endpoint. */

    this->st_in_async(new NodeInfoEvt(msg, false));
}

/**
 * wrk_send_node_qualify
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_qualify(bo::shared_ptr<fpi::NodeQualify> msg)
{
    this->st_in_async(new NodeQualifyEvt(msg, false));
}

/**
 * wrk_send_node_upgrade
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_upgrade(bo::shared_ptr<fpi::NodeUpgrade> msg)
{
    this->st_in_async(new NodeUpgradeEvt(msg, false));
}

/**
 * wrk_send_node_integrate
 * -----------------------
 */
void
NodeWorkItem::wrk_send_node_integrate(bo::shared_ptr<fpi::NodeIntegrate> msg)
{
    this->st_in_async(new NodeIntegrateEvt(msg, false));
}

/**
 * wrk_send_node_deploy
 * --------------------
 */
void
NodeWorkItem::wrk_send_node_deploy(bo::shared_ptr<fpi::NodeDeploy> msg)
{
    this->st_in_async(new NodeDeployEvt(msg, false));
}

/**
 * wrk_send_node_functional
 * ------------------------
 */
void
NodeWorkItem::wrk_send_node_functional(bo::shared_ptr<fpi::NodeFunctional> msg)
{
    this->st_in_async(new NodeFunctionalEvt(msg, false));
}

/**
 * wrk_send_node_down
 * ------------------
 */
void
NodeWorkItem::wrk_send_node_down(bo::shared_ptr<fpi::NodeDown> msg)
{
    this->st_in_async(new NodeDownEvt(msg, false));
}

std::ostream &
operator << (std::ostream &os, const NodeWorkItem::ptr st)
{
    os << st->wrk_owner << " item: " << st.get() << " [" << st->st_curr_name() << "]";
    return os;
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
NodeWorkFlow::NodeWorkFlow() : Module("Node Work Flow")
{
    wrk_fsm = new FsmTable(FDS_ARRAY_ELEM(sgt_node_wrk_flow), sgt_node_wrk_flow);
}

/**
 * wrk_item_create
 * ---------------
 * Default factory method.
 */
NodeWorkItem::ptr
NodeWorkFlow::wrk_item_create(fpi::SvcUuid      &peer,
                              PmAgent::pointer   pm,
                              DomainNodeInv::ptr domain)
{
    fpi::DomainID did;

    return new NodeWorkItem(peer, did, pm, wrk_fsm, this);
}

/**
 * wrk_item_frm_uuid
 * -----------------
 */
NodeWorkItem::ptr
NodeWorkFlow::wrk_item_frm_uuid(fpi::DomainID &did, fpi::SvcUuid &svc, bool inv)
{
    PmAgent::pointer   pm;
    NodeAgent::pointer na;
    ResourceUUID       uuid(svc.svc_uuid);

    if (inv == true) {
        na = wrk_inv->dc_find_node_agent(uuid);
    } else {
        na = wrk_clus->dc_find_node_agent(uuid);
    }
    if (na == NULL) {
        return NULL;
    }
    pm = agt_cast_ptr<PmAgent>(na);
    fds_assert(pm->pm_wrk_item != NULL);

    return pm->pm_wrk_item;
}

/**
 * wrk_item_submit
 * ---------------
 */
void
NodeWorkFlow::wrk_item_submit(fpi::DomainID &did,
                              fpi::SvcUuid  &svc, EventObj::pointer evt)
{
    NodeWorkItem::ptr wrk;

    wrk = wrk_item_frm_uuid(did, svc, true);
    if (wrk == NULL) {
        /* TODO(Vy): Log event here. */
        return;
    }
    fds_assert(wrk->st_fsm() == wrk_fsm);
    wrk->st_in_async(evt);
}

/**
 * wrk_recv_node_info
 * ------------------
 */
void
NodeWorkFlow::wrk_recv_node_info(bo::shared_ptr<fpi::NodeInfoMsg> msg)
{
    wrk_item_submit(msg->node_domain,
                    msg->node_loc.svc_id.svc_uuid, new NodeInfoEvt(msg));
}

/**
 * wrk_recv_node_qualify
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_qualify(bo::shared_ptr<fpi::NodeQualify> msg)
{
    fpi::NodeInfoMsg *ptr;

    ptr = &(msg.get())->nd_info;
    wrk_item_submit(ptr->node_domain,
                    ptr->node_loc.svc_id.svc_uuid, new NodeQualifyEvt(msg));
}

/**
 * wrk_recv_node_upgrade
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_upgrade(bo::shared_ptr<fpi::NodeUpgrade> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeUpgradeEvt(msg));
}

/**
 * wrk_recv_node_integrate
 * -----------------------
 */
void
NodeWorkFlow::wrk_recv_node_integrate(bo::shared_ptr<fpi::NodeIntegrate> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeIntegrateEvt(msg));
}

/**
 * wrk_recv_node_deploy
 * --------------------
 */
void
NodeWorkFlow::wrk_recv_node_deploy(bo::shared_ptr<fpi::NodeDeploy> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeDeployEvt(msg));
}

/**
 * wrk_recv_node_functional
 * ------------------------
 */
void
NodeWorkFlow::wrk_recv_node_functional(bo::shared_ptr<fpi::NodeFunctional> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeFunctionalEvt(msg));
}

/**
 * wrk_recv_node_down
 * ------------------
 */
void
NodeWorkFlow::wrk_recv_node_down(bo::shared_ptr<fpi::NodeDown> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeDownEvt(msg));
}

}  // namespace fds
