/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/platform-lib.h>

namespace fds {

DomainNodeInv::~DomainNodeInv() {}
DomainNodeInv::DomainNodeInv(char const *const name) : DomainContainer(name) {}
DomainNodeInv::DomainNodeInv(char const *const    name,
                             OmAgent::pointer     master,
                             SmContainer::pointer sm,
                             DmContainer::pointer dm,
                             AmContainer::pointer am,
                             PmContainer::pointer pm,
                             OmContainer::pointer om)
        : DomainContainer(name, master, sm, dm, am, pm, om, NULL) {}

DomainClusterMap::~DomainClusterMap() {}
DomainClusterMap::DomainClusterMap(char const *const name) : DomainNodeInv(name) {}
DomainClusterMap::DomainClusterMap(char const *const    name,
                                   OmAgent::pointer     master,
                                   SmContainer::pointer sm,
                                   DmContainer::pointer dm,
                                   AmContainer::pointer am,
                                   PmContainer::pointer pm,
                                   OmContainer::pointer om)
        : DomainNodeInv(name, master, sm, dm, am, pm, om) {}

DomainResources::~DomainResources() {}
DomainResources::DomainResources(char const *const name)
    : drs_dlt(NULL)
{
    drs_cur_throttle_lvl = 0;
    drs_dmt_version      = 0;
}

PlatEvent::~PlatEvent() {}
PlatEvent::PlatEvent(char const *const name) : pe_name(name) {}

// -------------------------------------------------------------------------------------
// Common Platform Functions
// -------------------------------------------------------------------------------------


}  // namespace fds
