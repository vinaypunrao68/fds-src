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
                             OmContainer::pointer om)
    : DomainContainer(name, master, sm, dm, am, om, NULL) {}

DomainClusterMap::~DomainClusterMap() {}
DomainClusterMap::DomainClusterMap(char const *const name) : DomainNodeInv(name) {}
DomainClusterMap::DomainClusterMap(char const *const    name,
                                   OmAgent::pointer     master,
                                   SmContainer::pointer sm,
                                   DmContainer::pointer dm,
                                   AmContainer::pointer am,
                                   OmContainer::pointer om)
    : DomainNodeInv(name, master, sm, dm, am, om) {}

DomainResources::~DomainResources() {}
DomainResources::DomainResources(char const *const name)
    : drs_dlt(NULL)
{
    drs_cur_throttle_lvl = 0;
    drs_dmt_version      = 0;
}

PlatEvent::~PlatEvent() {}
PlatEvent::PlatEvent(char const *const          name,
                     DomainResources::pointer   mgr,
                     DomainClusterMap::pointer  clus,
                     const Platform            *plat)
    : pe_name(name), pe_resources(mgr), pe_clusmap(clus), pe_platform(plat) {}

// -------------------------------------------------------------------------------------
// Node Event Handlers
// -------------------------------------------------------------------------------------
void
NodePlatEvent::plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr)
{
}

int
NodePlatEvent::plat_recvNodeEvent(const FDSP_MsgHdrTypePtr  &hdr,
                                  const FDSP_Node_Info_Type &evt)
{
    return 0;
}

int
NodePlatEvent::plat_recvMigrationEvent(const FDSP_MsgHdrTypePtr &hdr,
                                       const FDSP_DLT_Data_Type &dlt)
{
    return 0;
}

int
NodePlatEvent::plat_recvDLTStartMigration(const FDSP_MsgHdrTypePtr    &hdr,
                                          const FDSP_DLT_Data_TypePtr &dlt)
{
    return 0;
}

int
NodePlatEvent::plat_recvDLTUpdate(const FDSP_MsgHdrTypePtr &hdr,
                                  const FDSP_DLT_Data_Type &dlt)
{
    return 0;
}

int
NodePlatEvent::plat_recvDMTUpdate(const FDSP_MsgHdrTypePtr &hdr,
                                  const FDSP_DMT_Type      &dlt)
{
    return 0;
}

// -------------------------------------------------------------------------------------
// Volume Event Handlers
// -------------------------------------------------------------------------------------
void
VolPlatEvent::plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr)
{
}

int
VolPlatEvent::plat_set_throttle(const FDSP_MsgHdrTypePtr      &hdr,
                                const FDSP_ThrottleMsgTypePtr &msg)
{
    return 0;
}

int
VolPlatEvent::plat_bucket_stat(const FDSP_MsgHdrTypePtr          &hdr,
                               const FDSP_BucketStatsRespTypePtr &msg)
{
    return 0;
}

int
VolPlatEvent::plat_add_vol(const FDSP_MsgHdrTypePtr    &hdr,
                           const FDSP_NotifyVolTypePtr &add)
{
    return 0;
}

int
VolPlatEvent::plat_rm_vol(const FDSP_MsgHdrTypePtr &hdr, const FDSP_NotifyVolTypePtr &rm)
{
    return 0;
}

// -------------------------------------------------------------------------------------
// Common Platform Functions
// -------------------------------------------------------------------------------------
void
Platform::plf_create_domain(const FdspCrtDomPtr &msg)
{
}

void
Platform::plf_delete_domain(const FdspCrtDomPtr &msg)
{
}

}  // namespace fds
