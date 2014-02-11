/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace OM with your namespace.
 */
#include <orch-mgr-probe.h>
#include <list>
#include <string>
#include <iostream>
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmDataPlacement.h>

namespace fds {

probe_mod_param_t OM_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

OM_ProbeMod gl_OM_ProbeMod("OM Probe Adapter",
                           &OM_probe_param, &gl_OMModule);

// pr_new_instance
// ---------------
//
ProbeMod *
OM_ProbeMod::pr_new_instance()
{
    OM_ProbeMod *adapter = new OM_ProbeMod("OM Inst", &OM_probe_param, &gl_OMModule);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
OM_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
OM_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
OM_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
OM_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
OM_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
OM_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
OM_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
OM_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Setup");
        svc->js_register_template(new UT_OMSetupTemplate(mgr));

        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new UT_OMRuntimeTempl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
OM_ProbeMod::mod_shutdown()
{
}

// -------------------------------------------------------------------------------------
// OM REST Test API
// -------------------------------------------------------------------------------------

JsObject *
UT_OM_NodeInfo::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    int               i, num;
    FdspNodeRegPtr    ptr;
    UT_OM_NodeInfo   *node;
    ut_node_info_t   *info;
    OM_NodeDomainMod *domain;

    domain = OM_NodeDomainMod::om_local_domain();
    ptr = FdspNodeRegPtr(new FdspNodeReg());
    num = parent->js_array_size();

    for (i = 0; i < num; i++) {
        node = static_cast<UT_OM_NodeInfo *>((*parent)[i]);
        info = node->om_node_info();
        std::cout << "Node uuid " << std::hex << info->nd_uuid
            << ", name " << info->nd_node_name << std::endl;

        // TODO(vy): encode this in json format
        ptr->node_name = info->nd_node_name;
        ptr->disk_info.ssd_capacity = info->nd_weight;
        ResourceUUID r_uuid(info->nd_uuid);

        if (info->add == true) {
            domain->om_reg_node_info(r_uuid, ptr);
        } else {
            domain->om_del_node_info(r_uuid, info->nd_node_name);
        }
    }

    // Update the cluster map
    ProbeMod *mod  = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    DataPlacement *dp = om->om_dataplace_mod();
    boost::shared_ptr<UT_DLT_EvalHelper> eval_helper(new UT_DLT_EvalHelper);

    fds_verify(om == &gl_OMModule);
    const DLT *oldDlt = dp->getCurDlt();
    eval_helper->setOldDlt(oldDlt);

    // Drive cluster map update via state machine
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DltCompEvt event(dp);
    dltMod->dlt_deploy_event(event);

    // Get new/old dlt states
    const DLT *dlt = dp->getCurDlt();
    eval_helper->setNewDlt(dlt);

    // Print the DLTs and compare them
    eval_helper->printAndCompareDlts();

    return this;  //  to free this obj
}

JsObject *
UT_DP_NodeInfo::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    int               i, num;
    NodeList          newNodes, rmNodes;
    FdspNodeRegPtr    ptr;
    UT_OM_NodeInfo   *node;
    ut_node_info_t   *info;
    OM_NodeDomainMod *domain;

    domain = OM_NodeDomainMod::om_local_domain();
    ptr = FdspNodeRegPtr(new FdspNodeReg());
    num = parent->js_array_size();

    for (i = 0; i < num; i++) {
        node = static_cast<UT_OM_NodeInfo *>((*parent)[i]);
        info = node->om_node_info();
        std::cout << "Node uuid " << std::hex << info->nd_uuid
                  << ", name " << info->nd_node_name
                  << ", weight " << std::dec << info->nd_weight << std::endl;

        // TODO(vy): encode this in json format
        ptr->disk_info.ssd_capacity = info->nd_weight;
        ResourceUUID r_uuid(info->nd_uuid);

        if (info->add == true) {
            domain->om_reg_node_info(r_uuid, ptr);
        } else {
            domain->om_del_node_info(r_uuid, info->nd_node_name);
        }
    }

    // Get modules and print current DLT
    ProbeMod *mod  = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    DataPlacement *dp = om->om_dataplace_mod();
    boost::shared_ptr<UT_DLT_EvalHelper> eval_helper(new UT_DLT_EvalHelper);
    OM_SmContainer::pointer smNodes = domain->om_sm_nodes();

    const DLT *oldDlt = dp->getCurDlt();
    eval_helper->setOldDlt(oldDlt);

    // Update cluster map and recompute the DLT
    smNodes->om_splice_nodes_pend(&newNodes, &rmNodes);
    dp->updateMembers(newNodes, rmNodes);
    dp->computeDlt();

    // Get new dlt states
    const DLT *dlt = dp->getCurDlt();
    eval_helper->setNewDlt(dlt);

    // Print the DLTs and compare
    eval_helper->printAndCompareDlts();

    return this;  //  to free this obj
}


fds_uint64_t*
UT_DLT_EvalHelper::copy_dlt(const DLT* dlt)
{
    if (dlt == NULL)
        return NULL;

    fds_uint32_t depth = dlt->getDepth();
    fds_uint32_t toks = dlt->getNumTokens();
    fds_uint64_t *tbl = new fds_uint64_t[depth * toks];
    for (fds_token_id i = 0; i < toks; ++i) {
        DltTokenGroupPtr column = dlt->getNodes(i);
        for (fds_uint32_t j = 0; j < depth; ++j) {
            tbl[j*toks + i] = (column->get(j)).uuid_get_val();
        }
    }

    return tbl;
}

// compare_dlts
// ------------
// Calculate and output the number of tokens that will have to be migrated
// because of the changes in the DLT. In each column in DLT,
// we count number of node uuids that were not in this column
// in old DLT, and calculate the some of these.
//
void
UT_DLT_EvalHelper::compare_dlts(const fds_uint64_t* old_tbl,
                                 fds_uint32_t old_depth,
                                 fds_uint32_t old_toks,
                                 const fds_uint64_t* new_tbl,
                                 fds_uint32_t new_depth,
                                 fds_uint32_t new_toks) {
    fds_uint32_t move_toks = 0;
    fds_uint32_t min_depth = old_depth;
    if (min_depth > new_depth) {
        min_depth = new_depth;
    }

    if (old_tbl == NULL)
        return;

    if (old_toks != new_toks) {
        std::cout << "Cannot compare DLTs of different width" << std::endl;
        return;
    }

    // count number of cells that changed, this is the number
    // of tokens that will have to be migrated
    for (fds_uint32_t j = 0; j < new_depth; ++j) {
        for (fds_uint32_t i = 0; i < new_toks; ++i) {
            fds_uint64_t cur_node = new_tbl[j*new_toks + i];
            if ((j >= old_depth) ||
                (old_tbl[j*new_toks + i] != cur_node)) {
                // the cell changed, but a node uuid in new DLT
                // could have been in the same column in old DLT
                // which means it already has the token, so search old DLT
                fds_bool_t found = false;
                for (fds_uint32_t k = 0; k < old_depth; ++k) {
                    if (old_tbl[k*new_toks + i] == cur_node) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    move_toks++;
                }
            }
        }
    }
    std::cout << "DLT changes: will migrate " << move_toks << " tokens" << std::endl;
}

void
UT_DLT_EvalHelper::print_dlt(const fds_uint64_t* tbl,
                             fds_uint32_t depth,
                             fds_uint32_t toks)
{
    if (tbl == NULL) {
        std::cout << "NULL" << std::endl;
        return;
    }
    for (fds_uint32_t j = 0; j < depth; ++j) {
        for (fds_uint32_t i = 0; i < toks; ++i) {
            std::cout << std::hex << tbl[j*toks + i] << std::dec << " ";
        }
        std::cout << std::endl;
    }
}

// -------------------------------------------------------------------------------------
// OM Data Runtime Test API
// -------------------------------------------------------------------------------------

JsObject *
UT_OM_DltFsm::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    int              i, num;
    UT_OM_DltFsm    *inp;
    ut_dlt_fsm_in_t *evt;

    ProbeMod  *mod = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    OM_DLTMod *dlt = static_cast<OM_DLTMod *>(om->om_dlt_mod());

    num = parent->js_array_size();
    for (i = 0; i < num; i++) {
        inp = static_cast<UT_OM_DltFsm *>((*parent)[i]);
        evt = inp->om_dlt_evt();

        std::cout << "DLT Event " << evt->dlt_evt << std::endl;
        switch (evt->dlt_evt) {
        case DLT_EVT_COMPUTE:
            dlt->dlt_deploy_event(DltCompEvt(NULL));
            break;

        case DLT_EVT_UPDATE:
            dlt->dlt_deploy_event(DltRebalEvt(NULL));
            break;

        case DLT_EVT_UPDATE_DONE:
            dlt->dlt_deploy_event(DltRebalOkEvt(NULL, NULL));
            break;

        case DLT_EVT_COMMIT_DONE:
            dlt->dlt_deploy_event(DltCommitOkEvt());
            break;
        }
        std::cout << "-> " << dlt->dlt_deploy_curr_state() << std::endl;
    }
    return this;  // to free this obj.
}

}  // namespace fds
