/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <node-work.h>
#include <fdsp/fds_service_types.h>
#include <platform/platform-lib.h>
#include <platform/node-workflow.h>
#include <platform/net-plat-shared.h>
#include <net/net-service.h>
#include <net/SvcRequestPool.h>

namespace fds {

static void
init_async_hdr(fpi::AsyncHdrPtr hdr, fpi::FDSPMsgTypeId id)
{
    hdr->msg_chksum = 0;
    hdr->msg_src_id = 0;
    hdr->msg_code   = 0;

    Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_assign(&hdr->msg_src_uuid);
    hdr->msg_dst_uuid = hdr->msg_src_uuid;
}

static void
init_domain_info(const char    *uuid,
                 fds_uint32_t   did,
                 fpi::SvcUuid  *o_uuid,
                 fpi::DomainID *o_did)
{
    o_did->domain_id.svc_uuid = did;
    o_did->domain_name        = "ProbeUT";
    o_uuid->svc_uuid          = strtoull(uuid, NULL, 16);

    Platform::platf_singleton()->plf_get_my_node_uuid()->uuid_assign(o_uuid);
}

JsObject *
NodeQualifyObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_qualify_in_t *in = node_qualify_in();
    bo::shared_ptr<fpi::AsyncHdr>    hdr;
    bo::shared_ptr<fpi::NodeQualify> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeQualify>();
    pkt->nd_acces_token = "abc123";

    init_async_hdr(hdr, fpi::NodeQualifyTypeId);
    init_domain_info(in->svc_uuid, in->domain_id,
                     &pkt->nd_info.node_loc.svc_id.svc_uuid, &pkt->nd_info.node_domain);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_qualify(hdr, pkt);
    return this;
}

JsObject *
NodeIntegrateObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_integrate_in_t *in = node_integrate_in();
    bo::shared_ptr<fpi::AsyncHdr>      hdr;
    bo::shared_ptr<fpi::NodeIntegrate> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeIntegrate>();
    pkt->nd_start_am = in->start_am;
    pkt->nd_start_sm = in->start_sm;
    pkt->nd_start_dm = in->start_dm;
    pkt->nd_start_om = in->start_om;

    init_async_hdr(hdr, fpi::NodeIntegrateTypeId);
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_integrate(hdr, pkt);
    return this;
}

JsObject *
NodeUpgradeObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_upgrade_in_t *in = node_upgrade_in();
    bo::shared_ptr<fpi::AsyncHdr>    hdr;
    bo::shared_ptr<fpi::NodeUpgrade> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeUpgrade>();
    pkt->nd_op_code = fpi::NodeUpgradeTypeId;  // in->op_code;
    pkt->nd_pkg_path.assign(in->path);
    pkt->nd_md5_chksum.assign(in->md5_sum);

    init_async_hdr(hdr, fpi::NodeUpgradeTypeId);
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_upgrade(hdr, pkt);
    return this;
}

JsObject *
NodeDownObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_down_in_t *in = node_down_in();
    bo::shared_ptr<fpi::AsyncHdr> hdr;
    bo::shared_ptr<fpi::NodeDown> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeDown>();

    init_async_hdr(hdr, fpi::NodeDownTypeId);
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_down(hdr, pkt);
    return this;
}

JsObject *
NodeDeployObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_deploy_in_t *in = node_deploy_in();
    bo::shared_ptr<fpi::AsyncHdr>   hdr;
    bo::shared_ptr<fpi::NodeDeploy> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeDeploy>();

    init_async_hdr(hdr, fpi::NodeDeployTypeId);
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_deploy(hdr, pkt);
    return this;
}

JsObject *
NodeInfoMsgObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_info_msg_in_t *in = node_info_msg_in();
    bo::shared_ptr<fpi::AsyncHdr>    hdr;
    bo::shared_ptr<fpi::NodeInfoMsg> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeInfoMsg>();

    init_async_hdr(hdr, fpi::NodeInfoMsgTypeId);
    init_domain_info(in->svc_uuid, in->domain_id,
                     &pkt->node_loc.svc_id.svc_uuid, &pkt->node_domain);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_info(hdr, pkt);
    return this;
}

JsObject *
NodeWorkItemObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    node_work_item_in_t *in = node_work_item_in();
    bo::shared_ptr<fpi::AsyncHdr>     hdr;
    bo::shared_ptr<fpi::NodeWorkItem> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
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
    bo::shared_ptr<fpi::AsyncHdr>       hdr;
    bo::shared_ptr<fpi::NodeFunctional> pkt;

    hdr = bo::make_shared<fpi::AsyncHdr>();
    pkt = bo::make_shared<fpi::NodeFunctional>();
    pkt->nd_op_code = fpi::NodeUpgradeTypeId;

    init_async_hdr(hdr, fpi::NodeFunctionalTypeId);
    init_domain_info(in->svc_uuid, in->domain_id, &pkt->nd_uuid, &pkt->nd_dom_id);

    NodeWorkFlow::nd_workflow_sgt()->wrk_recv_node_functional(hdr, pkt);
    return this;
}

JsObject *
NodeListStepObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::stringstream    stt;
    node_list_step_in_t *in = node_list_step_in();

    if (strcmp(in->op_code, "local") == 0) {
        NodeWorkFlow::nd_workflow_sgt()->wrk_dump_steps(&stt);
        out->js_push_str(stt.str().c_str());
    } else {
        fpi::DomainID            domain;
        fpi::SvcUuid             svc;
        fpi::NodeEventPtr        pkt;
        bo::shared_ptr<EPSvcRequest> req;

        gl_OmUuid.uuid_assign(&svc);
        pkt = bo::make_shared<fpi::NodeEvent>();
        pkt->nd_dom_id   = domain;
        pkt->nd_uuid     = svc;
        pkt->nd_evt      = "";
        pkt->nd_evt_text = "";

        req = gSvcRequestPool->newEPSvcRequest(svc);
        req->setPayload(FDSP_MSG_TYPEID(fpi::NodeEvent), pkt);
        req->invoke();
    }
    return this;
}

}  // namespace fds
