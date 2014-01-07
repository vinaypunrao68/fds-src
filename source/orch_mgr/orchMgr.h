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

#include <Ice/Ice.h>

#include <unordered_map>
#include <cstdio>

#include <string>
#include <vector>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_placement_table.h>
#include <fdsp/FDSP.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <lib/Catalog.h>
#include <lib/PerfStats.h>
#include "OmTier.h"
#include "OmVolPolicy.h"
#include "OmLocDomain.h"
#include "OmAdminCtrl.h"

#define MAX_OM_NODES 512
#define DEFAULT_LOC_DOMAIN_ID  1
#define DEFAULT_GLB_DOMAIN_ID  1

namespace fds {

  typedef std::string fds_node_name_t;
  typedef FDS_ProtocolInterface::FDSP_MgrIdType fds_node_type_t;
  typedef FDS_ProtocolInterface::FDSP_NodeState FdspNodeState;
  typedef FDS_ProtocolInterface::FDSP_ConfigPathReqPtr  ReqCfgHandlerPtr;
  typedef FDS_ProtocolInterface::FDSP_ControlPathReqPrx ReqCtrlPrx;

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

  class OrchMgr :
  virtual public Ice::Application,
          public Module {
  private:
    fds_log *om_log;
    SysParams *sysParams;
    ReqCfgHandlerPtr reqCfgHandlersrv;
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
    int port_num;
    std::string stor_prefix;
    fds_bool_t test_mode;

    /* policy manager */
    VolPolicyMgr        *policy_mgr;
    Ice_VolPolicyServ   *om_ice_proxy;
    Orch_VolPolicyServ  *om_policy_srv;

    void SetThrottleLevelForDomain(int domain_id, float throttle_level);

  public:
    OrchMgr();
    ~OrchMgr();

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    virtual int run(int argc, char* argv[]);
    void interruptCallback(int cb);

    /**
     * Runs the orch manager server.
     * This function is not intended to return until
     * the server is no longer running.
     */
    void runServer();

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
    void DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                   const FdspDelVolPtr& del_vol_req);
    void ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                   const FdspModVolPtr& mod_vol_req);

    void CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                      const FdspCrtPolPtr& crt_pol_req);
    void DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                      const FdspDelPolPtr& del_pol_req);
    void ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                      const FdspModPolPtr& mod_pol_req);

    void AttachVol(const FdspMsgHdrPtr& fdsp_msg,
                   const FdspAttVolCmdPtr& atc_vol_req);
    void DetachVol(const FdspMsgHdrPtr& fdsp_msg,
                   const FdspAttVolCmdPtr& dtc_vol_req);

    void RegisterNode(const FdspMsgHdrPtr& fdsp_msg,
                      const FdspRegNodePtr& reg_node_req);
    int CreateDomain(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtDomPtr& crt_dom_req);
    int DeleteDomain(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtDomPtr& del_dom_req);
    void SetThrottleLevel(const FDSP_MsgHdrTypePtr& fdsp_msg, 
			  const FDSP_ThrottleMsgTypePtr& throttle_req);
    void NotifyQueueFull(const FDSP_MsgHdrTypePtr& fdsp_msg,
			 const FDSP_NotifyQueueStateTypePtr& queue_state_req);
    void NotifyPerfstats(const FDSP_MsgHdrTypePtr& fdsp_msg,
			 const FDSP_PerfstatsTypePtr& perf_stats_msg);
    void TestBucket(const FDSP_MsgHdrTypePtr& fdsp_msg,
		    const FDSP_TestBucketPtr& test_buck_req);

    void GetDomainStats(const FdspMsgHdrPtr& fdsp_msg,
			const FdspGetDomStatsPtr& get_stats_req);

    class ReqCfgHandler : public FDS_ProtocolInterface::FDSP_ConfigPathReq {
   private:
      OrchMgr *orchMgr;

   public:
      explicit ReqCfgHandler(OrchMgr *oMgr);
      ~ReqCfgHandler();

      int CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtVolPtr& crt_vol_req,
                     const Ice::Current&);
      void DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspDelVolPtr& del_vol_req,
                     const Ice::Current&);
      void ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspModVolPtr& mod_vol_req,
                     const Ice::Current&);

      void CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspCrtPolPtr& crt_pol_req,
                        const Ice::Current&);
      void DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspDelPolPtr& del_pol_req,
                        const Ice::Current&);
      void ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspModPolPtr& mod_pol_req,
                        const Ice::Current&);

      void AttachVol(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspAttVolCmdPtr& atc_vol_req,
                     const Ice::Current&);
      void DetachVol(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspAttVolCmdPtr& dtc_vol_req,
                     const Ice::Current&);

      void GetVolInfo(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg, 
		      const ::FDS_ProtocolInterface::FDSP_GetVolInfoReqTypePtr& get_vol_req, 
		      const ::Ice::Current&);

      void RegisterNode(const FdspMsgHdrPtr&  fdsp_msg,
                        const FdspRegNodePtr& reg_node_req,
                        const Ice::Current&);

      void AssociateRespCallback(const Ice::Identity& ident,
                                 const Ice::Current& current);
      int CreateDomain(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtDomPtr& crt_dom_req,
                     const Ice::Current&);
      int DeleteDomain(const FdspMsgHdrPtr& fdsp_msg,
                     const FdspCrtDomPtr& del_dom_req,
                     const Ice::Current&);

      void GetDomainStats(const FdspMsgHdrPtr& fdsp_msg,
			  const FdspGetDomStatsPtr& get_stats_req,
			  const Ice::Current&);

      void SetThrottleLevel(const FDSP_MsgHdrTypePtr& fdsp_msg, 
			    const FDSP_ThrottleMsgTypePtr& throttle_req, 
			    const Ice::Current&);

      void NotifyQueueFull(const FDSP_MsgHdrTypePtr& fdsp_msg,
			   const FDSP_NotifyQueueStateTypePtr& queue_state_req, 
			   const Ice::Current&);

      void NotifyPerfstats(const FDSP_MsgHdrTypePtr& fdsp_msg,
			   const FDSP_PerfstatsTypePtr& perf_stats_msg,
			   const Ice::Current&);

      void TestBucket(const FDSP_MsgHdrTypePtr& fdsp_msg,
		      const FDSP_TestBucketPtr& test_buck_req,
		      const Ice::Current&);

    };
  };

extern OrchMgr *gl_orch_mgr;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCHMGR_H_
