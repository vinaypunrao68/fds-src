/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform/domain_cluster_map.h"

namespace fds
{
    DomainClusterMap::~DomainClusterMap()
    {
    }

    DomainClusterMap::DomainClusterMap(char const *const name) : DomainContainer(name)
    {
    }

    DomainClusterMap::DomainClusterMap(char const *const name, OmAgent::pointer master,
                                       SmContainer::pointer sm, DmContainer::pointer dm,
                                       AmContainer::pointer am,
                                       PmContainer::pointer pm,
                                       OmContainer::pointer om) : DomainContainer(name, master, sm,
                                                                                  dm, am, pm, om)
    {
    }
}  // namespace fds
