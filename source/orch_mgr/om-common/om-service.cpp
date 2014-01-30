/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmDataPlacement.h>

namespace fds {

OM_Module                    gl_OMModule("OM");

OM_Module::OM_Module(char const *const name)
    : Module(name)
{
    /*
     * TODO: Let's use member variables rather than globals.
     * Members are a better OO-design.
     */
    static Module *om_mods[] = {
        &gl_OMNodeDomainMod,
        &gl_OMClusMapMod,
        &gl_OMDltMod,
        NULL
    };
    mod_intern = om_mods;

    om_clus_map = &gl_OMClusMapMod;
}

OM_Module::~OM_Module() {}

int
OM_Module::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
OM_Module::mod_startup()
{
}

void
OM_Module::mod_shutdown()
{
}

}  // namespace fds
