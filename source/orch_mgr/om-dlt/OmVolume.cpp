/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <OmVolume.h>
#include <OmResources.h>
#include <orchMgr.h>

namespace fds {

// --------------------------------------------------------------------------------------
// Volume Info
// --------------------------------------------------------------------------------------
VolumeInfo::VolumeInfo(const ResourceUUID &uuid)
    : Resource(uuid), vol_properties(NULL) {}

VolumeInfo::~VolumeInfo()
{
    delete vol_properties;
}

// vol_mk_description
// ------------------
//
void
VolumeInfo::vol_mk_description(const fpi::FDSP_VolumeInfoType &info)
{
    vol_properties = new VolumeDesc(info, rs_uuid.uuid_get_val());
}

// vol_apply_description
// ---------------------
//
void
VolumeInfo::vol_apply_description(const VolumeDesc &desc)
{
    VolumeDesc *mine;

    mine = vol_properties;
    mine->volPolicyId  = desc.volPolicyId;
    mine->iops_min     = desc.iops_min;
    mine->iops_max     = desc.iops_max;
    mine->relativePrio = desc.relativePrio;
}

// vol_fmt_desc_pkt
// ----------------
//
void
VolumeInfo::vol_fmt_desc_pkt(FDSP_VolumeDescType *pkt) const
{
    VolumeDesc *pVol;

    pVol               = vol_properties;
    pkt->vol_name      = pVol->name;
    pkt->volUUID       = pVol->volUUID;
    pkt->tennantId     = pVol->tennantId;
    pkt->localDomainId = pVol->localDomainId;
    pkt->globDomainId  = pVol->globDomainId;

    pkt->capacity      = pVol->capacity;
    pkt->volType       = pVol->volType;
    pkt->maxQuota      = pVol->maxQuota;
    pkt->defReplicaCnt = pVol->replicaCnt;

    pkt->volPolicyId   = pVol->volPolicyId;
    pkt->iops_max      = pVol->iops_max;
    pkt->iops_min      = pVol->iops_min;
    pkt->rel_prio      = pVol->relativePrio;

    pkt->defConsisProtocol = fpi::FDSP_ConsisProtoType(pVol->consisProtocol);
    pkt->appWorkload       = pVol->appWorkload;
}

// vol_am_agent
// ------------
//
NodeAgent::pointer
VolumeInfo::vol_am_agent(const std::string &am_node)
{
    NodeUuid           am_uuid(fds_get_uuid64(am_node));
    OM_NodeContainer  *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    return local->om_am_agent(am_uuid);
}

// vol_attach_node
// ---------------
//
void
VolumeInfo::vol_attach_node(const std::string &node_name)
{
    OM_AmAgent::pointer  am_agent;

    am_agent = OM_AmAgent::agt_cast_ptr(vol_am_agent(node_name));
    if (am_agent == NULL) {
        // Provisioning vol before the AM is online.
        //
        FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Received attach vol " << vol_name
            << ", am node " << node_name << std::endl;
        return;
    }
    // TODO(Vy): not thread safe here...
    //
    for (int i = 0; i < vol_am_nodes.size(); i++) {
        if (vol_am_nodes[i] == node_name) {
            FDS_PLOG_SEV(g_fdslog, fds_log::normal)
                << "Volume " << vol_name
                << " is already attached to node " << node_name << std::endl;
            return;
        }
    }
    vol_am_nodes.push_back(node_name);
    am_agent->om_send_vol_cmd(this, fpi::FDSP_MSG_ATTACH_VOL_CTRL);
}

// vol_detach_node
// ---------------
//
void
VolumeInfo::vol_detach_node(const std::string &node_name)
{
    OM_AmAgent::pointer  am_agent;

    am_agent = OM_AmAgent::agt_cast_ptr(vol_am_agent(node_name));
    if (am_agent == NULL) {
        // Provisioning vol before the AM is online.
        //
        FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Received attach vol " << vol_name
            << ", am node " << node_name << std::endl;
        return;
    }
    // TODO(Vy): not thread safe here...
    //
    for (int i = 0; i < vol_am_nodes.size(); i++) {
        if (vol_am_nodes[i] == node_name) {
            vol_am_nodes.erase(vol_am_nodes.begin() + i);
            am_agent->om_send_vol_cmd(this, fpi::FDSP_MSG_DETACH_VOL_CTRL);
            return;
        }
    }
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
        << "Detach vol " << vol_name
        << " didn't attached to " << node_name << std::endl;
}

// --------------------------------------------------------------------------------------
// Volume Container
// --------------------------------------------------------------------------------------
VolumeContainer::~VolumeContainer() {}
VolumeContainer::VolumeContainer() : RsContainer() {}

// om_create_vol
// -------------
//
int
VolumeContainer::om_create_vol(const FdspCrtVolPtr &creat_msg)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = (creat_msg->vol_info).vol_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol != NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
            << "Received CreateVol for existing volume " << vname << std::endl;
        return -1;
    }
    vol = VolumeInfo::vol_cast_ptr(rs_alloc_new(uuid));
    vol->vol_mk_description(creat_msg->vol_info);

    Error err = v_pol->fillVolumeDescPolicy(vol->vol_get_properties());
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // TODO(xxx): policy not in the catalog.  Should we return error or use the
        // default policy?
        //
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
            << "Create volume " << vname
            << " with unknown policy " << creat_msg->vol_info.volPolicyId << std::endl;
    } else if (!err.ok()) {
        // TODO(xxx): for now ignoring error.
        //
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
            << "Create volume " << vname
            << ": failed to get policy info" << std::endl;
        return -1;
    }
    err = admin->volAdminControl(vol->vol_get_properties());
    if (!err.ok()) {
        // TODO(Vy): delete the volume here.
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
            << "Unable to create volume " << vname
            << " error: " << err.GetErrstr() << std::endl;
        return -1;
    }
    rs_register(vol);
    local->om_bcast_vol_create(vol);
    return 0;
}

// om_delete_vol
// -------------
//
int
VolumeContainer::om_delete_vol(const FdspDelVolPtr &del_msg)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = del_msg->vol_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
            << "Received DeleteVol for non-existing volume " << vname << std::endl;
        return -1;
    }
    // TODO(Vy): abstraction in vol class for this
    // for (int i = 0; i < del_vol->hv_nodes.size(); i++)
    //     if (currentDom->domain_ptr->currentShMap.count(del_vol->hv_nodes[i]) == 0) {
    //         FDS_PLOG_SEV(om_log, fds_log::error)
    //             << "Inconsistent State Detected. "
    //             << "HV node in volume's hvnode list but not in SH map";
    //         assert(0);
    //     }

    // Update admission control to reflect the deleted volume.
    //
    admin->updateAdminControlParams(vol->vol_get_properties());
    local->om_bcast_vol_delete(vol);

    // TODO(Vy): delete the volume out of the container.
    rs_unregister(vol);
    return 0;
}

// om_modify_vol
// -------------
//
int
VolumeContainer::om_modify_vol(const FdspModVolPtr &mod_msg)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = (mod_msg->vol_desc).vol_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
            << "Received ModifyVol for non-existing volume " << vname << std::endl;
        return -1;
    }
    Error      err(ERR_OK);
    VolumeDesc new_desc(*(vol->vol_get_properties()));

    // We will not modify capacity for now; just policy id or min/max and priority.
    //
    if (mod_msg->vol_desc.volPolicyId != 0) {
        // Change policy id and its description from the catalog.
        //
        new_desc.volPolicyId = mod_msg->vol_desc.volPolicyId;
        err = v_pol->fillVolumeDescPolicy(&new_desc);
        if (!err.ok()) {
            const char *msg = (err == ERR_CAT_ENTRY_NOT_FOUND) ?
                " - requested unknown policy id " :
                " - failed to get policy info id ";

            // Policy not in the catalog, revert to old policy id and return.
            FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "Modify volume " << vname << msg
                << mod_msg->vol_desc.volPolicyId
                << " keeping original policy "
                << vol->vol_get_properties()->volPolicyId << std::endl;
            return -1;
        }
    } else {
        // Don't modify policy id, just min/max ips and priority.
        //
        new_desc.iops_min     = mod_msg->vol_desc.iops_min;
        new_desc.iops_max     = mod_msg->vol_desc.iops_max;
        new_desc.relativePrio = mod_msg->vol_desc.rel_prio;
        FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "Modify volume " << vname
            << " - keeps policy id " << vol->vol_get_properties()->volPolicyId
            << " with new min iops " << new_desc.iops_min
            << " max iops " << new_desc.iops_max
            << " priority " << new_desc.relativePrio << std::endl;
    }
    // Check if this volume can go through admission control with modified policy info
    //
    err = admin->checkVolModify(vol->vol_get_properties(), &new_desc);
    if (!err.ok()) {
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
            << "Modify volume " << vname
            << " -- cannot admit new policy info, keeping the old policy" << std::endl;
        return -1;
    }
    // We admitted modified policy.
    vol->vol_apply_description(new_desc);
    local->om_bcast_vol_modify(vol);
    return 0;
}

// om_attach_vol
// -------------
//
int
VolumeContainer::om_attach_vol(const FDSP_MsgHdrTypePtr &hdr,
                               const FdspAttVolCmdPtr   &attach)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    std::string         &vname = attach->vol_name;
    std::string         &nname = hdr->src_node_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
        << "Received attach volume " << vname << " from " << nname << std::endl;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
            << "Received AttachVol for non-existing volume " << vname << std::endl;
        return -1;
    }
    vol->vol_attach_node(nname);
    return 0;
}

// om_detach_vol
// -------------
//
int
VolumeContainer::om_detach_vol(const FDSP_MsgHdrTypePtr &hdr,
                               const FdspAttVolCmdPtr   &detach)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    std::string         &vname = detach->vol_name;
    std::string         &nname = hdr->src_node_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
        << "Received detach volume " << vname << " from " << nname << std::endl;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
            << "Received AttachVol for non-existing volume " << vname << std::endl;
        return -1;
    }
    vol->vol_detach_node(nname);
    return 0;
}

}  // namespace fds
