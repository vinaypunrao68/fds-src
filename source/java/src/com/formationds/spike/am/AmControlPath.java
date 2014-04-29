package com.formationds.spike.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.spike.Dlt;
import com.formationds.spike.Mutable;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

public class AmControlPath implements FDSP_ControlPathReq.Iface {
    private static Logger LOG = Logger.getLogger(AmControlPath.class);
    private Mutable<Dlt> dlt;

    public AmControlPath(Mutable<Dlt> dlt) {
        this.dlt = dlt;
    }

    @Override
    public void NotifyAddVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_add_vol_req) throws TException {
        LOG.debug(not_add_vol_req);
    }

    @Override
    public void NotifyRmVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_rm_vol_req) throws TException {
        LOG.debug(not_rm_vol_req);
    }

    @Override
    public void NotifyModVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_mod_vol_req) throws TException {
        LOG.debug(not_mod_vol_req);
    }

    @Override
    public void NotifySnapVol(FDSP_MsgHdrType fdsp_msg, FDSP_NotifyVolType not_snap_vol_req) throws TException {
        LOG.debug(not_snap_vol_req);
    }

    @Override
    public void AttachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType atc_vol_req) throws TException {
        LOG.debug(atc_vol_req);
    }

    @Override
    public void DetachVol(FDSP_MsgHdrType fdsp_msg, FDSP_AttachVolType dtc_vol_req) throws TException {
        LOG.debug(dtc_vol_req);
    }

    @Override
    public void NotifyNodeAdd(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {
        LOG.debug(node_info);
    }

    @Override
    public void NotifyNodeActive(FDSP_MsgHdrType fdsp_msg, FDSP_ActivateNodeType act_node_req) throws TException {
        LOG.debug(act_node_req);
    }

    @Override
    public void NotifyNodeRmv(FDSP_MsgHdrType fdsp_msg, FDSP_Node_Info_Type node_info) throws TException {
        LOG.debug(node_info);
    }

    @Override
    public void NotifyDLTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Data_Type dlt_info) throws TException {
        LOG.debug(dlt_info);
        this.dlt.set(new Dlt(dlt_info.getDlt_data()));
    }

    @Override
    public void NotifyDLTClose(FDSP_MsgHdrType fdsp_msg, FDSP_DltCloseType dlt_close) throws TException {
        LOG.debug(dlt_close);
    }

    @Override
    public void NotifyDMTUpdate(FDSP_MsgHdrType fdsp_msg, FDSP_DMT_Type dmt_info) throws TException {
        LOG.debug(dmt_info);
    }

    @Override
    public void SetThrottleLevel(FDSP_MsgHdrType fdsp_msg, FDSP_ThrottleMsgType throttle_msg) throws TException {
        LOG.debug(throttle_msg);
    }

    @Override
    public void TierPolicy(FDSP_TierPolicy tier) throws TException {
        LOG.debug(tier);
    }

    @Override
    public void TierPolicyAudit(FDSP_TierPolicyAudit audit) throws TException {
        LOG.debug(audit);
    }

    @Override
    public void NotifyBucketStats(FDSP_MsgHdrType fdsp_msg, FDSP_BucketStatsRespType buck_stats_msg) throws TException {
        LOG.debug(buck_stats_msg);
    }

    @Override
    public void NotifyStartMigration(FDSP_MsgHdrType fdsp_msg, FDSP_DLT_Data_Type dlt_info) throws TException {
        LOG.debug(dlt_info);
    }

    @Override
    public void NotifyScavengerStart(FDSP_MsgHdrType fdsp_msg, FDSP_ScavengerStartType gc_info) throws TException {
        LOG.debug(gc_info);
    }
}
