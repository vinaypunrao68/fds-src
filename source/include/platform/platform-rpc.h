/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_PLATFORM_RPC_H_
#define SOURCE_INCLUDE_PLATFORM_PLATFORM_RPC_H_

#include <string>
#include <fds_typedefs.h>
#include <NetSession.h>

namespace fds {

namespace fpi = FDS_ProtocolInterface;

class PlatRpcReqt : public fpi::FDSP_ControlPathReqIf
{
  public:
    PlatRpcReqt();
    virtual ~PlatRpcReqt();

    void NotifyAddVol(const FDSP_MsgHdrType &, const FDSP_NotifyVolType &);
    virtual void NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void NotifyRmVol(const FDSP_MsgHdrType &, const FDSP_NotifyVolType &);
    virtual void NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                             fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void NotifyModVol(const FDSP_MsgHdrType &, const FDSP_NotifyVolType &);
    virtual void NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void AttachVol(const FDSP_MsgHdrType &, const FDSP_AttachVolType &);
    virtual void AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                           fpi::FDSP_AttachVolTypePtr &vol_msg);

    void DetachVol(const FDSP_MsgHdrType &, const FDSP_AttachVolType &);
    virtual void DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                           fpi::FDSP_AttachVolTypePtr &vol_msg);

    void NotifyNodeAdd(const FDSP_MsgHdrType &, const FDSP_Node_Info_Type &);
    void NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyNodeRmv(const FDSP_MsgHdrType &, const FDSP_Node_Info_Type &);
    void NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyDLTUpdate(const FDSP_MsgHdrType &, const FDSP_DLT_Data_Type &);
    void NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                         fpi::FDSP_DLT_Data_TypePtr &dlt_info);

    void NotifyDMTUpdate(const FDSP_MsgHdrType &, const FDSP_DMT_Type &);
    void NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,
                         fpi::FDSP_DMT_TypePtr   &dmt_info);

    void SetThrottleLevel(const FDSP_MsgHdrType &, const FDSP_ThrottleMsgType &);
    void SetThrottleLevel(fpi::FDSP_MsgHdrTypePtr      &msg_hdr,
                          fpi::FDSP_ThrottleMsgTypePtr &throttle_msg);

    void TierPolicy(const FDSP_TierPolicy &tier);
    void TierPolicy(fpi::FDSP_TierPolicyPtr &tier);

    void TierPolicyAudit(const FDSP_TierPolicyAudit &audit);
    void TierPolicyAudit(fpi::FDSP_TierPolicyAuditPtr &audit);

    void NotifyBucketStats(const fpi::FDSP_MsgHdrType          &msg_hdr,
                           const fpi::FDSP_BucketStatsRespType &buck_stats_msg);
    void NotifyBucketStats(fpi::FDSP_MsgHdrTypePtr          &msg_hdr,
                           fpi::FDSP_BucketStatsRespTypePtr &buck_stats_msg);

    void NotifyStartMigration(const fpi::FDSP_MsgHdrType    &msg_hdr,
                              const fpi::FDSP_DLT_Data_Type &dlt_info);
    void NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_DLT_Data_TypePtr &dlt_info);
};

class PlatRpcResp : public FDSP_OMControlPathRespIf
{
  public:
    PlatRpcResp();
    virtual ~PlatRpcResp();

    void CreateBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                          const FDSP_CreateVolType   &crt_buck_rsp);
    void CreateBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                          fpi::FDSP_CreateVolTypePtr &crt_buck_rsp);
    void DeleteBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                          const FDSP_DeleteVolType   &del_buck_rsp);
    void DeleteBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                          fpi::FDSP_DeleteVolTypePtr &del_buck_rsp);
    void ModifyBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                          const FDSP_ModifyVolType   &mod_buck_rsp);
    void ModifyBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                          fpi::FDSP_ModifyVolTypePtr &mod_buck_rsp);
    void AttachBucketResp(const FDSP_MsgHdrType         &fdsp_msg,
                          const FDSP_AttachVolCmdType   &atc_buck_req);
    void AttachBucketResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                          fpi::FDSP_AttachVolCmdTypePtr &atc_buck_req);
    void RegisterNodeResp(const FDSP_MsgHdrType         &fdsp_msg,
                          const FDSP_RegisterNodeType   &reg_node_rsp);
    void RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                          fpi::FDSP_RegisterNodeTypePtr &reg_node_rsp);
    void NotifyQueueFullResp(const FDSP_MsgHdrType           &fdsp_msg,
                             const FDSP_NotifyQueueStateType &queue_state_rsp);
    void NotifyQueueFullResp(fpi::FDSP_MsgHdrTypePtr           &fdsp_msg,
                             fpi::FDSP_NotifyQueueStateTypePtr &queue_state_rsp);
    void NotifyPerfstatsResp(const FDSP_MsgHdrType    &fdsp_msg,
                             const FDSP_PerfstatsType &perf_stats_rsp);
    void NotifyPerfstatsResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                             fpi::FDSP_PerfstatsTypePtr &perf_stats_rsp);
    void TestBucketResp(const FDSP_MsgHdrType       &fdsp_msg,
                        const FDSP_TestBucket       &test_buck_rsp);
    void TestBucketResp(fpi::FDSP_MsgHdrTypePtr     &fdsp_msg,
                        fpi::FDSP_TestBucketPtr     &test_buck_rsp);
    void GetDomainStatsResp(const FDSP_MsgHdrType           &fdsp_msg,
                            const FDSP_GetDomainStatsType   &get_stats_rsp);
    void GetDomainStatsResp(fpi::FDSP_MsgHdrTypePtr         &fdsp_msg,
                            fpi::FDSP_GetDomainStatsTypePtr &get_stats_rsp);
    void MigrationDoneResp(const FDSP_MsgHdrType            &fdsp_msg,
                           const FDSP_MigrationStatusType   &status_resp);
    void MigrationDoneResp(fpi::FDSP_MsgHdrTypePtr          &fdsp_msg,
                           fpi::FDSP_MigrationStatusTypePtr &status_resp);
};

};  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_
