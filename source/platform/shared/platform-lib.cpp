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
        : DomainContainer(name, master, sm, dm, am, pm, om) {}

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

// Table of keys used to access the persistent storage DB.
//

PlatformProcess::PlatformProcess(int argc, char **argv,
                                 const std::string  &cfg,
                                 const std::string  &log,
                                 Platform           *platform,
                                 Module            **vec)
    : FdsProcess(argc, argv, "platform.conf", cfg, log, vec),
      plf_mgr(platform), plf_db(NULL),
      plf_test_mode(false), plf_stand_alone(false)
{
    memset(&plf_node_data, 0, sizeof(plf_node_data));
    plf_db_key = proc_root->dir_fdsroot() + "node-root";
    // TODO(Bao): daemonize();  make each platform process a daemon to get core.
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
    if ((plf_test_mode == true) || (plf_stand_alone == true)) {
        return;
    }
    if (plf_db->isConnected() == false) {
        // TODO(Vy): platform daemon should start redis server here...
        //
        LOGNORMAL << "Sorry, you must start redis server manually & try again"
                  << std::endl;
        exit(1);
    }
    std::string val = plf_db->get(plf_db_key);
    if (val == "") {
        // The caller will have to deal with it.
        return;
    }
    memcpy(&plf_node_data, val.data(), sizeof(plf_node_data));

    // TODO(Vy): consistency check on the data read.
    //
}

// plf_apply_node_data
// -------------------
//
void
PlatformProcess::plf_apply_node_data()
{
    plf_mgr->plf_change_info(&plf_node_data);
}

void
PlatformProcess::proc_pre_startup()
{
    plf_db = new kvstore::PlatformDB();
    plf_load_node_data();
    plf_apply_node_data();
}

// -------------------------------------------------------------------------------------
// Platform Utility Functions.
// -------------------------------------------------------------------------------------
/* static */ int
Platform::plf_get_my_node_svc_uuid(fpi::SvcUuid *uuid, fpi::FDSP_MgrIdType type)
{
    NodeUuid   svc_uuid;
    Platform  *plat = Platform::platf_singleton();

    Platform::plf_svc_uuid_from_node(plat->plf_my_uuid, &svc_uuid, type);
    svc_uuid.uuid_assign(uuid);

    return Platform::plf_svc_port_from_node(plat->plf_my_node_port, type);
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
