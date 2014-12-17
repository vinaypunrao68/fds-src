/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <platform/node-workflow.h>
#include <net/SvcRequestPool.h>

#include "state_switch.h"

namespace fds
{
    static NodeWorkFlow            sgt_GenNodeWorkFlow;
    NodeWorkFlow                  *gl_NodeWorkFlow = &sgt_GenNodeWorkFlow;

    /*
     * ----------------------------------------------------------------------------------
     * Common Node Workflow
     * ----------------------------------------------------------------------------------
     */
    static const NodeDown             sgt_node_down;
    static const NodeStarted          sgt_node_started;
    static const NodeQualify          sgt_node_qualify;
    static const NodeUpgrade          sgt_node_upgrade;
    static const NodeRollback         sgt_node_rollback;
    static const NodeIntegrate        sgt_node_integrate;
    static const NodeDeploy           sgt_node_deploy;
    static const NodeFunctional       sgt_node_functional;

    /* Index to each state entry. */
    static StateEntry const *const    sgt_node_wrk_flow[] =
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

    NodeWorkFlow::~NodeWorkFlow()
    {
    }

    NodeWorkFlow::NodeWorkFlow() :    Module("Node Work Flow")
    {
    }

    /**
     * wrk_item_create
     * ---------------
     * Default factory method.
     */
    void NodeWorkFlow::wrk_item_create(fpi::SvcUuid             &peer, PmAgent::pointer pm,
                                       DomainContainer::pointer domain)
    {
        NodeWorkFlow::wrk_item_assign(pm, wrk_item_alloc(peer, pm, domain));
    }

    /**
     * wrk_item_alloc
     * --------------
     */
    NodeWorkItem::ptr NodeWorkFlow::wrk_item_alloc(fpi::SvcUuid            &peer,
                                                   PmAgent::pointer owner,
                                                   DomainContainer::pointer domain)
    {
        fpi::DomainID    did;
        return new NodeWorkItem(peer, did, owner, wrk_fsm, this);
    }

    /**
     * wrk_item_assign
     * ---------------
     * Atomic assign the workflow item to the node agent owner.
     */
    /* static */ void NodeWorkFlow::wrk_item_assign(PmAgent::pointer owner, NodeWorkItem::ptr wrk)
    {
        owner->rs_mutex()->lock();

        if (owner->pm_wrk_item == NULL)
        {
            owner->pm_wrk_item = wrk;
        }
        owner->rs_mutex()->unlock();
    }

    /**
     * mod_init
     * --------
     */
    int NodeWorkFlow::mod_init(const SysParams *arg)
    {
        Platform   *plat = Platform::platf_singleton();

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
    void NodeWorkFlow::mod_startup()
    {
        Module::mod_startup();
    }

    /**
     * mod_enable_service
     * ------------------
     */
    void NodeWorkFlow::mod_enable_service()
    {
        Module::mod_enable_service();
    }

    /**
     * mod_shutdown
     * ------------
     */
    void NodeWorkFlow::mod_shutdown()
    {
        Module::mod_shutdown();
    }

    /**
     * wrk_item_frm_uuid
     * -----------------
     */
    NodeWorkItem::ptr NodeWorkFlow::wrk_item_frm_uuid(fpi::DomainID &did, fpi::SvcUuid &svc,
                                                      bool inv)
    {
        PmAgent::pointer      pm;
        NodeAgent::pointer    na;
        ResourceUUID          uuid(svc.svc_uuid);

        if (inv == true)
        {
            na = wrk_inv->dc_find_node_agent(uuid);
        }else {
            na = wrk_clus->dc_find_node_agent(uuid);
        }

        if (na == NULL)
        {
            return NULL;
        }
        /* Fixme(Vy): cleanup OM class hierachy to use PmAgent. */
        pm = agt_cast_ptr<PmAgent>(na);

        if (na->pm_wrk_item == NULL)
        {
            wrk_item_create(svc, pm, wrk_inv);
        }
        fds_assert(na->pm_wrk_item != NULL);
        return na->pm_wrk_item;
    }

    /**
     * wrk_item_submit
     * ---------------
     */
    void NodeWorkFlow::wrk_item_submit(fpi::DomainID &did, fpi::AsyncHdrPtr &hdr,
                                       EventObj::pointer evt)
    {
        fpi::SvcUuid         svc;
        NodeWorkItem::ptr    wrk;

        if (NodeWorkItem::wrk_is_om_uuid(hdr->msg_dst_uuid))
        {
            svc = hdr->msg_src_uuid;
        }else {
            svc = hdr->msg_dst_uuid;
        }
        wrk = wrk_item_frm_uuid(did, svc, true);

        if (wrk == NULL)
        {
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
    void NodeWorkFlow::wrk_recv_node_info(fpi::AsyncHdrPtr &hdr, fpi::NodeInfoMsgPtr &msg)
    {
        wrk_item_submit(msg->node_domain, hdr, new NodeInfoEvt(hdr, msg));
    }

    /**
     * wrk_recv_node_qualify
     * ---------------------
     */
    void NodeWorkFlow::wrk_recv_node_qualify(fpi::AsyncHdrPtr &hdr, fpi::NodeQualifyPtr &msg)
    {
        fpi::NodeInfoMsg   *ptr;

        ptr = &(msg.get())->nd_info;
        wrk_item_submit(ptr->node_domain, hdr, new NodeQualifyEvt(hdr, msg));
    }

    /**
     * wrk_recv_node_upgrade
     * ---------------------
     */
    void NodeWorkFlow::wrk_recv_node_upgrade(fpi::AsyncHdrPtr &hdr, fpi::NodeUpgradePtr &msg)
    {
        wrk_item_submit(msg->nd_dom_id, hdr, new NodeUpgradeEvt(hdr, msg));
    }

    /**
     * wrk_recv_node_integrate
     * -----------------------
     */
    void NodeWorkFlow::wrk_recv_node_integrate(fpi::AsyncHdrPtr &hdr, fpi::NodeIntegratePtr &msg)
    {
        wrk_item_submit(msg->nd_dom_id, hdr, new NodeIntegrateEvt(hdr, msg));
    }

    /**
     * wrk_recv_node_deploy
     * --------------------
     */
    void NodeWorkFlow::wrk_recv_node_deploy(fpi::AsyncHdrPtr &hdr, fpi::NodeDeployPtr &msg)
    {
        wrk_item_submit(msg->nd_dom_id, hdr, new NodeDeployEvt(hdr, msg));
    }

    /**
     * wrk_recv_node_functional
     * ------------------------
     */
    void NodeWorkFlow::wrk_recv_node_functional(fpi::AsyncHdrPtr &hdr, fpi::NodeFunctionalPtr &msg)
    {
        wrk_item_submit(msg->nd_dom_id, hdr, new NodeFunctionalEvt(hdr, msg));
    }

    /**
     * wrk_recv_node_down
     * ------------------
     */
    void NodeWorkFlow::wrk_recv_node_down(fpi::AsyncHdrPtr &hdr, fpi::NodeDownPtr &msg)
    {
        wrk_item_submit(msg->nd_dom_id, hdr, new NodeDownEvt(hdr, msg));
    }

    /**
     * wrk_node_down
     * -------------
     */
    void NodeWorkFlow::wrk_node_down(fpi::DomainID &dom, fpi::SvcUuid &svc)
    {
        fpi::AsyncHdrPtr    hdr;

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
    void NodeWorkFlow::wrk_recv_node_wrkitem(fpi::AsyncHdrPtr &hdr, fpi::NodeWorkItemPtr &msg)
    {
        NodeWrkEvent::ptr    evt;

        evt = new NodeWorkItemEvt(hdr, msg);
        wrk_item_submit(msg->nd_dom_id, hdr, evt);
    }
}  // namespace fds
