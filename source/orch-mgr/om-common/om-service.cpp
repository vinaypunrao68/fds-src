/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <orch-mgr/om-service.h>
#include <kvstore/configdbmodule.h>
#include <OmDeploy.h>
#include <OmDmtDeploy.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>

namespace fds {

OM_Module::OM_Module(char const *const name)
    : Module(name)
{
    om_data_place = new DataPlacement();
    om_volume_place = new VolumePlacement();
    /*
     * TODO: Let's use member variables rather than globals.
     * Members are a better OO-design.
     */
    static Module *om_mods[] = {
        &gl_OMNodeDomainMod,
        &gl_OMClusMapMod,
        om_data_place,
        om_volume_place,
        &gl_OMDltMod,
        &gl_OMDmtMod,
        NULL
    };
    mod_intern     = om_mods;
    om_dlt         = &gl_OMDltMod;
    om_dmt         = &gl_OMDmtMod;
    om_clus_map    = &gl_OMClusMapMod;
    om_node_domain = &gl_OMNodeDomainMod;
}

OM_Module::~OM_Module() {
    delete om_data_place;
    delete om_volume_place;
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
