/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <platform/node-workflow.h>
#include <net/SvcRequestPool.h>

namespace fds {

static NodeWorkFlow         sgt_GenNodeWorkFlow;
NodeWorkFlow               *gl_NodeWorkFlow = &sgt_GenNodeWorkFlow;

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

    { NodeDeploy::st_index(),     fpi::NodeDeployTypeId,     NodeDeploy::st_index() },
    { NodeDeploy::st_index(),     fpi::NodeFunctionalTypeId, NodeFunctional::st_index() },
    { NodeDeploy::st_index(),     fpi::NodeDownTypeId,       NodeDown::st_index() },

    { NodeFunctional::st_index(), fpi::NodeDeployTypeId,     NodeFunctional::st_index() },
    { NodeFunctional::st_index(), fpi::NodeFunctionalTypeId, NodeFunctional::st_index() },
    { NodeFunctional::st_index(), fpi::NodeDownTypeId,       NodeDown::st_index() },

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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true)|| (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true)|| (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true)|| (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true)) {
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

    wrk->st_trace(arg) << wrk << "\n";
    if ((arg->evt_run_act == true) || (wrk->wrk_is_in_om() == true)) {
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
    : StateObj(NodeDown::st_index(), fsm), wrk_sent_ndown(false), wrk_peer_uuid(peer),
      wrk_peer_did(did), wrk_owner(owner), wrk_module(mod) {}

/**
 * wrk_is_in_om
 * ------------
 */
bool
NodeWorkItem::wrk_is_in_om()
{
    /* Do testing with PM for now... */
    if (Platform::platf_singleton()->plf_get_my_node_port() == 7000) {
        return true;
    }
    return false;

    return Platform::platf_singleton()->plf_is_om_node();
}

/**
 * wrk_assign_pkt_uuid
 * -------------------
 */
void
NodeWorkItem::wrk_assign_pkt_uuid(fpi::SvcUuid *svc)
{
    if (wrk_is_in_om() == false) {
        Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_assign(svc);
    } else {
        *svc = wrk_peer_uuid;
    }
}

/**
 * wrk_fmt_node_qualify
 * --------------------
 */
void
NodeWorkItem::wrk_fmt_node_qualify(fpi::NodeQualifyPtr &m)
{
    wrk_owner->init_plat_info_msg(&m->nd_info);
    m->nd_acces_token = "";
    wrk_assign_pkt_uuid(&m->nd_info.node_loc.svc_id.svc_uuid);
}

/**
 * wrk_fmt_node_upgrade
 * --------------------
 */
void
NodeWorkItem::wrk_fmt_node_upgrade(fpi::NodeUpgradePtr &m)
{
    wrk_assign_pkt_uuid(&m->nd_uuid);
    m->nd_dom_id        = wrk_peer_did;
    m->nd_op_code       = fpi::NodeUpgradeTypeId;
    m->nd_md5_chksum    = "";
    m->nd_pkg_path      = "";
}

/**
 * wrk_fmt_node_integrate
 * ----------------------
 */
void
NodeWorkItem::wrk_fmt_node_integrate(fpi::NodeIntegratePtr &m)
{
    wrk_assign_pkt_uuid(&m->nd_uuid);
    m->nd_dom_id    = wrk_peer_did;
    m->nd_start_am  = true;
    m->nd_start_dm  = true;
    m->nd_start_sm  = true;
    m->nd_start_om  = false;
}

/**
 * wrk_fmt_node_wrkitem
 * --------------------
 */
void
NodeWorkItem::wrk_fmt_node_wrkitem(fpi::NodeWorkItemPtr &m)
{
}

/**
 * wrk_fmt_node_deploy
 * -------------------
 */
void
NodeWorkItem::wrk_fmt_node_deploy(fpi::NodeDeployPtr &m)
{
    wrk_assign_pkt_uuid(&m->nd_uuid);
    m->nd_dom_id = wrk_peer_did;
}

/**
 * wrk_fmt_node_functional
 * -----------------------
 */
void
NodeWorkItem::wrk_fmt_node_functional(fpi::NodeFunctionalPtr &m)
{
    wrk_assign_pkt_uuid(&m->nd_uuid);
    m->nd_dom_id  = wrk_peer_did;
    m->nd_op_code = fpi::NodeFunctionalTypeId;
}

/**
 * wrk_fmt_node_down
 * -----------------
 */
void
NodeWorkItem::wrk_fmt_node_down(fpi::NodeDownPtr &m)
{
    wrk_assign_pkt_uuid(&m->nd_uuid);
    m->nd_dom_id  = wrk_peer_did;
}

/**
 * act_node_down
 * -------------
 */
void
NodeWorkItem::act_node_down(NodeWrkEvent::ptr e, fpi::NodeDownPtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

/**
 * act_node_started
 * ----------------
 */
void
NodeWorkItem::act_node_started(NodeWrkEvent::ptr e, fpi::NodeInfoMsgPtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

    if (wrk_is_in_om() == false) {
        fpi::NodeDownPtr    rst;
        fpi::NodeInfoMsgPtr pkt;

        if (wrk_sent_ndown == false) {
            wrk_sent_ndown = true;
            rst = bo::make_shared<fpi::NodeDown>();
            wrk_fmt_node_down(rst);
            wrk_send_node_down(NodeWorkFlow::wrk_om_uuid(), rst, false);
        }
        pkt = bo::make_shared<fpi::NodeInfoMsg>();
        wrk_owner->init_plat_info_msg(pkt.get());
        wrk_send_node_info(NodeWorkFlow::wrk_om_uuid(), pkt, false);
        std::cout << "Send to OM node " << std::endl;
    } else {
        fpi::NodeQualifyPtr pkt;

        pkt = bo::make_shared<fpi::NodeQualify>();
        wrk_fmt_node_qualify(pkt);
        wrk_send_node_qualify(&wrk_peer_uuid, pkt, true);
        std::cout << "Send to node " << wrk_owner->get_uuid().uuid_get_val() << std::endl;
    }
}

/**
 * act_node_qualify
 * ----------------
 */
void
NodeWorkItem::act_node_qualify(NodeWrkEvent::ptr e, fpi::NodeQualifyPtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

    if (wrk_is_in_om() == true) {
        fpi::NodeIntegratePtr pkt;

        pkt = bo::make_shared<fpi::NodeIntegrate>();
        wrk_fmt_node_integrate(pkt);
        wrk_send_node_integrate(&wrk_peer_uuid, pkt, true);
        std::cout << "Send to node " << __FUNCTION__ << std::endl;
    }
}

/**
 * act_node_upgrade
 * ----------------
 */
void
NodeWorkItem::act_node_upgrade(NodeWrkEvent::ptr e, fpi::NodeUpgradePtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

/**
 * act_node_rollback
 * -----------------
 */
void
NodeWorkItem::act_node_rollback(NodeWrkEvent::ptr e, fpi::NodeUpgradePtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

/**
 * act_node_integrate
 * ------------------
 */
void
NodeWorkItem::act_node_integrate(NodeWrkEvent::ptr e, fpi::NodeIntegratePtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

    if (wrk_is_in_om() == true) {
        fpi::NodeDeployPtr pkt;

        pkt = bo::make_shared<fpi::NodeDeploy>();
        wrk_fmt_node_deploy(pkt);
        wrk_send_node_deploy(&wrk_peer_uuid, pkt, true);
        std::cout << "Send to node " << __FUNCTION__ << std::endl;
    }
}

/**
 * act_node_deploy
 * ---------------
 */
void
NodeWorkItem::act_node_deploy(NodeWrkEvent::ptr e, fpi::NodeDeployPtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

    if (wrk_is_in_om() == true) {
        fpi::NodeFunctionalPtr pkt;

        pkt = bo::make_shared<fpi::NodeFunctional>();
        wrk_fmt_node_functional(pkt);
        wrk_send_node_functional(&wrk_peer_uuid, pkt, true);
        std::cout << "Send to node " << __FUNCTION__ << std::endl;
    }
}

/**
 * act_node_functional
 * -------------------
 */
void
NodeWorkItem::act_node_functional(NodeWrkEvent::ptr e, fpi::NodeFunctionalPtr &m)
{
    this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
}

/**
 * wrk_recv_node_wrkitem
 * ---------------------
 */
void
NodeWorkItem::wrk_recv_node_wrkitem(fpi::AsyncHdrPtr &h, fpi::NodeWorkItemPtr &m)
{
}

/**
 * wrk_send_node_wrkitem
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_wrkitem(fpi::SvcUuid *svc, fpi::NodeWorkItemPtr &m, bool run)
{
}

/**
 * wrk_send_node_info
 * ------------------
 */
void
NodeWorkItem::wrk_send_node_info(fpi::SvcUuid *svc, fpi::NodeInfoMsgPtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    if (run == true) {
        this->st_in_async(new NodeInfoEvt(NULL, msg, false));
    }
    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeInfoMsg), msg);
    req->invoke();
}

/**
 * wrk_send_node_qualify
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_qualify(fpi::SvcUuid *svc, fpi::NodeQualifyPtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    if (run == true) {
        this->st_in_async(new NodeQualifyEvt(NULL, msg, false));
    }
    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeQualify), msg);
    req->invoke();
}

/**
 * wrk_send_node_upgrade
 * ---------------------
 */
void
NodeWorkItem::wrk_send_node_upgrade(fpi::SvcUuid *svc, fpi::NodeUpgradePtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    if (run == true) {
        this->st_in_async(new NodeUpgradeEvt(NULL, msg, false));
    }
    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeUpgrade), msg);
    req->invoke();
}

/**
 * wrk_send_node_integrate
 * -----------------------
 */
void
NodeWorkItem::wrk_send_node_integrate(fpi::SvcUuid *svc,
                                      fpi::NodeIntegratePtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    if (run == true) {
        this->st_in_async(new NodeIntegrateEvt(NULL, msg, false));
    }
    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeIntegrate), msg);
    req->invoke();
}

/**
 * wrk_send_node_deploy
 * --------------------
 */
void
NodeWorkItem::wrk_send_node_deploy(fpi::SvcUuid *svc, fpi::NodeDeployPtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    if (run == true) {
        this->st_in_async(new NodeDeployEvt(NULL, msg, false));
    }
    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeDeploy), msg);
    req->invoke();
}

/**
 * wrk_send_node_functional
 * ------------------------
 */
void
NodeWorkItem::wrk_send_node_functional(fpi::SvcUuid *svc,
                                       fpi::NodeFunctionalPtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    if (run == true) {
        this->st_in_async(new NodeFunctionalEvt(NULL, msg, false));
    }
    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeFunctional), msg);
    req->invoke();
}

/**
 * wrk_send_node_down
 * ------------------
 */
void
NodeWorkItem::wrk_send_node_down(fpi::SvcUuid *svc, fpi::NodeDownPtr &msg, bool run)
{
    bo::shared_ptr<EPSvcRequest> req;

    req = gSvcRequestPool->newEPSvcRequest(*svc);
    req->setPayload(FDSP_MSG_TYPEID(fpi::NodeDown), msg);
    req->invoke();
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
NodeWorkFlow::NodeWorkFlow() : Module("Node Work Flow") {}

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

    wrk_fsm = new FsmTable(FDS_ARRAY_ELEM(sgt_node_wrk_flow), sgt_node_wrk_flow);
    wrk_fsm->st_set_switch_board(sgl_node_wrk_flow, FDS_ARRAY_ELEM(sgl_node_wrk_flow));
    gl_OmUuid.uuid_assign(&wrk_om_dest);

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
    /* Fixme(Vy): cleanup OM class hierachy to use PmAgent. */
    pm = agt_cast_ptr<PmAgent>(na);
    if (na->pm_wrk_item == NULL) {
        wrk_item_create(svc, pm, wrk_inv);
    }
    fds_assert(na->pm_wrk_item != NULL);
    return na->pm_wrk_item;
}

/**
 * wrk_item_submit
 * ---------------
 */
void
NodeWorkFlow::wrk_item_submit(fpi::DomainID &did,
                              fpi::AsyncHdrPtr &hdr, EventObj::pointer evt)
{
    fpi::SvcUuid      svc;
    NodeWorkItem::ptr wrk;

    if (NodeWorkItem::wrk_is_om_uuid(hdr->msg_dst_uuid)) {
        svc = hdr->msg_src_uuid;
    } else {
        svc = hdr->msg_dst_uuid;
    }
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
NodeWorkFlow::wrk_recv_node_info(fpi::AsyncHdrPtr &hdr, fpi::NodeInfoMsgPtr &msg)
{
    wrk_item_submit(msg->node_domain, hdr, new NodeInfoEvt(hdr, msg));
}

/**
 * wrk_recv_node_qualify
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_qualify(fpi::AsyncHdrPtr &hdr, fpi::NodeQualifyPtr &msg)
{
    fpi::NodeInfoMsg  *ptr;

    ptr = &(msg.get())->nd_info;
    wrk_item_submit(ptr->node_domain, hdr, new NodeQualifyEvt(hdr, msg));
}

/**
 * wrk_recv_node_upgrade
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_upgrade(fpi::AsyncHdrPtr &hdr, fpi::NodeUpgradePtr &msg)
{
    wrk_item_submit(msg->nd_dom_id, hdr, new NodeUpgradeEvt(hdr, msg));
}

/**
 * wrk_recv_node_integrate
 * -----------------------
 */
void
NodeWorkFlow::wrk_recv_node_integrate(fpi::AsyncHdrPtr &hdr, fpi::NodeIntegratePtr &msg)
{
    wrk_item_submit(msg->nd_dom_id, hdr, new NodeIntegrateEvt(hdr, msg));
}

/**
 * wrk_recv_node_deploy
 * --------------------
 */
void
NodeWorkFlow::wrk_recv_node_deploy(fpi::AsyncHdrPtr &hdr, fpi::NodeDeployPtr &msg)
{
    wrk_item_submit(msg->nd_dom_id, hdr, new NodeDeployEvt(hdr, msg));
}

/**
 * wrk_recv_node_functional
 * ------------------------
 */
void
NodeWorkFlow::wrk_recv_node_functional(fpi::AsyncHdrPtr &hdr, fpi::NodeFunctionalPtr &msg)
{
    wrk_item_submit(msg->nd_dom_id, hdr, new NodeFunctionalEvt(hdr, msg));
}

/**
 * wrk_recv_node_down
 * ------------------
 */
void
NodeWorkFlow::wrk_recv_node_down(fpi::AsyncHdrPtr &hdr, fpi::NodeDownPtr &msg)
{
    wrk_item_submit(msg->nd_dom_id, hdr, new NodeDownEvt(hdr, msg));
}

/**
 * wrk_node_down
 * -------------
 */
void
NodeWorkFlow::wrk_node_down(fpi::DomainID &dom, fpi::SvcUuid &svc)
{
    fpi::AsyncHdrPtr hdr;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    hdr->msg_chksum   = 0;
    hdr->msg_src_id   = 0;
    hdr->msg_code     = 0;
    hdr->msg_dst_uuid = svc;
    Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_assign(&hdr->msg_src_uuid);
    wrk_item_submit(dom, hdr, new NodeDownEvt(hdr, NULL));
}

/**
 * wrk_recv_node_wrkitem
 * ---------------------
 */
void
NodeWorkFlow::wrk_recv_node_wrkitem(fpi::AsyncHdrPtr &hdr, fpi::NodeWorkItemPtr &msg)
{
    NodeWrkEvent::ptr  evt;

    evt = new NodeWorkItemEvt(hdr, msg);
    wrk_item_submit(msg->nd_dom_id, hdr, evt);
}

}  // namespace fds
