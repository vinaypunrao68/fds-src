/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

OM_NodeDomainMod             gl_OMNodeDomainMod("OM-Node");

OM_NodeDomainMod::OM_NodeDomainMod(char const *const name)
    : Module(name)
{
}

OM_NodeDomainMod::~OM_NodeDomainMod() {}

int
OM_NodeDomainMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
OM_NodeDomainMod::mod_startup()
{
}

void
OM_NodeDomainMod::mod_shutdown()
{
}

// om_local_domain
// ---------------
//
OM_NodeDomainMod *
OM_NodeDomainMod::om_local_domain()
{
    return &gl_OMNodeDomainMod;
}

// om_reg_node_info
// ----------------
//
void
OM_NodeDomainMod::om_reg_node_info(const NodeUuid       *uuid,
                                   const FdspNodeRegPtr msg)
{
    fds_bool_t         add;
    NodeAgent::pointer agent;

    add   = false;
    agent = NULL;
    if (uuid != NULL) {
        agent = om_node_info(uuid);
    }
    if (agent == NULL) {
        add   = true;
        agent = om_new_node();
    }
    fds_verify(agent != NULL);
    agent->node_update_info(uuid, msg);
    if (add == true) {
        om_activate_node(agent->node_index());
    }
    /* XXX: TODO (vy), remove this code once we have node FSM */
    OM_Module *om = OM_Module::om_singleton();
    ClusterMap *clus = static_cast<ClusterMap *>(om->om_clusmap_mod());

    if (clus->getNumMembers() == 0) {
        static int node_up_cnt = 0;

        node_up_cnt++;
        if (node_up_cnt < 3) {
            std::cout << "Batch up node up, cnt " << node_up_cnt << std::endl;
            return;
        }
    }
    om_update_cluster_map();
}

// om_del_node_info
// ----------------
//
void
OM_NodeDomainMod::om_del_node_info(const NodeUuid *uuid)
{
    NodeAgent::pointer agent;

    agent = om_node_info(uuid);
    if (agent != NULL) {
        om_deactivate_node(agent->node_index());
    }
    om_update_cluster_map();
}

// om_persist_node_info
// --------------------
//
void
OM_NodeDomainMod::om_persist_node_info(fds_uint32_t idx)
{
}

// om_update_cluster_map
// ---------------------
//
void
OM_NodeDomainMod::om_update_cluster_map()
{
    NodeList       addNodes, rmNodes;
    OM_Module     *om;
    OM_DLTMod     *dlt;
    ClusterMap    *clus;
    DataPlacement *dp;

    node_mtx.lock();
    addNodes.splice(addNodes.begin(), node_up_pend);
    rmNodes.splice(rmNodes.begin(), node_down_pend);
    node_mtx.unlock();

    om   = OM_Module::om_singleton();
    dp   = om->om_dataplace_mod();
    dlt  = om->om_dlt_mod();
    clus = om->om_clusmap_mod();

    std::cout << "Call cluster update map" << std::endl;

    clus->updateMap(addNodes, rmNodes);
    // dlt->dlt_deploy_event(DltCompEvt(dp));
}

}  // namespace fds
