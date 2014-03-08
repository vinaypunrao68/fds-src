/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
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
PlatEvent::PlatEvent(char const *const          name,
                     DomainResources::pointer   mgr,
                     DomainClusterMap::pointer  clus,
                     const Platform            *plat)
    : pe_name(name), pe_resources(mgr), pe_clusmap(clus), pe_platform(plat) {}

// -------------------------------------------------------------------------------------
// FDS Platform Process
// -------------------------------------------------------------------------------------
PlatformProcess::PlatformProcess(int argc, char **argv,
                                 const std::string  &cfg,
                                 Platform           *platform,
                                 Module            **vec)
    : FdsProcess(argc, argv, "platform.conf", cfg, "platform.log", vec),
      plf_mgr(platform), plf_db(NULL)
{
    memset(&plf_node_data, 0, sizeof(plf_node_data));
}

PlatformProcess::~PlatformProcess()
{
    delete plf_db;
}

// plf_load_node_data
// ------------------
//
void
PlatformProcess::plf_load_node_data()
{
    if (plf_test_mode == true) {
        return;
    }
    if (plf_db->isConnected() == false) {
        std::cout << "Sorry, you must start redis server manually & try again"
                  << std::endl;
        exit(1);
    }
    std::string val = plf_db->get("NodeUUID");
    if (val == "") {
        plf_node_data.node_uuid = fds_get_uuid64(get_uuid());
        std::cout << "No uuid, generate one " << std::hex
                  << plf_node_data.node_uuid << std::endl;
    } else {
        memcpy(&plf_node_data, val.data(), sizeof(plf_node_data));
    }
}

// plf_apply_node_data
// -------------------
//
void
PlatformProcess::plf_apply_node_data()
{
    char         name[64];
    NodeUuid     my_uuid(plf_node_data.node_uuid);

    if (plf_node_data.node_magic != 0) {
        snprintf(name, sizeof(name), "node-%u", plf_node_data.node_number);
        plf_mgr->plf_change_info(my_uuid, std::string(name), 0);
    } else {
        plf_mgr->plf_change_info(my_uuid, std::string(""), 0);
    }
}

void
PlatformProcess::setup()
{
    FdsProcess::setup();
    plf_db = new kvstore::PlatformDB();

    plf_load_node_data();
    plf_apply_node_data();
}

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
