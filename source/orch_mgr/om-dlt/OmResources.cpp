/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
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
}

// om_persist_node_info
// --------------------
//
void
OM_NodeDomainMod::om_persist_node_info(fds_uint32_t idx)
{
}

}  // namespace fds
