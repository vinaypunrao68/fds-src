/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <kvstore/configdb.h>
#include <om-node-workflow.h>
#include <orchMgr.h>

#include "platform/node_data.h"
#include "platform/node_shm_ctrl.h"

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
 * When we get here, the node may be new or known before.
 * If the node is new, we must wait for the NWL_ADMIT_NODE event sent from UI.
 */
void
OmWorkItem::act_node_started(NodeWrkEvent::ptr evt, fpi::NodeInfoMsgPtr &msg)
{
    NodeUuid           uuid(msg->node_loc.svc_id.svc_uuid.svc_uuid);
    unsigned int       counter;
    bool               known;
    node_data_t        node;
    kvstore::ConfigDB *db;

    known = false;
    db = gl_orch_mgr->getConfigDB();
    if (db->getNode(uuid, &node)) {
        known = true;
    } else {
        if (evt->evt_op == NWL_ADMIT_NODE) {
            /* We're told to admit the node. */
            NodeInventory::node_info_msg_to_shm(msg.get(), &node);

            /* Atomic sequential counter. */
            counter = db->getNodeNameCounter();
            snprintf(node.nd_auto_name, sizeof(node.nd_auto_name), "Node-%u", counter);

            /*
             * TODO(Vy): we need to have different state for this because it hasn't
             * been added to DLT yet.
             */
            db->addNode(&node);
            known = true;
        }
    }
    if (known == true) {
        /* Follow the standard started sequence; we'll qualify the node later. */
        NodeWorkItem::act_node_started(evt, msg);
    }
    NodeWorkItem::act_node_started(evt, msg);
}

/**
 * act_node_qualify
 * ----------------
 * We need to do any version compatibility check here.
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

/**
 * wrk_recv_node_info
 * ------------------
 */
void
OmWorkFlow::wrk_recv_node_info(fpi::AsyncHdrPtr &hdr, fpi::NodeInfoMsgPtr &pkt)
{
    int      idx;
    NodeUuid uuid(pkt->node_loc.svc_id.svc_uuid.svc_uuid);

    if (wrk_inv->dc_find_node_agent(uuid) == NULL) {
        NodeAgent::pointer agent;
        node_data_t        ndata;
        fds_uint64_t       uid;
        ShmObjROKeyUint64 *shm;

        /*
         * PM should have told OM about this node at PlatLibNodeReg::shmq_handler().  If
         * somehow the shmq is slow, we can register the node here; the other path will
         * do consistency check if the node is already in the inventory.
         */
        shm = NodeShmCtrl::shm_node_inventory();
        uid = uuid.uuid_get_val();
        idx = shm->shm_lookup_rec(static_cast<const void *>(&uid),
                                  static_cast<void *>(&ndata), sizeof(ndata));
        fds_verify(idx != -1);
        wrk_inv->dc_register_node(shm, &agent, idx, -1, NODE_DO_PROXY_ALL_SVCS);
    }
    NodeWorkFlow::wrk_recv_node_info(hdr, pkt);
}

/* Entry to access to DLT functions. */

/**
 * wrk_comp_dlt_add
 * ----------------
 */
void
OmWorkFlow::wrk_comp_dlt_add(NodeAgent::pointer na)
{
}

/**
 * wrk_comp_dlt_remove
 * -------------------
 */
void
OmWorkFlow::wrk_comp_dlt_remove(NodeAgent::pointer na)
{
}

/**
 * wrk_wrkitem_dlt
 * ---------------
 */
void
OmWorkFlow::wrk_wrkitem_dlt(NodeAgent::pointer na, std::vector<fpi::NodeWorkItem> *wk)
{
}

/**
 * wrk_commit_dlt
 * --------------
 */
void
OmWorkFlow::wrk_commit_dlt(NodeAgent::pointer na, fpi::NodeWorkItemPtr &wrk)
{
}

/* Entry to access to DMT functions. */

/**
 * wrk_comp_dmt_add
 * ----------------
 */
void
OmWorkFlow::wrk_comp_dmt_add(NodeAgent::pointer na)
{
}

/**
 * wrk_comp_dmt_remove
 * -------------------
 */
void
OmWorkFlow::wrk_comp_dmt_remove(NodeAgent::pointer na)
{
}

/**
 * wrk_wrkitem_dmt
 * ---------------
 */
void
OmWorkFlow::wrk_wrkitem_dmt(NodeAgent::pointer na, std::vector<fpi::NodeWorkItem> *wk)
{
}

/**
 * wrk_commit_dmt
 * --------------
 */
void
OmWorkFlow::wrk_commit_dmt(NodeAgent::pointer na, fpi::NodeWorkItemPtr &wrk)
{
}

}  // namespace fds
