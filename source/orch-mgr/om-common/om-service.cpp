/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <orch-mgr/om-service.h>
#include <kvstore/configdbmodule.h>
#include <OmDeploy.h>
#include <OmDmtDeploy.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <OmResources.h>

namespace fds {

OM_Module::OM_Module(char const *const name)
    : Module(name)
{
    om_data_place = new DataPlacement();
    om_volume_place = new VolumePlacement();
    om_node_domain = new OM_NodeDomainMod("OM-Domain"); // OM-NODE
    om_clus_map = new ClusterMap();
    om_dlt = new OM_DLTMod("OM-DLT"); // OM-DLT
    om_dmt = new OM_DMTMod("OM-DMT"); // OM_DMT
    /*
     * TODO: Let's use member variables rather than globals.
     * Members are a better OO-design.
     */
    static Module *om_mods[7]; // declare first, then assign
    om_mods[0] = om_node_domain;
    om_mods[1] = om_clus_map;
    om_mods[2] = om_data_place;
    om_mods[3] = om_volume_place;
    om_mods[4] = om_dlt;
    om_mods[5] = om_dmt;
    om_mods[6] = NULL;
    mod_intern     = om_mods;
}

OM_Module::~OM_Module() {
    delete om_data_place;
    delete om_volume_place;
    delete om_node_domain;
    delete om_clus_map;
    delete om_dlt;
    delete om_dmt;
}

int
OM_Module::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void
OM_Module::mod_startup()
{
    Module::mod_startup();
}

void
OM_Module::mod_shutdown()
{
    Module::mod_shutdown();
}

void
OM_Module::setOMTestMode(fds_bool_t value) {
    for (unsigned i = 0; i < sizeof(mod_intern); i++) {
        if (mod_intern[i] != nullptr) {
            mod_intern[i]->setTestMode(value);
        }
    }
    setTestMode(value);
}
}  // namespace fds
