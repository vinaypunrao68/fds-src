package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.fdsp.ClientFactory;
import com.google.common.net.InetAddresses;
import org.apache.thrift.TException;

import java.util.UUID;


public class FakeAm {
    // I am a control server
    // I have an OM session client

    public static void main(String[] args) throws Exception {
        int myControlPort = 6666;
        new ServerFactory().startControlPathServer(new Am(), 6666);
        FDSP_OMControlPathReq.Iface client = new ClientFactory().omControlPathClient("localhost", 8904);
        FDSP_RegisterNodeType nodeMesg = new FDSP_RegisterNodeType();
        nodeMesg.setNode_type(FDSP_MgrIdType.FDSP_STOR_HVISOR);
        nodeMesg.setNode_name("h4xx0r-am");
        nodeMesg.setService_uuid(new FDSP_Uuid(UUID.randomUUID().getMostSignificantBits()));
        int ipInt = InetAddresses.coerceToInteger(InetAddresses.forString("127.0.0.1"));
        nodeMesg.setIp_lo_addr(ipInt);
        nodeMesg.setControl_port(myControlPort);
        client.RegisterNode(new FDSP_MsgHdrType(), nodeMesg);
    }

    public static class Am implements FDSP_ControlPathReq.Iface {
        @Override
        public void NotifyAddVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_req) throws TException {
            System.out.println(not_add_vol_req);
        }

        @Override
        public void NotifyRmVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_req) throws TException {
            System.out.println(not_rm_vol_req);
        }

        @Override
        public void NotifyModVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_mod_vol_req) throws TException {
            System.out.println(not_mod_vol_req);
        }

        @Override
        public void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_req) throws TException {
            System.out.println(atc_vol_req);
        }

        @Override
        public void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_req) throws TException {
            System.out.println(dtc_vol_req);
        }

        @Override
        public void NotifyNodeAdd(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {
            System.out.println(node_info);
        }

        @Override
        public void NotifyNodeActive(FDSP_MsgHdrType fdsp_msg, FDSP_ActivateNodeType act_node_req) throws TException {
            System.out.println(act_node_req);
        }

        @Override
        public void NotifyNodeRmv(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {
            System.out.println(node_info);
        }

        @Override
        public void NotifyDLTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Data_Type dlt_info) throws TException {
            System.out.println(dlt_info);
        }

        @Override
        public void NotifyDMTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info) throws TException {
            System.out.println(dmt_info);
        }

        @Override
        public void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg) throws TException {
            System.out.println(throttle_msg);
        }

        @Override
        public void TierPolicy(FDSP_TierPolicy tier) throws TException {
            System.out.println(tier);
        }

        @Override
        public void TierPolicyAudit(FDSP_TierPolicyAudit audit) throws TException {
            System.out.println(audit);
        }

        @Override
        public void NotifyBucketStats(FDSP_MsgHdrType fdsp_msg, FDSP_BucketStatsRespType buck_stats_msg) throws TException {
            System.out.println(buck_stats_msg);
        }

        @Override
        public void NotifyStartMigration(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Data_Type dlt_info) throws TException {
            System.out.println(dlt_info);
        }
    }
}
