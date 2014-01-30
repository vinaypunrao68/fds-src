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

void
OM_NodeDomainMod::om_reg_node_info(const ResourceUUID   &uuid,
                                   const FdspNodeRegMsg *msg)
{
}

void
OM_NodeDomainMod::om_del_node_info(const ResourceUUID &uuid)
{
}

void
OM_NodeDomainMod::om_persist_node_info(int idx)
{
}

int
OM_NodeDomainMod::om_avail_nodes()
{
    return 16;
}

NodeAgent::pointer
OM_NodeDomainMod::om_node_info(int node_idx)
{
    return NULL;
}

NodeAgent::pointer
OM_NodeDomainMod::om_node_info(const ResourceUUID &uuid)
{
    return NULL;
}

}  // namespace fds
