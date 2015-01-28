/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
#define SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_

#include <cstdio>
#include <string>
#include <vector>

#include <fds_types.h>
#include <fds_error.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <fdsp/FDSP_OMControlPathReq.h>
#include <fdsp/FDSP_ControlPathResp.h>
#include <fds_typedefs.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <omutils.h>
#include <OmTier.h>
#include <OmVolPolicy.hpp>
#include <OmAdminCtrl.h>
#include <kvstore/configdb.h>
#include "platform/platform_process.h"
#include <snapshot/manager.h>
#include <deletescheduler.h>
#include <net/PlatNetSvcHandler.h>

#define MAX_OM_NODES            (512)
#define DEFAULT_LOC_DOMAIN_ID   (1)
#define DEFAULT_GLB_DOMAIN_ID   (1)

namespace fpi = FDS_ProtocolInterface;

namespace FDS_ProtocolInterface {
    class FDSP_OMControlPathReqProcessor;
    class FDSP_OMControlPathReqIf;
    class FDSP_OMControlPathRespClient;
    class FDSP_OMControlPathReqIf;
    class FDSP_ControlPathRespIf;
    class FDSP_ConfigPathReqProcessor;
    class FDSP_ConfigPathReqIf;
    class FDSP_ConfigPathRespClient;
}

class netSessionTbl;
template <class A, class B, class C> class netServerSessionEx;
typedef netServerSessionEx<fpi::FDSP_OMControlPathReqProcessor,
                fpi::FDSP_OMControlPathReqIf,
                fpi::FDSP_OMControlPathRespClient> netOMControlPathServerSession;
typedef netServerSessionEx<fpi::FDSP_ConfigPathReqProcessor,
                fpi::FDSP_ConfigPathReqIf,
                fpi::FDSP_ConfigPathRespClient> netConfigPathServerSession;

namespace fds {

class OrchMgr: public PlatformProcess {
  private:
    fds_log *om_log;
    SysParams *sysParams;
    /* net session tbl for OM control path*/
    boost::shared_ptr<netSessionTbl> omcp_session_tbl;
    netOMControlPathServerSession *omc_server_session;
    boost::shared_ptr<fpi::FDSP_OMControlPathReqIf> omcp_req_handler;
    boost::shared_ptr<fpi::FDSP_ControlPathRespIf> cp_resp_handler;
    std::string my_node_name;

    /* net session tbl for OM config path server */
    boost::shared_ptr<netSessionTbl> cfg_session_tbl;
    netConfigPathServerSession *cfg_server_session;
    boost::shared_ptr<fpi::FDSP_ConfigPathReqIf> cfg_req_handler;
    /* config path server is run on this thread */
    boost::shared_ptr<std::thread> cfgserver_thread;


    int current_dlt_version;
    FDS_ProtocolInterface::Node_Table_Type current_dlt_table;
    fds_mutex *om_mutex;
    std::string node_id_to_name[MAX_OM_NODES];
    const int table_type_dlt = 0;

    /*
     * Cmdline configurables
     */
    int conf_port_num; /* config port to listen for cli commands */
    int ctrl_port_num; /* control port (register node + config cmds from AM) */
    std::string stor_prefix;
    fds_bool_t test_mode;

    /* policy manager */
    VolPolicyMgr           *policy_mgr;
    Thrift_VolPolicyServ   *om_ice_proxy;
    Orch_VolPolicyServ     *om_policy_srv;
    kvstore::ConfigDB      *configDB;
    void SetThrottleLevelForDomain(int domain_id, float throttle_level);

  public:
    OrchMgr(int argc, char *argv[], Platform *platform, Module **mod_vec);
    ~OrchMgr();
    void start_cfgpath_server();

    /**** From FdsProcess ****/
    virtual void proc_pre_startup() override;
    virtual void proc_pre_service() override;
    /*
     * Runs the orch manager server.
     * Does not return until the server is no longer running
     */
    virtual int  run() override;
    virtual void interrupt_cb(int signum) override;

    bool loadFromConfigDB();
    void defaultS3BucketPolicy();  // default  policy  desc  for s3 bucket
    DmtColumnPtr getDMTNodesForVolume(fds_volid_t volId);
    kvstore::ConfigDB* getConfigDB();

    static VolPolicyMgr      *om_policy_mgr();
    static const std::string &om_stor_prefix();

    int CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtPolPtr& crt_pol_req);
    int DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspDelPolPtr& del_pol_req);
    int ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspModPolPtr& mod_pol_req);
    void NotifyQueueFull(const fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                        const fpi::FDSP_NotifyQueueStateTypePtr& queue_state_req);
    void NotifyPerfstats(const boost::shared_ptr<fpi::AsyncHdr>& fdsp_msg,
                        const fpi::FDSP_PerfstatsType  * perf_stats_msg);
    int ApplyTierPolicy(::fpi::tier_pol_time_unitPtr& policy);  // NOLINT
    int AuditTierPolicy(::fpi::tier_pol_auditPtr& audit);  // NOLINT

    fds::snapshot::Manager snapshotMgr;
    DeleteScheduler deleteScheduler;
};

/* config path: cli -> OM  */
class FDSP_ConfigPathReqHandler : virtual public fpi::FDSP_ConfigPathReqIf {
  public:
        explicit FDSP_ConfigPathReqHandler(OrchMgr *oMgr);

        int32_t CreateVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateVolType& crt_vol_req);
        int32_t CreateVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& crt_vol_req);

        int32_t DeleteVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DeleteVolType& del_vol_req);
        int32_t DeleteVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& del_vol_req);

        int32_t ModifyVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ModifyVolType& mod_vol_req);
        int32_t ModifyVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ModifyVolTypePtr& mod_vol_req);

        int32_t SnapVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateVolType& snap_vol_req);
        int32_t SnapVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& snap_vol_req);

        int32_t CreatePolicy(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreatePolicyType& crt_pol_req);
        int32_t CreatePolicy(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr& crt_pol_req);

        int32_t DeletePolicy(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DeletePolicyType& del_pol_req);
        int32_t DeletePolicy(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr& del_pol_req);

        int32_t ModifyPolicy(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ModifyPolicyType& mod_pol_req);
        int32_t ModifyPolicy(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr& mod_pol_req);

        int32_t AttachVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& atc_vol_req);
        int32_t AttachVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& atc_vol_req);

        int32_t DetachVol(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& dtc_vol_req);
        int32_t DetachVol(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& dtc_vol_req);

        int32_t AssociateRespCallback(const int64_t ident);
        int32_t AssociateRespCallback(boost::shared_ptr<int64_t>& ident); // NOLINT

        int32_t CreateDomain(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateDomainType& crt_dom_req);
        int32_t CreateDomain(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateDomainTypePtr& crt_dom_req);

        int32_t DeleteDomain(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateDomainType& del_dom_req);
        int32_t DeleteDomain(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateDomainTypePtr& del_dom_req);

        int32_t ShutdownDomain(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ShutdownDomainType& shut_dom_req);
        int32_t ShutdownDomain(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ShutdownDomainTypePtr& shut_dom_req);

        int32_t RemoveServices(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_RemoveServicesType& rm_svc_req);
        int32_t RemoveServices(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_RemoveServicesTypePtr& rm_svc_req);

        int32_t SetThrottleLevel(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ThrottleMsgType& throttle_msg);
        int32_t SetThrottleLevel(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ThrottleMsgTypePtr& throttle_msg);

        void GetVolInfo(
            ::FDS_ProtocolInterface::FDSP_VolumeDescType& _return,
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_GetVolInfoReqType& vol_info_req);
        void GetVolInfo(
            ::FDS_ProtocolInterface::FDSP_VolumeDescType& _return,
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_GetVolInfoReqTypePtr& vol_info_req);

        int32_t GetDomainStats(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_GetDomainStatsType& get_stats_msg);
        int32_t GetDomainStats(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr& get_stats_msg);

        int32_t ActivateAllNodes(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ActivateAllNodesType& act_node_msg);
        int32_t ActivateAllNodes(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ActivateAllNodesTypePtr& act_node_msg);

        int32_t ActivateNode(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ActivateOneNodeType& act_node_msg);
        int32_t ActivateNode(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ActivateOneNodeTypePtr& act_node_msg);

        int32_t ScavengerCommand(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ScavengerType& gc_msg);
        int32_t ScavengerCommand(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ScavengerTypePtr& gc_msg);

        int32_t applyTierPolicy(
            const ::FDS_ProtocolInterface::tier_pol_time_unit& policy);
        int32_t applyTierPolicy(
            ::FDS_ProtocolInterface::tier_pol_time_unitPtr& policy);

        int32_t auditTierPolicy(
            const ::FDS_ProtocolInterface::tier_pol_audit& audit);
        int32_t auditTierPolicy(
            ::FDS_ProtocolInterface::tier_pol_auditPtr& audit);

        void ListServices(std::vector<fpi::FDSP_Node_Info_Type> & ret,
                          const fpi::FDSP_MsgHdrType& fdsp_msg);
        void ListServices(std::vector<fpi::FDSP_Node_Info_Type> & ret,
                          boost::shared_ptr<fpi::FDSP_MsgHdrType>& fdsp_msg);

        void ListVolumes(
            std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> & _return,
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg);
        void ListVolumes(
            std::vector<FDS_ProtocolInterface::FDSP_VolumeDescType> & _return,
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);

  private:
    OrchMgr *orchMgr;
};

/* OM control path: SH/SM/DM to OM */
class FDSP_OMControlPathReqHandler : virtual public fpi::FDSP_OMControlPathReqIf,
        virtual public PlatNetSvcHandler {
  public:
        explicit FDSP_OMControlPathReqHandler(OrchMgr *oMgr);

        void CreateBucket(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_CreateVolType& crt_buck_req);
        void CreateBucket(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_CreateVolTypePtr& crt_buck_req);

        void DeleteBucket(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DeleteVolType& del_buck_req);
        void DeleteBucket(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& del_buck_req);

        void ModifyBucket(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ModifyVolType& mod_buck_req);
        void ModifyBucket(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ModifyVolTypePtr& mod_buck_req);

        void AttachBucket(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolCmdType& atc_buck_req);
        void AttachBucket(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr& atc_buck_req);

        void RegisterNode(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_RegisterNodeType& reg_node_req);
        void RegisterNode(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr& reg_node_req);

        void NotifyQueueFull(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_NotifyQueueStateType& queue_state_info);
        void NotifyQueueFull(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_NotifyQueueStateTypePtr& queue_state_info);

        void NotifyPerfstats(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_PerfstatsType& perf_stats_msg);
        void NotifyPerfstats(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_PerfstatsTypePtr& perf_stats_msg);

        void TestBucket(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_TestBucket& test_buck_msg);
        void TestBucket(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_TestBucketPtr& test_buck_msg);

        void GetDomainStats(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_GetDomainStatsType& get_stats_msg);
        void GetDomainStats(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr& get_stats_msg);

        void NotifyMigrationDone(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_MigrationStatusType& status_msg);
        void NotifyMigrationDone(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_MigrationStatusTypePtr& status_msg);

        void migrationDone(
                boost::shared_ptr<fpi::AsyncHdr>& hdr,
                boost::shared_ptr<fpi::CtrlNotifyMigrationStatus>& status);

  private:
        OrchMgr* orchMgr;
};

/* control response handler*/
class FDSP_ControlPathRespHandler : virtual public fpi::FDSP_ControlPathRespIf {
  public:
        explicit FDSP_ControlPathRespHandler(OrchMgr *oMgr);

        void NotifyAddVolResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_add_vol_resp);
        void NotifyAddVolResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_add_vol_resp);

        void NotifyRmVolResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_rm_vol_resp);
        void NotifyRmVolResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_rm_vol_resp);

        void NotifyModVolResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_mod_vol_resp);
        void NotifyModVolResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_mod_vol_resp);

        void NotifySnapVolResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_NotifyVolType& not_snap_vol_resp);
        void NotifySnapVolResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_snap_vol_resp);

        void AttachVolResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolType& atc_vol_resp);
        void AttachVolResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolTypePtr& atc_vol_resp);

        void DetachVolResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_AttachVolType& dtc_vol_resp);
        void DetachVolResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_AttachVolTypePtr& dtc_vol_resp);

        void NotifyNodeAddResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
        void NotifyNodeAddResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

        void NotifyNodeActiveResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
        void NotifyNodeActiveResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

        void NotifyNodeRmvResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
        void NotifyNodeRmvResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

        void NotifyDLTUpdateResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DLT_Resp_Type& dlt_info_resp);
        void NotifyDLTUpdateResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DLT_Resp_TypePtr& dlt_info_resp);

        void NotifyDLTCloseResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DLT_Resp_Type& dlt_info_resp);
        void NotifyDLTCloseResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DLT_Resp_TypePtr& dlt_info_resp);

        void NotifyDMTCloseResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_info_resp);
        void NotifyDMTCloseResp(
            FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_info_resp);

        void PushMetaDMTResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_PushMeta& push_meta_resp);
        void PushMetaDMTResp(
            FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            FDS_ProtocolInterface::FDSP_PushMetaPtr& push_meta_resp);

        void NotifyDMTUpdateResp(
            const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_info_resp);
        void NotifyDMTUpdateResp(
            FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_info_resp);

  private:
        OrchMgr* orchMgr;
};

std::thread* runConfigService(OrchMgr* om);
extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_ORCHMGR_H_
