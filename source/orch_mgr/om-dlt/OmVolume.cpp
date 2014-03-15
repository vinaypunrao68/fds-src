/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <OmVolume.h>
#include <OmResources.h>
#include <orchMgr.h>
#include <om-discovery.h>

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
    setName(info.vol_name);
    vol_name.assign(rs_name);
}

// setDescription
// ---------------------
//
void
VolumeInfo::setDescription(const VolumeDesc &desc)
{
    if (vol_properties == NULL) {
        vol_properties = new VolumeDesc(desc);
    } else {
        (*vol_properties) = desc;
    }
    setName(desc.name);
    vol_name = desc.name;
    volUUID = desc.volUUID;
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

// vol_fmt_message
// ---------------
//
void
VolumeInfo::vol_fmt_message(om_vol_msg_t *out)
{
    switch (out->vol_msg_code) {
        case fpi::FDSP_MSG_GET_BUCKET_STATS_RSP: {
            VolumeDesc          *desc = vol_properties;
            FDSP_BucketStatType *stat = out->u.vol_stats;

            fds_verify(stat != NULL);
            stat->vol_name = vol_name;
            stat->sla      = desc->iops_min;
            stat->limit    = desc->iops_max;
            stat->rel_prio = desc->relativePrio;
            break;
        }
        case fpi::FDSP_MSG_DELETE_VOL:
        case fpi::FDSP_MSG_MODIFY_VOL:
        case fpi::FDSP_MSG_CREATE_VOL: {
            FdspNotVolPtr notif = *out->u.vol_notif;

            vol_fmt_desc_pkt(&notif->vol_desc);
            notif->vol_name  = vol_get_name();
            if (out->vol_msg_code == fpi::FDSP_MSG_CREATE_VOL) {
                notif->type = fpi::FDSP_NOTIFY_ADD_VOL;
            } else if (out->vol_msg_code == fpi::FDSP_MSG_MODIFY_VOL) {
                notif->type = fpi::FDSP_NOTIFY_MOD_VOL;
            } else {
                notif->type = fpi::FDSP_NOTIFY_RM_VOL;
            }
            break;
        }
        case fpi::FDSP_MSG_ATTACH_VOL_CTRL:
        case fpi::FDSP_MSG_DETACH_VOL_CTRL: {
            FdspAttVolPtr attach = *out->u.vol_attach;

            vol_fmt_desc_pkt(&attach->vol_desc);
            attach->vol_name = vol_get_name();
            break;
        }
        default: {
            fds_panic("Unknown volume request code");
            break;
        }
    }
}

// vol_send_message
// ----------------
//
void
VolumeInfo::vol_send_message(om_vol_msg_t *out, NodeAgent::pointer dest)
{
    fpi::FDSP_MsgHdrTypePtr  m_hdr;
    NodeAgentCpReqClientPtr  clnt;

    if (out->vol_msg_hdr == NULL) {
        m_hdr = fpi::FDSP_MsgHdrTypePtr(new fpi::FDSP_MsgHdrType);
        dest->init_msg_hdr(m_hdr);
        m_hdr->msg_code       = out->vol_msg_code;
        m_hdr->glob_volume_id = vol_properties->volUUID;
    } else {
        m_hdr = *(out->vol_msg_hdr);
    }
    clnt = OM_SmAgent::agt_cast_ptr(dest)->getCpClient(&m_hdr->session_uuid);
    switch (out->vol_msg_code) {
        case fpi::FDSP_MSG_DELETE_VOL:
            clnt->NotifyRmVol(m_hdr, *out->u.vol_notif);
            break;

        case fpi::FDSP_MSG_MODIFY_VOL:
            clnt->NotifyModVol(m_hdr, *out->u.vol_notif);
            break;

        case fpi::FDSP_MSG_CREATE_VOL:
            clnt->NotifyAddVol(m_hdr, *out->u.vol_notif);
            break;

        case fpi::FDSP_MSG_ATTACH_VOL_CTRL:
            clnt->AttachVol(m_hdr, *out->u.vol_attach);
            break;

        case fpi::FDSP_MSG_DETACH_VOL_CTRL:
            clnt->DetachVol(m_hdr, *out->u.vol_attach);
            break;

        default:
            fds_panic("Unknown volume request code");
            break;
    }
}

// vol_am_agent
// ------------
//
NodeAgent::pointer
VolumeInfo::vol_am_agent(const NodeUuid &am_uuid)
{
    OM_NodeContainer  *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    return local->om_am_agent(am_uuid);
}

// vol_attach_node
// ---------------
//
void
VolumeInfo::vol_attach_node(const NodeUuid &node_uuid)
{
    OM_AmAgent::pointer  am_agent;

    am_agent = OM_AmAgent::agt_cast_ptr(vol_am_agent(node_uuid));
    if (am_agent == NULL) {
        // Provisioning vol before the AM is online.
        //
        LOGNORMAL << "Received attach vol " << vol_name
                  << ", am node uuid " << std::hex << node_uuid << std::dec;
        return;
    }
    // TODO(Vy): not thread safe here...
    //
    for (uint i = 0; i < vol_am_nodes.size(); i++) {
        if (vol_am_nodes[i] == node_uuid) {
            LOGNORMAL << "Volume " << vol_name << " is already attached to node "
                      << std::hex << node_uuid << std::dec;
            return;
        }
    }
    vol_am_nodes.push_back(node_uuid);
    am_agent->om_send_vol_cmd(this, fpi::FDSP_MSG_ATTACH_VOL_CTRL);
}

// vol_detach_node
// ---------------
//
void
VolumeInfo::vol_detach_node(const NodeUuid &node_uuid)
{
    OM_AmAgent::pointer  am_agent;

    am_agent = OM_AmAgent::agt_cast_ptr(vol_am_agent(node_uuid));
    if (am_agent == NULL) {
        // Provisioning vol before the AM is online.
        //
        LOGNORMAL << "Received Detach vol " << vol_name
                  << ", am node " << std::hex << node_uuid << std::dec;
        return;
    }
    // TODO(Vy): not thread safe here...
    //
    for (uint i = 0; i < vol_am_nodes.size(); i++) {
        if (vol_am_nodes[i] == node_uuid) {
            vol_am_nodes.erase(vol_am_nodes.begin() + i);
            am_agent->om_send_vol_cmd(this, fpi::FDSP_MSG_DETACH_VOL_CTRL);
            return;
        }
    }
    LOGNORMAL << "Detach vol " << vol_name
              << " didn't attached to " << std::hex << node_uuid << std::dec;
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
VolumeContainer::om_create_vol(const FdspMsgHdrPtr &hdr,
                               const FdspCrtVolPtr &creat_msg)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = (creat_msg->vol_info).vol_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol != NULL) {
        LOGWARN << "Received CreateVol for existing volume " << vname;
        return -1;
    }
    vol = VolumeInfo::vol_cast_ptr(rs_alloc_new(uuid));
    vol->vol_mk_description(creat_msg->vol_info);

    Error err = v_pol->fillVolumeDescPolicy(vol->vol_get_properties());
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // TODO(xxx): policy not in the catalog.  Should we return error or use the
        // default policy?
        //
        LOGWARN << "Create volume " << vname
                << " with unknown policy "
                << creat_msg->vol_info.volPolicyId;
    } else if (!err.ok()) {
        // TODO(xxx): for now ignoring error.
        //
        LOGERROR << "Create volume " << vname
                 << ": failed to get policy info";
        return -1;
    }

    err = admin->volAdminControl(vol->vol_get_properties());
    if (!err.ok()) {
        // TODO(Vy): delete the volume here.
        LOGERROR << "Unable to create volume " << vname
                 << " error: " << err.GetErrstr();
        rs_free_resource(vol);
        return -1;
    }
    rs_register(vol);

    const VolumeDesc& volumeDesc=*(vol->vol_get_properties());
    // store it in config db..
    if (!gl_orch_mgr->getConfigDB()->addVolume(volumeDesc)) {
        LOGWARN << "unable to store volume info in to config db "
                << "[" << volumeDesc.name << ":" <<volumeDesc.volUUID << "]";
    }

    local->om_bcast_vol_create(vol);

    // Attach the volume to the requester's AM.
    //
    vol->vol_attach_node(NodeUuid(hdr->src_service_uuid.uuid));
    return 0;
}

// om_delete_vol
// -------------
//
int
VolumeContainer::om_delete_vol(const FdspMsgHdrPtr &hdr,
                               const FdspDelVolPtr &del_msg)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = del_msg->vol_name;
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        LOGWARN << "Received DeleteVol for non-existing volume " << vname;
        return -1;
    }
    // TODO(Vy): abstraction in vol class for this
    // for (int i = 0; i < del_vol->hv_nodes.size(); i++)
    //     if (currentDom->domain_ptr->currentShMap.count(del_vol->hv_nodes[i]) == 0) {
    //         LOGERROR //             << "Inconsistent State Detected. "
    //             << "HV node in volume's hvnode list but not in SH map";
    //         assert(0);
    //     }

    // Update admission control to reflect the deleted volume.
    //
    admin->updateAdminControlParams(vol->vol_get_properties());
    local->om_bcast_vol_delete(vol);

    vol->vol_detach_node(NodeUuid(hdr->src_service_uuid.uuid));
    rs_unregister(vol);
    rs_free_resource(vol);
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
        LOGWARN << "Received ModifyVol for non-existing volume " << vname;
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
            LOGERROR << "Modify volume " << vname << msg
                     << mod_msg->vol_desc.volPolicyId
                     << " keeping original policy "
                     << vol->vol_get_properties()->volPolicyId;
            return -1;
        }
    } else {
        // Don't modify policy id, just min/max ips and priority.
        //
        new_desc.iops_min     = mod_msg->vol_desc.iops_min;
        new_desc.iops_max     = mod_msg->vol_desc.iops_max;
        new_desc.relativePrio = mod_msg->vol_desc.rel_prio;
        LOGNOTIFY << "Modify volume " << vname
                  << " - keeps policy id " << vol->vol_get_properties()->volPolicyId
                  << " with new min iops " << new_desc.iops_min
                  << " max iops " << new_desc.iops_max
                  << " priority " << new_desc.relativePrio;
    }
    // Check if this volume can go through admission control with modified policy info
    //
    err = admin->checkVolModify(vol->vol_get_properties(), &new_desc);
    if (!err.ok()) {
        LOGERROR << "Modify volume " << vname
                 << " -- cannot admit new policy info, keeping the old policy";
        return -1;
    }
    // We admitted modified policy.
    vol->setDescription(new_desc);
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
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    LOGNOTIFY << "Received attach volume " << vname << " from "
              << hdr->src_node_name  << "node uuid: "
              << hdr->src_service_uuid.uuid;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        LOGWARN << "Received AttachVol for non-existing volume " << vname;
        return -1;
    }
    if (hdr->src_service_uuid.uuid == 0) {
        /* Don't have uuid, only have the name. */
        OM_AmContainer::pointer am_nodes = local->om_am_nodes();
        OM_AmAgent::pointer am =
            OM_AmAgent::agt_cast_ptr(am_nodes->
                 rs_get_resource(hdr->src_node_name.c_str()));

        if (am != NULL) {
           vol->vol_attach_node(am->rs_get_uuid());
           LOGNOTIFY << "uuid for  the node:" << hdr->src_node_name
            << ": " << am->rs_get_uuid();
        }
    } else {
        vol->vol_attach_node(NodeUuid(hdr->src_service_uuid.uuid));
    }
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
    ResourceUUID         uuid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;

    LOGNOTIFY << "Received detach volume " << vname << " from "
              << hdr->src_node_name;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        LOGWARN << "Received detach Vol for non-existing volume " << vname;
        return -1;
    }

    if (hdr->src_service_uuid.uuid == 0) {
        /* Don't have uuid, only have the name. */
        OM_AmContainer::pointer am_nodes = local->om_am_nodes();
        OM_AmAgent::pointer am =
            OM_AmAgent::agt_cast_ptr(am_nodes->
                 rs_get_resource(hdr->src_node_name.c_str()));

        if (am != NULL) {
           vol->vol_detach_node(am->rs_get_uuid());
           LOGNOTIFY << "uuid for  the node:" << hdr->src_node_name
            << ": " << am->rs_get_uuid();
        }
    } else {
        vol->vol_detach_node(NodeUuid(hdr->src_service_uuid.uuid));
    }
    return 0;
}

// om_test_bucket
// --------------
//
void
VolumeContainer::om_test_bucket(const FdspMsgHdrPtr     &hdr,
                                const FdspTestBucketPtr &req)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    std::string         &vname = req->bucket_name;
    NodeUuid             n_uid(hdr->src_service_uuid.uuid);
    ResourceUUID         v_uid(fds_get_uuid64(vname));
    VolumeInfo::pointer  vol;
    OM_AmAgent::pointer  am;

    LOGNOTIFY << "Received test bucket request " << vname
              << "attach_vol_reqd " << req->attach_vol_reqd;

    am = local->om_am_agent(n_uid);
    if (am == NULL) {
        LOGNOTIFY << "OM does not know about node " << hdr->src_node_name;
    }
    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(v_uid));
    if (vol == NULL) {
        LOGNOTIFY << "Bucket " << vname << " does not exists, notify node "
                  << hdr->src_node_name;

        if (am != NULL) {
            am->om_send_vol_cmd(NULL, &vname, fpi::FDSP_MSG_ATTACH_VOL_CTRL);
        }
    } else if (req->attach_vol_reqd == false) {
        // Didn't request OM to attach this volume.
        LOGNOTIFY << "Bucket " << vname << " exists, notify node "
                  << hdr->src_node_name;

        if (am != NULL) {
            am->om_send_vol_cmd(vol, fpi::FDSP_MSG_ATTACH_VOL_CTRL);
        }
    } else {
        vol->vol_attach_node(n_uid);
    }
}

bool VolumeContainer::addVolume(const VolumeDesc& volumeDesc) {
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    VolumeInfo::pointer  vol;
    ResourceUUID         uuid = volumeDesc.volUUID;
    Error  err(ERR_OK);

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol != NULL) {
        LOGWARN << "volume already exists : " << volumeDesc.name <<":" << uuid;
        return false;
    }

    vol->setDescription(volumeDesc);

    err = admin->volAdminControl(vol->vol_get_properties());
    if (!err.ok()) {
        // TODO(Vy): delete the volume here.
        LOGERROR << "Unable to create volume " << " error: " << err.GetErrstr();
        rs_free_resource(vol);
        return false;
    }
    rs_register(vol);
    return true;
}

}  // namespace fds
