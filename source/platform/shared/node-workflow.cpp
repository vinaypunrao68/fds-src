/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <platform/node-workflow.h>

namespace fds {

NodeWorkFlow                gl_NodeWorkFlow;
static const state_switch_t sgl_node_wrk_flow[] =
{
    /* Current State            | Message Input           | Next State              */
    { NodeDown::st_index(),       fpi::NodeInfoMsgTypeId,   NodeStarted::st_index() },
    { NodeDown::st_index(),       fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeStarted::st_index(),    fpi::NodeQualifyTypeId,   NodeQualify::st_index() },
    { NodeStarted::st_index(),    fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeQualify::st_index(),    fpi::NodeUpgradeTypeId,   NodeUpgrade::st_index() },
    { NodeQualify::st_index(),    fpi::NodeRollbackTypeId,  NodeUpgrade::st_index() },
    { NodeQualify::st_index(),    fpi::NodeIntegrateTypeId, NodeIntegrate::st_index() },
    { NodeQualify::st_index(),    fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeUpgrade::st_index(),    fpi::NodeInfoMsgTypeId,   NodeQualify::st_index() },
    { NodeUpgrade::st_index(),    fpi::NodeDownTypeId,      NodeDown::st_index() },
    { NodeRollback::st_index(),   fpi::NodeInfoMsgTypeId,   NodeQualify::st_index() },
    { NodeRollback::st_index(),   fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeIntegrate::st_index(),  fpi::NodeDeployTypeId,    NodeDeploy::st_index() },
    { NodeIntegrate::st_index(),  fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeDeploy::st_index(),     fpi::NodeDeployTypeId,    NodeDeploy::st_index() },
    { NodeDeploy::st_index(),     fpi::NodeFuncTypeId,      NodeFunctional::st_index() },
    { NodeDeploy::st_index(),     fpi::NodeDownTypeId,      NodeDown::st_index() },

    { NodeFunctional::st_index(), fpi::NodeDeployTypeId,    NodeFunctional::st_index() },
    { NodeFunctional::st_index(), fpi::NodeFuncTypeId,      NodeFunctional::st_index() },
    { NodeFunctional::st_index(), fpi::NodeDownTypeId,      NodeDown::st_index() },

    { -1,                         fpi::UnknownMsgTypeId,    -1 }
    /* Current State            | Message Input           | Next State              */
};

int
NodeDown::st_handle(EventObj::pointer evt, StateObj::pointer cur) const
{
    NodeWorkItem::ptr  wrk;
    bo::intrusive_ptr<NodeDownEvt> arg;

    wrk = state_cast_ptr<NodeWorkItem>(cur);
    arg = evt_cast_ptr<NodeDownEvt>(evt);

    wrk->st_trace(arg) << "\n";
    if (arg->evt_run_act == true) {
        wrk->act_node_down(arg, arg->evt_msg);
    }
    /* No more parent state; we have to handle all events here. */
    return StateEntry::st_no_change;
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
        wrk->act_node_started(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
        wrk->act_node_qualify(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
        wrk->act_node_upgrade(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
        wrk->act_node_rollback(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
        wrk->act_node_integrate(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
        wrk->act_node_deploy(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
        wrk->act_node_functional(arg, arg->evt_msg);
    }
    return StateEntry::st_no_change;
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
NodeWorkItem::act_node_down(NodeWrkEvent::ptr e, bo::shared_ptr<fpi::NodeDown> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_started(NodeWrkEvent::ptr e, bo::shared_ptr<fpi::NodeInfoMsg> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_qualify(NodeWrkEvent::ptr e, bo::shared_ptr<fpi::NodeQualify> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_upgrade(NodeWrkEvent::ptr e, bo::shared_ptr<fpi::NodeUpgrade> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_rollback(NodeWrkEvent::ptr e, bo::shared_ptr<fpi::NodeUpgrade> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_integrate(NodeWrkEvent::ptr e,
                                 bo::shared_ptr<fpi::NodeIntegrate> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

void
NodeWorkItem::act_node_deploy(NodeWrkEvent::ptr e, bo::shared_ptr<fpi::NodeDeploy> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    this->wrk_send_node_functional(NULL, NULL);
}

void
NodeWorkItem::act_node_functional(NodeWrkEvent::ptr e,
                                  bo::shared_ptr<fpi::NodeFunctional> m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

/**
 * wrk_send_node_info
 * ------------------
 */
void
NodeWorkItem::wrk_send_node_info(fpi::AsyncHdrPtr hdr,
                                 bo::shared_ptr<fpi::NodeInfoMsg> msg)
{
    /* Send to the peer endpoint. */

    this->st_in_async(new NodeInfoEvt(hdr, msg, false));
}

/**
 * wrk_send_node_qualify
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_qualify(fpi::AsyncHdrPtr hdr,
                                    bo::shared_ptr<fpi::NodeQualify> msg)
{
    this->st_in_async(new NodeQualifyEvt(hdr, msg, false));
}

/**
 * wrk_send_node_upgrade
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_upgrade(fpi::AsyncHdrPtr hdr,
                                    bo::shared_ptr<fpi::NodeUpgrade> msg)
{
    this->st_in_async(new NodeUpgradeEvt(hdr, msg, false));
}

/**
 * wrk_send_node_integrate
 * -----------------------
 */
void
NodeWorkItem::wrk_send_node_integrate(fpi::AsyncHdrPtr hdr,
                                      bo::shared_ptr<fpi::NodeIntegrate> msg)
{
    this->st_in_async(new NodeIntegrateEvt(hdr, msg, false));
}

/**
 * wrk_send_node_deploy
 * --------------------
 */
void
NodeWorkItem::wrk_send_node_deploy(fpi::AsyncHdrPtr hdr,
                                   bo::shared_ptr<fpi::NodeDeploy> msg)
{
    this->st_in_async(new NodeDeployEvt(hdr, msg, false));
}

/**
 * wrk_send_node_functional
 * ------------------------
 */
void
NodeWorkItem::wrk_send_node_functional(fpi::AsyncHdrPtr hdr,
                                       bo::shared_ptr<fpi::NodeFunctional> msg)
{
    this->st_in_async(new NodeFunctionalEvt(hdr, msg, false));
}

/**
 * wrk_send_node_down
 * ------------------
 */
void
NodeWorkItem::wrk_send_node_down(fpi::AsyncHdrPtr hdr,
                                 bo::shared_ptr<fpi::NodeDown> msg)
{
    this->st_in_async(new NodeDownEvt(hdr, msg, false));
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
    wrk_fsm->st_set_switch_board(sgl_node_wrk_flow, FDS_ARRAY_ELEM(sgl_node_wrk_flow));
}

/**
 * wrk_item_create
 * ---------------
 * Default factory method.
 */
void
NodeWorkFlow::wrk_item_create(fpi::SvcUuid             &peer,
                              PmAgent::pointer          pm,
                              DomainContainer::pointer  domain)
{
    NodeWorkFlow::wrk_item_assign(pm, wrk_item_alloc(peer, pm, domain));
}

/**
 * wrk_item_alloc
 * --------------
 */
NodeWorkItem::ptr
NodeWorkFlow::wrk_item_alloc(fpi::SvcUuid            &peer,
                             PmAgent::pointer         owner,
                             DomainContainer::pointer domain)
{
    fpi::DomainID  did;
    return new NodeWorkItem(peer, did, owner, wrk_fsm, this);
}

/**
 * wrk_item_assign
 * ---------------
 * Atomic assign the workflow item to the node agent owner.
 */
/* static */ void
NodeWorkFlow::wrk_item_assign(PmAgent::pointer owner, NodeWorkItem::ptr wrk)
{
    owner->rs_mutex()->lock();
    if (owner->pm_wrk_item == NULL) {
        owner->pm_wrk_item = wrk;
    }
    owner->rs_mutex()->unlock();
}

/**
 * mod_init
 * --------
 */
int
NodeWorkFlow::mod_init(const SysParams *arg)
{
    Platform *plat = Platform::platf_singleton();

    wrk_clus = plat->plf_cluster_map();
    wrk_inv  = plat->plf_node_inventory();

    Module::mod_init(arg);
    return 0;
}

/**
 * mod_startup
 * -----------
 */
void
NodeWorkFlow::mod_startup()
{
    Module::mod_startup();
}

/**
 * mod_enable_service
 * ------------------
 */
void
NodeWorkFlow::mod_enable_service()
{
    Module::mod_enable_service();
}

/**
 * mod_shutdown
 * ------------
 */
void
NodeWorkFlow::mod_shutdown()
{
    Module::mod_shutdown();
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
    if (pm->pm_wrk_item == NULL) {
        wrk_item_create(svc, pm, wrk_inv);
    }
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
NodeWorkFlow::wrk_recv_node_info(fpi::AsyncHdrPtr hdr,
                                 bo::shared_ptr<fpi::NodeInfoMsg> msg)
{
    wrk_item_submit(msg->node_domain,
                    msg->node_loc.svc_id.svc_uuid, new NodeInfoEvt(hdr, msg));
}

/**
 * wrk_recv_node_qualify
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_qualify(fpi::AsyncHdrPtr hdr,
                                    bo::shared_ptr<fpi::NodeQualify> msg)
{
    fpi::NodeInfoMsg *ptr;

    ptr = &(msg.get())->nd_info;
    wrk_item_submit(ptr->node_domain,
                    ptr->node_loc.svc_id.svc_uuid, new NodeQualifyEvt(hdr, msg));
}

/**
 * wrk_recv_node_upgrade
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_upgrade(fpi::AsyncHdrPtr hdr,
                                    bo::shared_ptr<fpi::NodeUpgrade> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeUpgradeEvt(hdr, msg));
}

/**
 * wrk_recv_node_integrate
 * -----------------------
 */
void
NodeWorkFlow::wrk_recv_node_integrate(fpi::AsyncHdrPtr hdr,
                                      bo::shared_ptr<fpi::NodeIntegrate> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeIntegrateEvt(hdr, msg));
}

/**
 * wrk_recv_node_deploy
 * --------------------
 */
void
NodeWorkFlow::wrk_recv_node_deploy(fpi::AsyncHdrPtr hdr,
                                   bo::shared_ptr<fpi::NodeDeploy> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeDeployEvt(hdr, msg));
}

/**
 * wrk_recv_node_functional
 * ------------------------
 */
void
NodeWorkFlow::wrk_recv_node_functional(fpi::AsyncHdrPtr hdr,
                                       bo::shared_ptr<fpi::NodeFunctional> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeFunctionalEvt(hdr, msg));
}

/**
 * wrk_recv_node_down
 * ------------------
 */
void
NodeWorkFlow::wrk_recv_node_down(fpi::AsyncHdrPtr hdr, bo::shared_ptr<fpi::NodeDown> msg)
{
    wrk_item_submit(msg->nd_dom_id, msg->nd_uuid, new NodeDownEvt(hdr, msg));
}

}  // namespace fds
