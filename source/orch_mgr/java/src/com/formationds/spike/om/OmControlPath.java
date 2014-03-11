package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

public class OmControlPath implements FDSP_OMControlPathReq.Iface {
    private static Logger LOG = Logger.getLogger(OmControlPath.class);

    private ServiceDirectory serviceDirectory;

    public OmControlPath(ServiceDirectory serviceDirectory) {
        this.serviceDirectory = serviceDirectory;
    }

    @Override
    public void CreateBucket(FDSP_MsgHdrType fdsp_msg, FDSP_CreateVolType crt_buck_req) throws TException {

    }

    @Override
    public void DeleteBucket(FDSP_MsgHdrType fdsp_msg, FDSP_DeleteVolType del_buck_req) throws TException {

    }

    @Override
    public void ModifyBucket(FDSP_MsgHdrType fdsp_msg, FDSP_ModifyVolType mod_buck_req) throws TException {

    }

    @Override
    public void AttachBucket(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolCmdType atc_buck_req) throws TException {

    }

    @Override
    public void RegisterNode(FDSP_MsgHdrType fdsp_msg, FDSP_RegisterNodeType reg_node_req) throws TException {
        LOG.info("Registering node " + reg_node_req);
        serviceDirectory.add(reg_node_req);
    }

    @Override
    public void NotifyQueueFull(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyQueueStateType queue_state_info) throws TException {

    }

    @Override
    public void NotifyPerfstats(FDSP_MsgHdrType fdsp_msg, FDSP_PerfstatsType perf_stats_msg) throws TException {

    }

    @Override
    public void TestBucket(FDSP_MsgHdrType fdsp_msg, FDSP_TestBucket test_buck_msg) throws TException {

    }

    @Override
    public void GetDomainStats(FDSP_MsgHdrType fdsp_msg, FDSP_GetDomainStatsType get_stats_msg) throws TException {

    }

    @Override
    public void NotifyMigrationDone(FDSP_MsgHdrType fdsp_msg, FDSP_MigrationStatusType status_msg) throws TException {

    }
}
