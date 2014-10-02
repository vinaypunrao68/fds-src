/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <om-domain.h>

namespace fds {

OM_NodeWrkFlow::~OM_NodeWrkFlow() {}
OM_NodeWrkFlow::OM_NodeWrkFlow(char const *const name) : Module(name)
{
}

int
OM_NodeWrkFlow::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void
OM_NodeWrkFlow::mod_startup()
{
}

void
OM_NodeWrkFlow::mod_enable_service()
{
}

void
OM_NodeWrkFlow::mod_shutdown()
{
}

}  // namespace fds
