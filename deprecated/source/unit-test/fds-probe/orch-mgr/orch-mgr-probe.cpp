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
#include <OmVolumePlacement.h>

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
            domain->om_del_services(r_uuid, info->nd_node_name, true, true, true);
        }
    }

    // Update the cluster map
    ProbeMod *mod  = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    DataPlacement *dp = om->om_dataplace_mod();
    boost::shared_ptr<UT_DLT_EvalHelper> eval_helper(new UT_DLT_EvalHelper);

    fds_verify(om == &gl_OMModule);
    const DLT *oldDlt = dp->getCommitedDlt();
    eval_helper->setOldDlt(oldDlt);

    // Drive cluster map update via state machine
    domain->om_dlt_update_cluster();

    // Get new/old dlt states
    const DLT *dlt = dp->getCommitedDlt();
    eval_helper->setNewDlt(dlt);

    // Print the DLTs and compare them
    eval_helper->printAndCompareDlts(NULL);

    return this;  //  to free this obj
}

JsObject *
UT_DP_NodeInfo::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    int               i, num;
    FdspNodeRegPtr    ptr;
    UT_OM_NodeInfo   *node;
    ut_node_info_t   *info;
    OM_NodeDomainMod *domain;

    NodeList newNodes, rmNodes;

    std::cout << "hello here" << std::endl;
    return this;

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
            OM_SmAgent::pointer agent(new OM_SmAgent(r_uuid, fpi::FDSP_STOR_MGR));
            agent->node_fill_inventory(ptr);
            agent->node_set_weight(info->nd_weight);
            newNodes.push_back(agent);
        } else {
            OM_SmAgent::pointer agent(new OM_SmAgent(r_uuid, fpi::FDSP_STOR_MGR));
            agent->node_fill_inventory(ptr);
            agent->node_set_weight(info->nd_weight);
            rmNodes.push_back(agent);
        }
    }

    // Get modules and print current DLT
    ProbeMod *mod  = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    DataPlacement *dp = om->om_dataplace_mod();
    boost::shared_ptr<UT_DLT_EvalHelper> eval_helper(new UT_DLT_EvalHelper);

    const DLT *oldDlt = dp->getCommitedDlt();
    eval_helper->setOldDlt(oldDlt);

    // Update cluster map and recompute the DLT
    dp->updateMembers(newNodes, rmNodes);
    dp->computeDlt();
    dp->commitDlt();

    // Get new dlt states
    const DLT *dlt = dp->getCommitedDlt();
    eval_helper->setNewDlt(dlt);

    // Print the DLTs and compare
    ClusterMap *cm = om->om_clusmap_mod();
    eval_helper->printAndCompareDlts(cm);

    return this;  //  to free this obj
}

JsObject *
UT_VP_NodeInfo::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    int               i, num;
    FdspNodeRegPtr    ptr;
    fpi::FDSP_VolumeInfoType fdsp_vol_info;
    UT_VP_NodeInfo   *node;
    ut_resource_info_t   *info;
    OM_NodeDomainMod *domain;

    NodeList newNodes, rmNodes;

    domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = loc_domain->om_vol_mgr();
    ptr = FdspNodeRegPtr(new FdspNodeReg());
    num = parent->js_array_size();

    for (i = 0; i < num; i++) {
        node = static_cast<UT_VP_NodeInfo *>((*parent)[i]);
        info = node->vp_node_info();

        if (info->rs_type == ut_rs_dm) {
            std::cout << "DM uuid " << std::hex << info->rs_uuid << std::dec
                      << ", name " << info->rs_name
                      << " add?" << info->add <<  std::endl;
        } else {
            std::cout << "Volume uuid " << std::hex << info->rs_uuid << std::dec
                      << ", name " << info->rs_name
                      << " add?" << info->add <<  std::endl;
        }

        ResourceUUID r_uuid(info->rs_uuid);

        if (info->add == true) {
            if (info->rs_type == ut_rs_dm) {
                OM_DmAgent::pointer agent(new OM_DmAgent(r_uuid, fpi::FDSP_DATA_MGR));
                agent->node_fill_inventory(ptr);
                newNodes.push_back(agent);
            } else {
                VolumeInfo::pointer vol;
                vol = VolumeInfo::vol_cast_ptr(volumes->rs_alloc_new(r_uuid));
                fdsp_vol_info.vol_name = std::string(info->rs_name);
                vol->vol_mk_description(fdsp_vol_info);
                volumes->rs_register(vol);
            }
        } else {
            if (info->rs_type == ut_rs_dm) {
                OM_DmAgent::pointer agent(new OM_DmAgent(r_uuid, fpi::FDSP_DATA_MGR));
                agent->node_fill_inventory(ptr);
                rmNodes.push_back(agent);
            } else {
                std::cout << "Removing volume not supported yet, ignoring"
                          << std::endl;
            }
        }
    }

    if ((newNodes.size() == 0) && (rmNodes.size() == 0)) {
        std::cout << "Did not add or remove any DMs, done" << std::endl;
        return this;
    }

    // Get modules and print current DMT
    ProbeMod *mod  = out->js_get_context();
    OM_Module *om  = static_cast<OM_Module *>(mod->pr_get_owner_module());
    VolumePlacement *vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    if (vp->hasCommittedDMT()) {
        DMTPtr oldDmt = vp->getCommittedDMT();
        std::cout << "Old Committed DMT: " << std::endl;
        std::cout << *oldDmt << std::endl;
    } else {
        std::cout << "Old Committed DMT: NULL" << std::endl;
    }

    // Update cluster map and recompute the DMT
    cm->updateMap(fpi::FDSP_DATA_MGR, newNodes, rmNodes);
    vp->computeDMT(cm);

    // do rebalance to see which vols we will pusj
    NodeUuidSet push_meta_dms;
    vp->beginRebalance(cm, &push_meta_dms);
    vp->commitDMT();
    cm->resetPendServices(fpi::FDSP_DATA_MGR);

    // print newly committed DMT
    fds_verify(vp->hasCommittedDMT());
    DMTPtr newDmt = vp->getCommittedDMT();
    std::cout << "New committed DMT:" << std::endl;
    std::cout << *newDmt << std::endl;

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
    fds_uint32_t move_l1_toks = 0;
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
                    if (j == 0)
                        move_l1_toks++;
                }
            }
        }
    }
    std::cout << "DLT changes: will migrate " << move_toks << " tokens" << std::endl;
    std::cout << "      --- " << move_l1_toks << " primary tokens" << std::endl;
}

void
UT_DLT_EvalHelper::print_dlt(const fds_uint64_t* tbl,
                             fds_uint32_t depth,
                             fds_uint32_t toks,
                             const ClusterMap *cm)
{
    if (tbl == NULL) {
        std::cout << "NULL" << std::endl;
        return;
    }
    for (fds_uint32_t j = 0; j < depth; ++j) {
        for (fds_uint32_t i = 0; i < toks; ++i) {
            fds_verify(tbl[j*toks + i] != 0);
            std::cout << std::hex << tbl[j*toks + i] << std::dec << " ";
        }
        std::cout << std::endl;
    }
    if (cm == NULL) {
        return;  // don't print optimal vs. actual
    }
    // primary tokens optimal vs. actual
    PlacementMetricsPtr metricsPtr(new PlacementMetrics(cm, toks, depth));
    NodeUuidSet nodes;
    for (ClusterMap::const_sm_iterator cit = cm->cbegin_sm();
         cit != cm->cend_sm();
         ++cit) {
        nodes.insert(cit->first);
    }
    NodeUuidSet::const_iterator sit, sit2, sit3;
    for (sit = nodes.cbegin(); sit != nodes.cend(); ++sit) {
        // calculate number of tokens
        NodeUuid uuid = *sit;
        int node_toks = 0;
        int rounded_opt_toks = metricsPtr->tokens(uuid);
        for (fds_uint32_t i = 0; i < toks; ++i) {
            if (tbl[i] == uuid.uuid_get_val()) {
                node_toks++;
            }
        }
        std::cout << "Node " << std::hex << uuid.uuid_get_val() << std::dec
                  << " -- tokens " << node_toks << " ; optimal "
                  << metricsPtr->optimalTokens(uuid)
                  << " ; integer optimal " << rounded_opt_toks
                  << " (error " << node_toks - rounded_opt_toks << ")" << std::endl;
        FDS_PLOG(g_fdslog)
                << "UT: Node " << std::hex << uuid.uuid_get_val() << std::dec
                << " -- tokens " << node_toks << "; optimal "
                << metricsPtr->optimalTokens(uuid)
                << "; round. opt. " << rounded_opt_toks
                << " (err " << node_toks - rounded_opt_toks << ")";
    }
    // print secondary tokens if DLT depth is at least 2
    if (depth < 2) return;
    double total_positive_error = 0;
    double positive_err_count = 0;
    double total_neg_error = 0;
    double negative_err_count = 0;
    for (sit = nodes.cbegin(); sit != nodes.cend(); ++sit) {
        NodeUuid l1_uuid = *sit;
        for (sit2 = nodes.cbegin(); sit2 != nodes.cend(); ++sit2) {
            NodeUuid l2_uuid = *sit2;
            int l12_toks = 0;
            if (l1_uuid == l2_uuid) continue;

            // calculate number of tokens in group l1_uuid, l2_uuid
            for (fds_uint32_t i = 0; i < toks; ++i) {
                if ((tbl[i] == l1_uuid.uuid_get_val()) &&
                    (tbl[toks + i] == l2_uuid.uuid_get_val())) {
                    ++l12_toks;
                }
            }
            double opt_toks = metricsPtr->optimalTokens(l1_uuid, l2_uuid);
            int rounded_toks = metricsPtr->tokens(l1_uuid, l2_uuid);
            int err = l12_toks - rounded_toks;
            std::cout << "Node group (" << std::hex << l1_uuid.uuid_get_val()
                      << ", " << l2_uuid.uuid_get_val() << std::dec
                      << ") -- tokens " << l12_toks << "; optimal " << opt_toks
                      << " ; rounded optimal " << rounded_toks
                      << " (error " << err << ")" << std::endl;
            FDS_PLOG(g_fdslog)
                    << "Node group (" << std::hex << l1_uuid.uuid_get_val()
                    << ", " << l2_uuid.uuid_get_val() << std::dec
                    << ") -- tokens " << l12_toks << "; optimal " << opt_toks
                    << "; round. opt. " << rounded_toks
                    << " (err " << err << ")";
            if (err > 0) {
                total_positive_error += err;
                positive_err_count++;
            } else if (err < 0) {
                total_neg_error -= err;
                negative_err_count++;
            }
        }
    }
    double ave_err = (total_positive_error == 0) ?
            0 : total_positive_error / positive_err_count;
    std::cout << "Total L1-2 group positive err " << total_positive_error
              << " (average " << ave_err << ")" << std::endl;
    ave_err = (total_neg_error == 0) ?
            0 : total_neg_error / negative_err_count;
    std::cout << "Total L1-2 group negative err " << total_neg_error
              << " (average " << ave_err << ")" << std::endl;

    // print third level tokens if DLT depth is at least 3
    if (depth < 3) return;
    total_positive_error = 0;
    total_neg_error = 0;
    positive_err_count = 0;
    negative_err_count = 0;
    for (sit = nodes.cbegin(); sit != nodes.cend(); ++sit) {
        NodeUuid l1_uuid = *sit;
        for (sit2 = nodes.cbegin(); sit2 != nodes.cend(); ++sit2) {
            NodeUuid l2_uuid = *sit2;
            if (l1_uuid == l2_uuid) continue;
            for (sit3 = nodes.cbegin(); sit3 != nodes.cend(); ++sit3) {
                NodeUuid l3_uuid = *sit3;
                int l123_toks = 0;
                if ((l1_uuid == l3_uuid) || (l2_uuid == l3_uuid))
                    continue;

                // calculate number of tokens in group l1_uuid, l2_uuid, l3_uuid
                for (fds_uint32_t i = 0; i < toks; ++i) {
                    if ((tbl[i] == l1_uuid.uuid_get_val()) &&
                        (tbl[toks + i] == l2_uuid.uuid_get_val()) &&
                        (tbl[2*toks + i] == l3_uuid.uuid_get_val())) {
                        ++l123_toks;
                    }
                }

                double opt_123toks = metricsPtr->optimalTokens(l1_uuid, l2_uuid, l3_uuid);
                double err = l123_toks - opt_123toks;
                FDS_PLOG(g_fdslog)
                        << "Node group (" << std::hex << l1_uuid.uuid_get_val()
                        << ", " << l2_uuid.uuid_get_val() << ", "
                        << l3_uuid.uuid_get_val() << std::dec
                        << ") -- tokens " << l123_toks << "; optimal " << opt_123toks
                        << " (err " << err << ")";
                if (err > 0) {
                    total_positive_error += err;
                    positive_err_count++;
                } else if (err < 0) {
                    total_neg_error += err;
                    negative_err_count++;
                }
            }
        }
    }
    ave_err = (total_positive_error == 0) ?
            0 : total_positive_error / positive_err_count;
    std::cout << "Total L1-2-3 group positive err " << total_positive_error
              << " (average " << ave_err << ")" << std::endl;
    ave_err = (total_neg_error == 0) ?
            0 : total_neg_error / negative_err_count;
    std::cout << "Total L1-2-3 group negative err " << total_neg_error
              << " (average " << ave_err << ")" << std::endl;
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
            dlt->dlt_deploy_event(DltComputeEvt());
            break;

        case DLT_EVT_UPDATE_DONE:
            dlt->dlt_deploy_event(DltRebalOkEvt(NULL, NULL));
            break;

        case DLT_EVT_COMMIT_DONE:
            dlt->dlt_deploy_event(DltCommitOkEvt(0, NodeUuid()));
            break;
        default:
            break;
        }
        std::cout << "-> " << dlt->dlt_deploy_curr_state() << std::endl;
    }
    return this;  // to free this obj.
}
}  // namespace fds
