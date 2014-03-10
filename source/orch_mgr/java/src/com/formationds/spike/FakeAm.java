package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TThreadPoolServer;
import org.apache.thrift.transport.TServerSocket;
import org.apache.thrift.transport.TServerTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransportException;

public class FakeAm {
    // I am a control server
    // I have an OM session client

    public static void main(String[] args) throws Exception {
        new Thread(() -> {
            try {
                TServerTransport channel = new TServerSocket(6666);
                FDSP_ControlPathReq.Processor<Am> processor = new FDSP_ControlPathReq.Processor<>(new Am());
                TThreadPoolServer.Args serverArgs = new TThreadPoolServer.Args(channel).processor(processor);
                TServer server = new TThreadPoolServer(serverArgs);
                server.serve();
            } catch (TTransportException e) {
                e.printStackTrace();
            }
        }).start();
        Thread.sleep(1000);
        TSocket socket = new TSocket("localhost", 8904);
        socket.open();
        TProtocol protocol = new TBinaryProtocol(socket);
        FDSP_Service.Client client = new FDSP_Service.Client(protocol);
        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
        msg.setSrc_node_name("h4xx0r-sm");
        msg.setMsg_code(FDSP_MsgCodeType.FDSP_MSG_REG_NODE);
        msg.setSrc_id(FDSP_MgrIdType.FDSP_STOR_HVISOR);
        msg.setSrc_port(6666);
        FDSP_SessionReqResp response = client.EstablishSession(msg);
        System.out.println(response);
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
        public void NotifyNodeActive(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {
            System.out.println(node_info);
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
