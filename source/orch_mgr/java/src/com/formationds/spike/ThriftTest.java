package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
import FDS_ProtocolInterface.*;
import org.apache.thrift.TException;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TServer.Args;
import org.apache.thrift.server.TSimpleServer;
import org.apache.thrift.server.TThreadPoolServer;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TServerSocket;
import org.apache.thrift.transport.TServerTransport;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;

import java.util.Map;

public class ThriftTest {
    static class FakeControlPathServer implements FDSP_ControlPathReq.Iface {
        @Override
        public void NotifyAddVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_req) throws TException {

        }

        @Override
        public void NotifyRmVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_req) throws TException {

        }

        @Override
        public void NotifyModVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_mod_vol_req) throws TException {

        }

        @Override
        public void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_req) throws TException {

        }

        @Override
        public void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_req) throws TException {

        }

        @Override
        public void NotifyNodeAdd(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {

        }

        @Override
        public void NotifyNodeRmv(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {

        }

        @Override
        public void NotifyDLTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Data_Type dlt_info) throws TException {

        }

        @Override
        public void NotifyDMTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info) throws TException {

        }

        @Override
        public void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg) throws TException {

        }

        @Override
        public void TierPolicy(FDSP_TierPolicy tier) throws TException {

        }

        @Override
        public void TierPolicyAudit(FDSP_TierPolicyAudit audit) throws TException {

        }

        @Override
        public void NotifyBucketStats(FDSP_MsgHdrType fdsp_msg, FDSP_BucketStatsRespType buck_stats_msg) throws TException {

        }

        @Override
        public void NotifyStartMigration(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Data_Type dlt_info) throws TException {

        }
    }
    public static void main(String[] args) throws Exception {
        TServerTransport channel = new TServerSocket(8904);
        TServer server = new TSimpleServer(new Args(channel).processor(new FDSP_ControlPathReq.Processor<FakeControlPathServer>(new FakeControlPathServer())));
        server.serve();
    }
}
