package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.fdsp.ClientFactory;
import com.formationds.spike.ServiceDirectory;
import org.apache.thrift.TException;

import java.util.List;

public class OmConfigPath implements FDSP_ConfigPathReq.Iface {
    private ServiceDirectory serviceDirectory;
    private ClientFactory controlClientFactory;

    public OmConfigPath(ServiceDirectory serviceDirectory, ClientFactory clientFactory) {
        this.serviceDirectory = serviceDirectory;
        this.controlClientFactory = clientFactory;
    }

    @Override
    public int CreateVol(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_vol_req) throws TException {
        return 0;
    }

    @Override
    public int DeleteVol(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_vol_req) throws TException {
        return 0;
    }

    @Override
    public int ModifyVol(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_vol_req) throws TException {
        return 0;
    }

    @Override
    public int CreatePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_CreatePolicyType crt_pol_req) throws TException {
        return 0;
    }

    @Override
    public int DeletePolicy(FDSP_MsgHdrType fdsp_msg, FDSP_DeletePolicyType del_pol_req) throws TException {
        return 0;
    }

    @Override
    public int ModifyPolicy(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyPolicyType mod_pol_req) throws TException {
        return 0;
    }

    @Override
    public int AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_vol_req) throws TException {
        return 0;
    }

    @Override
    public int DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType dtc_vol_req) throws TException {
        return 0;
    }

    @Override
    public int AssociateRespCallback(long ident) throws TException {
        return 0;
    }

    @Override
    public int CreateDomain(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType crt_dom_req) throws TException {
        return 0;
    }

    @Override
    public int DeleteDomain(FDSP_MsgHdrType fdsp_msg, FDSP_CreateDomainType del_dom_req) throws TException {
        return 0;
    }

    @Override
    public int SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg) throws TException {
        return 0;
    }

    @Override
    public FDSP_VolumeDescType GetVolInfo(FDSP_MsgHdrType fdsp_msg, FDSP_GetVolInfoReqType vol_info_req) throws TException {
        return null;
    }

    @Override
    public int GetDomainStats(FDSP_MsgHdrType fdsp_msg, FDSP_GetDomainStatsType get_stats_msg) throws TException {
        return 0;
    }

    @Override
    public int applyTierPolicy(tier_pol_time_unit policy) throws TException {
        return 0;
    }

    @Override
    public int auditTierPolicy(tier_pol_audit audit) throws TException {
        return 0;
    }

    @Override
    public int RemoveServices(FDSP_MsgHdrType fdsp_msg, FDSP_RemoveServicesType rm_node_req) throws TException {
        return 0;
    }

    @Override
    public int ActivateAllNodes(FDSP_MsgHdrType fdsp_msg, FDSP_ActivateAllNodesType act_node_req) throws TException {
        return (int) serviceDirectory
                .allNodes()
                .filter(n -> n.getNode_type().equals(FDSP_MgrIdType.FDSP_PLATFORM))
                .filter(n -> {
                    try {
                        FDSP_ActivateNodeType activateMsg = new FDSP_ActivateNodeType(
                                n.service_uuid,
                                n.getNode_name(),
                                true,
                                true,
                                false,
                                true);

                        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
                        msg.setMsg_code(FDSP_MsgCodeType.FDSP_MSG_NOTIFY_NODE_ACTIVE);
                        controlClientFactory.controlPathClient(n).NotifyNodeActive(msg, activateMsg);
                        return true;
                    } catch (TException e) {
                        // This service is dead
                        return false;
                    }
                })
                .count();
    }

    @Override
    public int ActivateNode(FDSP_MsgHdrType fdsp_msg, FDSP_ActivateOneNodeType req) throws TException {
        return 0;
    }

    @Override
    public int ScavengerCommand(FDSP_MsgHdrType fdsp_msg, FDSP_ScavengerType req) throws TException {
        return 0;
    }

    @Override
    public List<FDSP_Node_Info_Type> ListServices(FDSP_MsgHdrType fdsp_msg) throws TException {
        return null;
    }

    @Override
    public List<FDSP_VolumeDescType> ListVolumes(FDSP_MsgHdrType fdsp_msg) throws TException {
        return null;
    }

}
