/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <vector>
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <platform/node-workflow.h>
#include <net/SvcRequestPool.h>

#include "state_switch.h"
#include "platform/node_work_flow.h"
#include "platform/node_work_item.h"

namespace fds
{
    /*
     * ----------------------------------------------------------------------------------
     * Common Node Workflow Item
     * ----------------------------------------------------------------------------------
     */
    NodeWorkItem::~NodeWorkItem()
    {
    }

    NodeWorkItem::NodeWorkItem(fpi::SvcUuid      &peer, fpi::DomainID     &did,
                               PmAgent::pointer owner, FsmTable::pointer fsm,
                               NodeWorkFlow      *mod) : StateObj(NodeDown::st_index(),
                                                                  fsm), wrk_sent_ndown(false),
        wrk_peer_uuid(peer), wrk_peer_did(did), wrk_owner(owner),
        wrk_module(mod)
    {
    }

    /**
     * wrk_is_in_om
     * ------------
     */
    bool NodeWorkItem::wrk_is_in_om()
    {
        /* Do testing with PM for now... */
        if (Platform::platf_singleton()->plf_get_my_node_port() == 7000)
        {
            return true;
        }

        return false;

        return Platform::platf_singleton()->plf_is_om_node();
    }

    /**
     * wrk_assign_pkt_uuid
     * -------------------
     */
    void NodeWorkItem::wrk_assign_pkt_uuid(fpi::SvcUuid *svc)
    {
        if (wrk_is_in_om() == false)
        {
            Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_assign(svc);
        }else {
            *svc = wrk_peer_uuid;
        }
    }

    /**
     * wrk_fmt_node_qualify
     * --------------------
     */
    void NodeWorkItem::wrk_fmt_node_qualify(fpi::NodeQualifyPtr &m)
    {
        wrk_owner->init_plat_info_msg(&m->nd_info);
        m->nd_acces_token = "";
        wrk_assign_pkt_uuid(&m->nd_info.node_loc.svc_id.svc_uuid);
    }

    /**
     * wrk_fmt_node_upgrade
     * --------------------
     */
    void NodeWorkItem::wrk_fmt_node_upgrade(fpi::NodeUpgradePtr &m)
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
    void NodeWorkItem::wrk_fmt_node_integrate(fpi::NodeIntegratePtr &m)
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
    void NodeWorkItem::wrk_fmt_node_wrkitem(fpi::NodeWorkItemPtr &m)
    {
    }

    /**
     * wrk_fmt_node_deploy
     * -------------------
     */
    void NodeWorkItem::wrk_fmt_node_deploy(fpi::NodeDeployPtr &m)
    {
        wrk_assign_pkt_uuid(&m->nd_uuid);
        m->nd_dom_id = wrk_peer_did;
    }

    /**
     * wrk_fmt_node_functional
     * -----------------------
     */
    void NodeWorkItem::wrk_fmt_node_functional(fpi::NodeFunctionalPtr &m)
    {
        wrk_assign_pkt_uuid(&m->nd_uuid);
        m->nd_dom_id  = wrk_peer_did;
        m->nd_op_code = fpi::NodeFunctionalTypeId;
    }

    /**
     * wrk_fmt_node_down
     * -----------------
     */
    void NodeWorkItem::wrk_fmt_node_down(fpi::NodeDownPtr &m)
    {
        wrk_assign_pkt_uuid(&m->nd_uuid);
        m->nd_dom_id  = wrk_peer_did;
    }

    /**
     * act_node_down
     * -------------
     */
    void NodeWorkItem::act_node_down(NodeWrkEvent::ptr e, fpi::NodeDownPtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    }

    /**
     * act_node_started
     * ----------------
     */
    void NodeWorkItem::act_node_started(NodeWrkEvent::ptr e, fpi::NodeInfoMsgPtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

        if (wrk_is_in_om() == false)
        {
            fpi::NodeDownPtr       rst;
            fpi::NodeInfoMsgPtr    pkt;

            if (wrk_sent_ndown == false)
            {
                wrk_sent_ndown = true;
                rst = bo::make_shared<fpi::NodeDown>();
                wrk_fmt_node_down(rst);
                wrk_send_node_down(NodeWorkFlow::wrk_om_uuid(), rst, false);
            }
            pkt = bo::make_shared<fpi::NodeInfoMsg>();
            wrk_owner->init_plat_info_msg(pkt.get());
            wrk_send_node_info(NodeWorkFlow::wrk_om_uuid(), pkt, false);
            std::cout << "Send to OM node " << std::endl;
        }else {
            fpi::NodeQualifyPtr    pkt;

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
    void NodeWorkItem::act_node_qualify(NodeWrkEvent::ptr e, fpi::NodeQualifyPtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

        if (wrk_is_in_om() == true)
        {
            fpi::NodeIntegratePtr    pkt;

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
    void NodeWorkItem::act_node_upgrade(NodeWrkEvent::ptr e, fpi::NodeUpgradePtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    }

    /**
     * act_node_rollback
     * -----------------
     */
    void NodeWorkItem::act_node_rollback(NodeWrkEvent::ptr e, fpi::NodeUpgradePtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    }

    /**
     * act_node_integrate
     * ------------------
     */
    void NodeWorkItem::act_node_integrate(NodeWrkEvent::ptr e, fpi::NodeIntegratePtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

        if (wrk_is_in_om() == true)
        {
            fpi::NodeDeployPtr    pkt;

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
    void NodeWorkItem::act_node_deploy(NodeWrkEvent::ptr e, fpi::NodeDeployPtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";

        if (wrk_is_in_om() == true)
        {
            fpi::NodeFunctionalPtr    pkt;

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
    void NodeWorkItem::act_node_functional(NodeWrkEvent::ptr e, fpi::NodeFunctionalPtr &m)
    {
        this->st_trace(NULL) << " in " << __FUNCTION__ << "\n";
    }

    /**
     * wrk_recv_node_wrkitem
     * ---------------------
     */
    void NodeWorkItem::wrk_recv_node_wrkitem(fpi::AsyncHdrPtr &h, fpi::NodeWorkItemPtr &m)
    {
    }

    /**
     * wrk_send_node_wrkitem
     * ---------------------
     */
    void NodeWorkItem::wrk_send_node_wrkitem(fpi::SvcUuid *svc, fpi::NodeWorkItemPtr &m, bool run)
    {
    }

    /**
     * wrk_send_node_info
     * ------------------
     */
    void NodeWorkItem::wrk_send_node_info(fpi::SvcUuid *svc, fpi::NodeInfoMsgPtr &msg, bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        if (run == true)
        {
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
    void NodeWorkItem::wrk_send_node_qualify(fpi::SvcUuid *svc, fpi::NodeQualifyPtr &msg, bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        if (run == true)
        {
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
    void NodeWorkItem::wrk_send_node_upgrade(fpi::SvcUuid *svc, fpi::NodeUpgradePtr &msg, bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        if (run == true)
        {
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
    void NodeWorkItem::wrk_send_node_integrate(fpi::SvcUuid *svc, fpi::NodeIntegratePtr &msg,
                                               bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        if (run == true)
        {
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
    void NodeWorkItem::wrk_send_node_deploy(fpi::SvcUuid *svc, fpi::NodeDeployPtr &msg, bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        if (run == true)
        {
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
    void NodeWorkItem::wrk_send_node_functional(fpi::SvcUuid *svc, fpi::NodeFunctionalPtr &msg,
                                                bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        if (run == true)
        {
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
    void NodeWorkItem::wrk_send_node_down(fpi::SvcUuid *svc, fpi::NodeDownPtr &msg, bool run)
    {
        bo::shared_ptr<EPSvcRequest>    req;

        req = gSvcRequestPool->newEPSvcRequest(*svc);
        req->setPayload(FDSP_MSG_TYPEID(fpi::NodeDown), msg);
        req->invoke();
    }

    std::ostream &operator << (std::ostream &os, const NodeWorkItem::ptr st)
    {
        os << st->wrk_owner << " item: " << st.get() << " [" << st->st_curr_name() << "]";
        return os;
    }
}  // namespace fds
