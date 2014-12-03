/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/platform-lib.h>

namespace fds
{

#if 0
    // --------------------------------------------------------------------------------------
    // RPC request handlers
    // --------------------------------------------------------------------------------------
    PlatRpcReqt::PlatRpcReqt(const Platform *mgr) : plf_mgr(mgr)
    {
    }
    PlatRpcReqt::~PlatRpcReqt()
    {
    }

    void
    PlatRpcReqt::NotifyAddVol(const FDSP_MsgHdrType    &fdsp_msg,
                              const FDSP_NotifyVolType &not_add_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &msg)
    {
        if (plf_mgr->plf_vol_evt != NULL)
        {
            plf_mgr->plf_vol_evt->plat_add_vol(msg_hdr, msg);
        }
    }

    void
    PlatRpcReqt::NotifyRmVol(const FDSP_MsgHdrType    &fdsp_msg,
                             const FDSP_NotifyVolType &not_rm_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                             fpi::FDSP_NotifyVolTypePtr &msg)
    {
        if (plf_mgr->plf_vol_evt != NULL)
        {
            plf_mgr->plf_vol_evt->plat_rm_vol(msg_hdr, msg);
        }
    }

    void
    PlatRpcReqt::NotifyModVol(const FDSP_MsgHdrType    &fdsp_msg,
                              const FDSP_NotifyVolType &not_mod_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifySnapVol(const FDSP_MsgHdrType    &fdsp_msg,
                               const FDSP_NotifyVolType &not_snap_vol_req)
    {
    }

    void
    PlatRpcReqt::NotifySnapVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                               fpi::FDSP_NotifyVolTypePtr &msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::AttachVol(const FDSP_MsgHdrType    &fdsp_msg,
                           const FDSP_AttachVolType &atc_vol_req)
    {
    }

    void
    PlatRpcReqt::AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                           fpi::FDSP_AttachVolTypePtr &vol_msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::DetachVol(const FDSP_MsgHdrType    &fdsp_msg,
                           const FDSP_AttachVolType &dtc_vol_req)
    {
    }

    void
    PlatRpcReqt::DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                           fpi::FDSP_AttachVolTypePtr &vol_msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyNodeAdd(const FDSP_MsgHdrType     &fdsp_msg,
                               const FDSP_Node_Info_Type &node_info)
    {
    }

    void
    PlatRpcReqt::NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                               fpi::FDSP_Node_Info_TypePtr &node_info)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyNodeActive(const FDSP_MsgHdrType     &fdsp_msg,
                                  const FDSP_ActivateNodeType &act_node_req)
    {
    }

    void
    PlatRpcReqt::NotifyNodeActive(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                                  fpi::FDSP_ActivateNodeTypePtr &act_node_req)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyNodeRmv(const FDSP_MsgHdrType     &fdsp_msg,
                               const FDSP_Node_Info_Type &node_info)
    {
    }

    void
    PlatRpcReqt::NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                               fpi::FDSP_Node_Info_TypePtr &node_info)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyDLTUpdate(const FDSP_MsgHdrType    &fdsp_msg,
                                 const FDSP_DLT_Data_Type &dlt_info)
    {
    }

    void
    PlatRpcReqt::NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                                 fpi::FDSP_DLT_Data_TypePtr &dlt_info)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyDLTClose(const FDSP_MsgHdrType    &fdsp_msg,
                                const FDSP_DltCloseType &dlt_info)
    {
    }

    void
    PlatRpcReqt::NotifyDLTClose(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                                fpi::FDSP_DltCloseTypePtr &dlt_info)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyDMTUpdate(const FDSP_MsgHdrType &msg_hdr,
                                 const FDSP_DMT_Type   &dmt_info)
    {
    }

    void
    PlatRpcReqt::NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,  // NOLINT
                                 fpi::FDSP_DMT_TypePtr   &dmt)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyDMTClose(const FDSP_MsgHdrType &msg_hdr,
                                const FDSP_DmtCloseType &dmt)
    {
    }
    void
    PlatRpcReqt::NotifyDMTClose(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                                fpi::FDSP_DmtCloseTypePtr &dmt_close)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::PushMetaDMTReq(const FDSP_MsgHdrType &msg_hdr,
                                const FDSP_PushMeta &push_meta_resp)
    {
    }
    void
    PlatRpcReqt::PushMetaDMTReq(fpi::FDSP_MsgHdrTypePtr &msg_hdr,
                                fpi::FDSP_PushMetaPtr &push_meta_resp)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::SetThrottleLevel(const FDSP_MsgHdrType      &msg_hdr,
                                  const FDSP_ThrottleMsgType &throttle_msg)
    {
    }

    void
    PlatRpcReqt::SetThrottleLevel(fpi::FDSP_MsgHdrTypePtr      &msg_hdr,
                                  fpi::FDSP_ThrottleMsgTypePtr &throttle_msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::SetQoSControl(const FDSP_MsgHdrType& fdsp_msg,
                               const FDSP_QoSControlMsgType& qos_msg)
    {
    }

    void
    PlatRpcReqt::SetQoSControl(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                               fpi::FDSP_QoSControlMsgTypePtr& qos_msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::TierPolicy(const FDSP_TierPolicy &tier)
    {
    }

    void
    PlatRpcReqt::TierPolicy(fpi::FDSP_TierPolicyPtr &tier)  // NOLINT
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::TierPolicyAudit(const FDSP_TierPolicyAudit &audit)
    {
    }

    void
    PlatRpcReqt::TierPolicyAudit(fpi::FDSP_TierPolicyAuditPtr &audit)  // NOLINT
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyBucketStats(const fpi::FDSP_MsgHdrType          &msg_hdr,
                                   const fpi::FDSP_BucketStatsRespType &buck_stats_msg)
    {
    }

    void
    PlatRpcReqt::NotifyBucketStats(fpi::FDSP_MsgHdrTypePtr          &hdr,
                                   fpi::FDSP_BucketStatsRespTypePtr &msg)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyStartMigration(const fpi::FDSP_MsgHdrType    &msg_hdr,
                                      const fpi::FDSP_DLT_Data_Type &dlt_info)
    {
    }

    void
    PlatRpcReqt::NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &hdr,
                                      fpi::FDSP_DLT_Data_TypePtr &dlt)
    {
        fds_verify(0);
    }

    void
    PlatRpcReqt::NotifyScavengerCmd(const fpi::FDSP_MsgHdrType    &msg_hdr,
                                    const fpi::FDSP_ScavengerType &gc_info)
    {
    }

    void
    PlatRpcReqt::NotifyScavengerCmd(fpi::FDSP_MsgHdrTypePtr    &hdr,
                                    fpi::FDSP_ScavengerTypePtr &gc_info)
    {
        fds_verify(0);
    }

    // --------------------------------------------------------------------------------------
    // RPC response handlers
    // --------------------------------------------------------------------------------------
    PlatRpcResp::PlatRpcResp(const Platform *mgr) : plf_mgr(mgr)
    {
    }
    PlatRpcResp::~PlatRpcResp()
    {
    }

    void
    PlatRpcResp::CreateBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                                  const FDSP_CreateVolType   &crt_buck_rsp)
    {
    }

    void
    PlatRpcResp::CreateBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                                  fpi::FDSP_CreateVolTypePtr &crt_buck_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::DeleteBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                                  const FDSP_DeleteVolType   &del_buck_rsp)
    {
    }

    void
    PlatRpcResp::DeleteBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                                  fpi::FDSP_DeleteVolTypePtr &del_buck_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::ModifyBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                                  const FDSP_ModifyVolType   &mod_buck_rsp)
    {
    }

    void
    PlatRpcResp::ModifyBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                                  fpi::FDSP_ModifyVolTypePtr &mod_buck_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::AttachBucketResp(const FDSP_MsgHdrType         &fdsp_msg,
                                  const FDSP_AttachVolCmdType   &atc_buck_req)
    {
    }

    void
    PlatRpcResp::AttachBucketResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                                  fpi::FDSP_AttachVolCmdTypePtr &atc_buck_req)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::RegisterNodeResp(const FDSP_MsgHdrType         &fdsp_msg,
                                  const FDSP_RegisterNodeType   &reg_node_rsp)
    {
    }

    void
    PlatRpcResp::RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                                  fpi::FDSP_RegisterNodeTypePtr &reg_node_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::NotifyQueueFullResp(const FDSP_MsgHdrType           &fdsp_msg,
                                     const FDSP_NotifyQueueStateType &queue_state_rsp)
    {
    }

    void
    PlatRpcResp::NotifyQueueFullResp(fpi::FDSP_MsgHdrTypePtr           &fdsp_msg,
                                     fpi::FDSP_NotifyQueueStateTypePtr &queue_state_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::NotifyPerfstatsResp(const FDSP_MsgHdrType    &fdsp_msg,
                                     const FDSP_PerfstatsType &perf_stats_rsp)
    {
    }

    void
    PlatRpcResp::NotifyPerfstatsResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                                     fpi::FDSP_PerfstatsTypePtr &perf_stats_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::TestBucketResp(const FDSP_MsgHdrType    &fdsp_msg,
                                const FDSP_TestBucket    &test_buck_rsp)
    {
    }

    void
    PlatRpcResp::TestBucketResp(fpi::FDSP_MsgHdrTypePtr  &fdsp_msg,
                                fpi::FDSP_TestBucketPtr  &test_buck_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::GetDomainStatsResp(const FDSP_MsgHdrType           &fdsp_msg,
                                    const FDSP_GetDomainStatsType   &get_stats_rsp)
    {
    }

    void
    PlatRpcResp::GetDomainStatsResp(fpi::FDSP_MsgHdrTypePtr         &fdsp_msg,
                                    fpi::FDSP_GetDomainStatsTypePtr &get_stats_rsp)
    {
        fds_verify(0);
    }

    void
    PlatRpcResp::MigrationDoneResp(const FDSP_MsgHdrType            &fdsp_msg,
                                   const FDSP_MigrationStatusType   &status_resp)
    {
    }

    void
    PlatRpcResp::MigrationDoneResp(fpi::FDSP_MsgHdrTypePtr          &fdsp_msg,
                                   fpi::FDSP_MigrationStatusTypePtr &status_resp)
    {
        fds_verify(0);
    }

    // --------------------------------------------------------------------------------------
    // RPC Data Path Response
    // --------------------------------------------------------------------------------------
    PlatDataPathResp::PlatDataPathResp(Platform *mgr) : plf_mgr(mgr)
    {
    }
    PlatDataPathResp::~PlatDataPathResp()
    {
    }

    void
    PlatDataPathResp::GetObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                    const FDSP_GetObjType& get_obj_req)
    {
    }

    void
    PlatDataPathResp::GetObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_GetObjType>& get_obj_req)
    {
    }

    void
    PlatDataPathResp::PutObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                    const FDSP_PutObjType& put_obj_req)
    {
    }

    void
    PlatDataPathResp::PutObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  /// NOLINT
        boost::shared_ptr<FDSP_PutObjType>& put_obj_req)
    {
    }

    void
    PlatDataPathResp::DeleteObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                       const FDSP_DeleteObjType& del_obj_req)
    {
    }

    void
    PlatDataPathResp::DeleteObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_DeleteObjType>& del_obj_req)
    {
    }

    void
    PlatDataPathResp::OffsetWriteObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                            const FDSP_OffsetWriteObjType& req)
    {
    }

    void
    PlatDataPathResp::OffsetWriteObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_OffsetWriteObjType>& req)
    {
    }

    void
    PlatDataPathResp::RedirReadObjectResp(const FDSP_MsgHdrType& fdsp_msg,
                                          const FDSP_RedirReadObjType& req)
    {
    }

    void
    PlatDataPathResp::RedirReadObjectResp(
        boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg,  // NOLINT
        boost::shared_ptr<FDSP_RedirReadObjType>& req)
    {
    }

    void
    PlatDataPathResp::GetObjectMetadataResp(
        const FDSP_GetObjMetadataResp& metadata_resp)
    {
    }

    void
    PlatDataPathResp::GetObjectMetadataResp(
        boost::shared_ptr<FDSP_GetObjMetadataResp>& metadata_resp)  // NOLINT
    {
    }

#endif
}  // namespace fds
