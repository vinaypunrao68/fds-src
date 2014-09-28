/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <node-work.h>
#include <fdsp/fds_service_types.h>
#include <platform/platform-lib.h>
#include <platform/node-workflow.h>

namespace fds {

static void
init_domain_info(const char    *uuid,
                 fds_uint32_t   did,
                 fpi::SvcUuid  *o_uuid,
                 fpi::DomainID *o_did)
{
    o_did->domain_id.svc_uuid = did;
    o_did->domain_name        = "ProbeUT";
    o_uuid->svc_uuid          = strtoull(uuid, NULL, 16);

    /* Get my own uuid for now. */
    Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_assign(o_uuid);
}

JsObject *
NodeQualifyObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_qualify_in_t *in = node_qualify_in();
    bo::shared_ptr<fpi::NodeQualify> pkt;

    pkt = bo::make_shared<fpi::NodeQualify>();
    pkt->nd_acces_token = "abc123";
    init_domain_info(in->svc_uuid, in->domain_id,
                     &pkt->nd_info.node_loc.svc_id.svc_uuid, &pkt->nd_info.node_domain);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_qualify(pkt);
    return this;
}

JsObject *
NodeIntegrateObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_integrate_in_t *in = node_integrate_in();
    bo::shared_ptr<fpi::NodeIntegrate> pkt;

    pkt = bo::make_shared<fpi::NodeIntegrate>();
    pkt->nd_start_am = in->start_am;
    pkt->nd_start_sm = in->start_sm;
    pkt->nd_start_dm = in->start_dm;
    pkt->nd_start_om = in->start_om;
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_integrate(pkt);
    return this;
}

JsObject *
NodeUpgradeObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_upgrade_in_t *in = node_upgrade_in();
    bo::shared_ptr<fpi::NodeUpgrade> pkt;

    pkt = bo::make_shared<fpi::NodeUpgrade>();
    pkt->nd_op_code = fpi::NodeUpgradeTypeId;  // in->op_code;
    pkt->nd_pkg_path.assign(in->path);
    pkt->nd_md5_chksum.assign(in->md5_sum);
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_upgrade(pkt);
    return this;
}

JsObject *
NodeDownObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_down_in_t *in = node_down_in();
    bo::shared_ptr<fpi::NodeDown> pkt;

    pkt = bo::make_shared<fpi::NodeDown>();
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_down(pkt);
    return this;
}

JsObject *
NodeDeployObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_deploy_in_t *in = node_deploy_in();
    bo::shared_ptr<fpi::NodeDeploy> pkt;

    pkt = bo::make_shared<fpi::NodeDeploy>();
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_deploy(pkt);
    return this;
}

JsObject *
NodeInfoMsgObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_info_msg_in_t *in = node_info_msg_in();
    bo::shared_ptr<fpi::NodeInfoMsg> pkt;

    pkt = bo::make_shared<fpi::NodeInfoMsg>();
    init_domain_info(in->svc_uuid, in->domain_id,
                     &pkt->node_loc.svc_id.svc_uuid, &pkt->node_domain);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_info(pkt);
    return this;
}

JsObject *
NodeWorkItemObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_work_item_in_t *in = node_work_item_in();
    bo::shared_ptr<fpi::NodeWorkItem> pkt;

    pkt = bo::make_shared<fpi::NodeWorkItem>();
    pkt->nd_work_code = 0;
    pkt->nd_dom_id.domain_id.svc_uuid = in->domain_id;
    pkt->nd_dom_id.domain_name = "Probe domain";

    pkt->nd_from_svc.svc_uuid = strtoull(in->from_uuid, NULL, 16);
    pkt->nd_to_svc.svc_uuid   = strtoull(in->to_uuid, NULL, 16);

    return this;
}

JsObject *
NodeFunctionalObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_functional_in_t *in = node_functional_in();
    bo::shared_ptr<fpi::NodeFunctional> pkt;

    pkt = bo::make_shared<fpi::NodeFunctional>();
    pkt->nd_op_code = fpi::NodeUpgradeTypeId;
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_functional(pkt);
    return this;
}

}  // namespace fds
