/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <om-domain.h>
#include <om-node-workflow.h>

namespace fds {

OM_NodeWrkFlow               gl_OmNodeWorkMod("OMNodeWrklow");

OM_NodeWrkFlow::~OM_NodeWrkFlow() {}
OM_NodeWrkFlow::OM_NodeWrkFlow(char const *const name) : Module(name) {}

int
OM_NodeWrkFlow::mod_init(SysParams const *const param)
{
    static Module *om_node_wrkflow[] = {
        &gl_OmWorkFlow,
        NULL
    };
    mod_intern = om_node_wrkflow;
    return Module::mod_init(param);
}

void
OM_NodeWrkFlow::mod_startup()
{
    Module::mod_startup();
}

void
OM_NodeWrkFlow::mod_enable_service()
{
    Module::mod_enable_service();
}

void
OM_NodeWrkFlow::mod_shutdown()
{
    Module::mod_shutdown();
}

}  // namespace fds
