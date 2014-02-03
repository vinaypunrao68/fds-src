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
    int              i, num;
    FdspNodeRegPtr   ptr;
    UT_OM_NodeInfo  *node;
    ut_node_info_t  *info;

    std::list<NodeAgent::pointer> newNodes;
    std::list<NodeAgent::pointer> rmNodes;

    ptr = FdspNodeRegPtr(new FdspNodeReg());
    num = parent->js_array_size();
    for (i = 0; i < num; i++) {
        node = static_cast<UT_OM_NodeInfo *>((*parent)[i]);
        info = node->om_node_info();
        std::cout << "Node uuid " << std::hex << info->nd_uuid
            << ", name " << info->nd_node_name << std::endl;

        ResourceUUID r_uuid(info->nd_uuid);
        if (info->add == true) {
            newNodes.push_back(new NodeAgent(r_uuid, info->nd_weight));
        } else {
            rmNodes.push_back(new NodeAgent(r_uuid, info->nd_weight));
        }
        OM_NodeDomainMod::om_local_domain()->om_reg_node_info(&r_uuid, ptr);
    }

    // Update the cluster map
    ProbeMod *mod  = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    DataPlacement *dp = static_cast<DataPlacement *>(om->om_dataplace_mod());

    fds_verify(om == &gl_OMModule);
    const DLT *oldDlt = dp->getCurDlt();
    fds_uint32_t old_depth = 0;
    fds_uint32_t old_tokens = 0;
    fds_uint64_t* old_dlt_ptr = copy_dlt(oldDlt);
    if (oldDlt != NULL) {
        old_depth = oldDlt->getDepth();
        old_tokens = oldDlt->getNumTokens();
    }

    dp->updateMembers(newNodes, rmNodes);

    // Recompute the DLT
    dp->computeDlt();

    const DLT *dlt = dp->getCurDlt();
    fds_uint64_t new_depth = dlt->getDepth();
    fds_uint32_t new_tokens = dlt->getNumTokens();
    fds_uint64_t* new_dlt_ptr = copy_dlt(dlt);

    // Print the DLT
    std::cout << "Old DLT: " << std::endl;
    print_dlt(old_dlt_ptr, old_depth, old_tokens);
    std::cout << "New DLT: " << std::endl;
    print_dlt(new_dlt_ptr, new_depth, new_tokens);

    compare_dlts(old_dlt_ptr, old_depth, old_tokens, new_dlt_ptr, new_depth, new_tokens);

    if (old_dlt_ptr)
        delete[] old_dlt_ptr;
    delete[] new_dlt_ptr;

    return this;  //  to free this obj
}

fds_uint64_t*
UT_OM_NodeInfo::copy_dlt(const DLT* dlt)
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
UT_OM_NodeInfo::compare_dlts(const fds_uint64_t* old_tbl,
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
UT_OM_NodeInfo::print_dlt(const fds_uint64_t* tbl,
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
            dlt->dlt_deploy_event(DltUpdateEvt(NULL));
            break;

        case DLT_EVT_UPDATE_DONE:
            dlt->dlt_deploy_event(DltUpdateOkEvt());
            break;

        case DLT_EVT_COMMIT:
            dlt->dlt_deploy_event(DltCommitEvt());
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
