/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the orchestration  manager component.
 *
 * Orchestration Manager's Lock Domain documentation.
 *
 * I. Overview
 * ~~~~~~~~~~~
 * The OM is in the control/admin path that manages, configures, monitors, and
 * coordinates all FDS's components, as well as logical and physical resources.
 * It's critical that OM must not be blocked due to any single buggy code or
 * faulty resource, which could choke off its ability to response in timely
 * manner.
 *
 * II. OM Design Philosophy
 * ------------------------
 * The current design can't scale OM to its future roles.  It's monolithic
 * design with single lock with add on components.  The good model for OM
 * would be a distributed design with independent peer components working
 * through contract bindings instead of master-slave relationship.
 *
 * When all OM compoents working together through contract bindings, there's
 * no need to do locking accross components.  Each component is responsible
 * to protect its own data set to carry out the contract, very much the same
 * way OS syscall or thread-safe lib provide their services.
 *
 * Another side benefits of distributed, contract binding components design
 * is that one component can't change states or internal data structures of its
 * peer in unexpected way.  If all contract bindings can be expressed in state
 * machine design, the correctness of inter-component interaction can be
 * proven by:
 *   1) Verification of state machine design.
 *   2) Faithful state machine implementation.
 *   3) Random inputs to the state machine driven by "chaostic monkey" unit
 *      test code.
 *
 * III. OM Locking Philosophy
 * --------------------------
 * At the macro level, OM must handle the following input souces:
 * 1) User inputs classified in sub categories as:
 *   - Configuration requests that alter the system's working states, resource
 *     allocation...  OM needs to encode locking policies for these requests
 *     tabulation form instead of coding logic.  The locking table shows the
 *     proof of correctness in operational sense.
 *   - Management requests that alter policies on existing resources.  This
 *     code path can only block other conflicting requests from the same
 *     categoriy.  It can't lock any resources that could block requests from
 *     config for a well-define amount of time.  OM also needs to encode locking
 *     policies in this code path in tabulation format.  If this code path
 *     is blocked out by config path, it must return the status back to user
 *     instead of blocking for a long time.
 *   - Monitoring request to monitor resources or audit policies.  This code
 *     path should not need any exclusive lock.  Don't apply read lock in
 *     this code path either because it may starve requests in the config path.
 * 2) Notification from FDS SW components such as SM, SH, or peer OM, which
 *  could be classifed further as:
 *   - Notification of HW, system related about the availability of physical
 *     resources.
 *   - Notification about the availability of logical resources.
 *   - Notification about results of previous operations done by these
 *     components.
 * 3) Synchronization of peer OMs to keep the system image view at eventual
 *  consistent state.
 *
 * At resource view, OM must support the following resource domains:
 * 1) Physical inventory of all nodes that it manages.
 * 2) Logical resources and their dependencies tree.
 * 3) Logical resources that it knows and monitors.
 * 4) Configuration profiles of all logical resources that it managed.
 * 5) Policy profiles of all logical resources and services that they provide.
 *
 * To avoid dead lock, OM needs to document locking order properly once it has
 * a blue print for the architecture.
 */

#ifndef SOURCE_ORCH_MGR_ORCHMGR_H_
#define SOURCE_ORCH_MGR_ORCHMGR_H_

#include <unordered_map>
#include <cstdio>

#include <string>
#include <vector>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <fds_placement_table.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <fdsp/FDSP_OMControlPathReq.h>
#include <fdsp/FDSP_ControlPathResp.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <lib/Catalog.h>
#include <lib/PerfStats.h>
#include <NetSession.h>
#include "OmTier.h"
#include "OmVolPolicy.hpp"
#include "OmLocDomain.h"
#include "OmAdminCtrl.h"

#define MAX_OM_NODES 512
#define DEFAULT_LOC_DOMAIN_ID  1
#define DEFAULT_GLB_DOMAIN_ID  1


namespace fds {

    typedef std::string fds_node_name_t;
    typedef FDS_ProtocolInterface::FDSP_MgrIdType fds_node_type_t;
    typedef FDS_ProtocolInterface::FDSP_NodeState FdspNodeState;
    //    typedef FDS_ProtocolInterface::FDSP_ConfigPathReqPtr  ReqCfgHandlerPtr;
    //typedef FDS_ProtocolInterface::FDSP_ControlPathReqPrx ReqCtrlPrx;

    typedef FDS_ProtocolInterface::FDSP_MsgHdrTypePtr     FdspMsgHdrPtr;
    typedef FDS_ProtocolInterface::FDSP_CreateVolTypePtr  FdspCrtVolPtr;
    typedef FDS_ProtocolInterface::FDSP_DeleteVolTypePtr  FdspDelVolPtr;
    typedef FDS_ProtocolInterface::FDSP_ModifyVolTypePtr  FdspModVolPtr;

    typedef FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr FdspCrtPolPtr;
    typedef FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr FdspDelPolPtr;
    typedef FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr FdspModPolPtr;

    /*
     * NOTE: AttVolCmd is the command in the config plane, received at OM from CLI/GUI.
     * AttVol is the attach vol message sent from the OM to the HVs in the control plane.
     */
    typedef FDS_ProtocolInterface::FDSP_AttachVolTypePtr    FdspAttVolPtr;
    typedef FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr FdspAttVolCmdPtr;
    typedef FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr FdspRegNodePtr;
    typedef FDS_ProtocolInterface::FDSP_NotifyVolTypePtr    FdspNotVolPtr;
    typedef FDS_ProtocolInterface::FDSP_TestBucketPtr       FdspTestBucketPtr;

    typedef FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr FdspVolInfoPtr;
    typedef FDS_ProtocolInterface::FDSP_PolicyInfoTypePtr FdspPolInfoPtr;
    typedef FDS_ProtocolInterface::FDSP_VolumeDescTypePtr FdspVolDescPtr;
    typedef FDS_ProtocolInterface::FDSP_CreateDomainTypePtr  FdspCrtDomPtr;
    typedef FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr FdspGetDomStatsPtr;
    
    typedef std::unordered_map<int, localDomainInfo *> loc_domain_map_t;

    class OrchMgr:
    public FdsProcess,
    public Module {
    public:
      class FDSP_OMControlPathReqHandler;
  private:
        fds_log *om_log;
        SysParams *sysParams;
        netSessionTbl* net_session_tbl;
	boost::shared_ptr<FDSP_OMControlPathReqHandler> omc_req_handler;
        std::string my_node_name;

        /*
         * TODO: These maps should eventually be pulled out into
         * a separate class that defines a cluster map. In other
         * words, a class that defines the state of the cluster
         * at a certain point of time, which these maps being part
         * of that class.
         */
        loc_domain_map_t locDomMap;
        int current_dlt_version;
        int current_dmt_version;
        FDS_ProtocolInterface::Node_Table_Type current_dlt_table;
        FDS_ProtocolInterface::Node_Table_Type current_dmt_table;
        fds_mutex *om_mutex;
        std::string node_id_to_name[MAX_OM_NODES];
        const int table_type_dlt = 0;
        const int table_type_dmt = 1;
 
        /*
         * local domain 
         */
        FdsLocalDomain  *local_domain;
        FdsLocalDomain  *current_domain;
        
        /*
         * Cmdline configurables
         */
        int conf_port_num; /* config port to listen for cli commands */
        int ctrl_port_num; /* control port (register node + config cmds from AM) */
        std::string stor_prefix;
        fds_bool_t test_mode;
        
        /* policy manager */
        VolPolicyMgr        *policy_mgr;
        Thrift_VolPolicyServ   *om_ice_proxy;
        Orch_VolPolicyServ  *om_policy_srv;
        
        void SetThrottleLevelForDomain(int domain_id, float throttle_level);

  public:
    OrchMgr(int argc, char *argv[],
            const std::string& default_config_path,
            const std::string& base_path);
    ~OrchMgr();

    /**** From FdsProcess ****/
    virtual void setup(int argc, char *argv[], fds::Module **mod_vec) override;
    /* 
     * Runs the orch manager server.
     * Does not return until the server is no longer running
     */
    virtual void run() override;
    virtual void interrupt_cb(int signum) override;

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    fds_log* GetLog();
    void defaultS3BucketPolicy();  // default  policy  desc  for s3 bucket

    // With one big class, it's the same as using global variables for OM
    // with single big lock.
    //
    // We need to break it into functional components.  This call will return
    // the pointer to localDomainInfo and the caller will interact with it
    // until it's done.  The obj will take care of concurency inside its
    // data when we call its methods.  We shouldn't hold any OM lock
    // before and after this call.
    //
    localDomainInfo *om_GetDomainInfo(int domain_id)
    {
        return locDomMap[DEFAULT_LOC_DOMAIN_ID];
    }
    void om_BigLock() { om_mutex->lock(); }
    void om_BigUnlock() { om_mutex->unlock(); }

    int CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                  const FdspCrtVolPtr& crt_vol_req);
    int DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                  const FdspDelVolPtr& del_vol_req);
    int ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                  const FdspModVolPtr& mod_vol_req);

    int CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtPolPtr& crt_pol_req);
    int DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspDelPolPtr& del_pol_req);
    int ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspModPolPtr& mod_pol_req);

    int AttachVol(const FdspMsgHdrPtr& fdsp_msg,
                   const FdspAttVolCmdPtr& atc_vol_req);
    int DetachVol(const FdspMsgHdrPtr& fdsp_msg,
                   const FdspAttVolCmdPtr& dtc_vol_req);

    void RegisterNode(const FdspMsgHdrPtr& fdsp_msg,
                      const FdspRegNodePtr& reg_node_req);
    int CreateDomain(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtDomPtr& crt_dom_req);
    int DeleteDomain(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtDomPtr& del_dom_req);
    int SetThrottleLevel(const FDSP_MsgHdrTypePtr& fdsp_msg, 
                         const FDSP_ThrottleMsgTypePtr& throttle_req);
    void NotifyQueueFull(const FDSP_MsgHdrTypePtr& fdsp_msg,
                        const FDSP_NotifyQueueStateTypePtr& queue_state_req);
    void NotifyPerfstats(const FDSP_MsgHdrTypePtr& fdsp_msg,
                        const FDSP_PerfstatsTypePtr& perf_stats_msg);
    void TestBucket(const FDSP_MsgHdrTypePtr& fdsp_msg,
		    const FDSP_TestBucketPtr& test_buck_req);

    int GetDomainStats(const FdspMsgHdrPtr& fdsp_msg,
                       const FdspGetDomStatsPtr& get_stats_req);


    /* config path: cli -> OM  */
    class FDSP_ConfigPathReqHandler : 
    virtual public FDSP_ConfigPathReqIf {
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
        int32_t AssociateRespCallback(boost::shared_ptr<int64_t>& ident);

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

        int32_t SetThrottleLevel(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_ThrottleMsgType& throttle_msg);
        int32_t SetThrottleLevel(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_ThrottleMsgTypePtr& throttle_msg);

        int32_t GetVolInfo(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_GetVolInfoReqType& vol_info_req);
        int32_t GetVolInfo(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_GetVolInfoReqTypePtr& vol_info_req);

        int32_t GetDomainStats(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_GetDomainStatsType& get_stats_msg);
        int32_t GetDomainStats(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_GetDomainStatsTypePtr& get_stats_msg);

  private:
        OrchMgr *orchMgr;
    };

    /* OM control path: SH/SM/DM to OM */
    class FDSP_OMControlPathReqHandler :
    virtual public FDSP_OMControlPathReqIf {
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
  private:
        OrchMgr* orchMgr;
    };

    /* control response handler*/
    class FDSP_ControlPathRespHandler : 
    virtual public FDSP_ControlPathRespIf {
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

        void NotifyNodeRmvResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
        void NotifyNodeRmvResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

        void NotifyDLTUpdateResp(
            const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const ::FDS_ProtocolInterface::FDSP_DLT_Type& dlt_info_resp);
        void NotifyDLTUpdateResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DLT_TypePtr& dlt_info_resp);

        void NotifyDMTUpdateResp(
            const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
            const FDS_ProtocolInterface::FDSP_DMT_Type& dmt_info_resp);
        void NotifyDMTUpdateResp(
            ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
            ::FDS_ProtocolInterface::FDSP_DMT_TypePtr& dmt_info_resp);
  private:
        OrchMgr* orchMgr;
    };

    };

extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCHMGR_H_
