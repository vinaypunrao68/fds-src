/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

ClusterMap                   gl_OMClusMapMod;

ClusterMap::ClusterMap()
    : Module("OM-ClusMap")
{
}

ClusterMap::~ClusterMap()
{
}

int
ClusterMap::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
ClusterMap::mod_startup()
{
}

void
ClusterMap::mod_shutdown()
{
}

}  // namespace fds
