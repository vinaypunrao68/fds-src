/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <OmDeploy.h>

namespace fds {

OM_DLTMod                    gl_OMDltMod("OM-DLT");

OM_DLTMod::OM_DLTMod(char const *const name)
    : Module(name)
{
}

int
OM_DLTMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
OM_DLTMod::mod_startup()
{
}

void
OM_DLTMod::mod_shutdown()
{
}

}  // namespace fds
