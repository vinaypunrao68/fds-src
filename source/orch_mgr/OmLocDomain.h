/*
 * local domain  class 
 *   formation Data systems 
 */
#ifndef ORCH_MGR_LOCDOMAIN_H_
#define ORCH_MGR_LOCDOMAIN_H_

#include <Ice/Ice.h>
#include <unordered_map>
#include <cstdio>
#include <string>
#include <string>
#include <vector>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_placement_table.h>
#include <fds_volume.h>
#include <fdsp/FDSP.h>
#include <util/Log.h>
#include "orchMgr.h"
#include "OmAdminCtrl.h"

#define MAX_OM_NODES 512

namespace fds {

  typedef std::string fds_node_name_t;
  typedef FDS_ProtocolInterface::FDSP_MgrIdType fds_node_type_t;
  typedef FDS_ProtocolInterface::FDSP_NodeState FdspNodeState;
  typedef FDS_ProtocolInterface::FDSP_ControlPathReqPrx ReqCtrlPrx;
  typedef FDS_ProtocolInterface::FDSP_VolumeDescTypePtr FdspVolDescPtr; 
  typedef FDS_ProtocolInterface::FDSP_MsgHdrTypePtr     FdspMsgHdrPtr;


 class NodeInfo {
    /*
     * TODO: Make these private and add accessor/mutator
     * functions. That's better c++ style.
     */
 public:
    fds_node_name_t node_name;
    fds_nodeid_t node_id;
    fds_node_type_t node_type;
    fds_uint32_t  node_ip_address;
    fds_uint32_t  control_port;
    fds_uint32_t  data_port;
    fds_uint32_t  disk_iops_max;
    fds_uint32_t  disk_iops_min;
    double  	  disk_capacity;
    fds_uint32_t  disk_latency_max;
    fds_uint32_t  disk_latency_min;
    fds_uint32_t  ssd_iops_max;
    fds_uint32_t  ssd_iops_min;
    double        ssd_capacity;
    fds_uint32_t  ssd_latency_max;
    fds_uint32_t  ssd_latency_min;
    fds_uint32_t  disk_type;
    FdspNodeState node_state;
    ReqCtrlPrx    cpPrx;

 public:
    NodeInfo() { }

    NodeInfo(const fds_nodeid_t& _id,
          const fds_node_name_t& _name,
          const fds_node_type_t& _type,
          fds_uint32_t _ip,
          fds_uint32_t _cp_port,
          fds_uint32_t _d_port,
    	  fds_int32_t _disk_iops_max,
    	  fds_int32_t _disk_iops_min,
   	  double      _disk_capacity,
    	  fds_int32_t _d_latency_max,
    	  fds_int32_t _d_latency_min,
    	  fds_int32_t _ssd_iops_max,
    	  fds_int32_t _ssd_iops_min,
    	  double      _ssd_capacity,
    	  fds_int32_t _s_latency_max,
    	  fds_int32_t _s_latency_min,
    	  fds_uint32_t  disk_type,
          const FdspNodeState& _state,
          const ReqCtrlPrx& _prx) :
    node_id(_id),
        node_name(_name),
        node_type(_type),
        node_ip_address(_ip),
        control_port(_cp_port),
        data_port(_d_port),
    	disk_iops_max(_disk_iops_max),
    	disk_iops_min(_disk_iops_min),
   	disk_capacity(_disk_capacity),
    	disk_latency_max(_d_latency_max),
    	disk_latency_min(_d_latency_min),
    	ssd_iops_max(_ssd_iops_max),
    	ssd_iops_min(_ssd_iops_min),
    	ssd_capacity(_ssd_capacity),
    	ssd_latency_max(_s_latency_max),
    	ssd_latency_min(_s_latency_min),
        disk_type(_type),
        node_state(_state),
        cpPrx(_prx) {
        }

    /*
     * This constructor below is only used for
     * testing. It does not communicate with any
     * node via Ice, so does not take a Prx.
     */
    NodeInfo(const fds_node_name_t& _name,
          fds_uint32_t _ip,
          fds_uint32_t _cp_port,
          fds_uint32_t _d_port,
    	  fds_int32_t _disk_iops_max,
    	  fds_int32_t _disk_iops_min,
   	  double      _disk_capacity,
    	  fds_int32_t _d_latency_max,
    	  fds_int32_t _d_latency_min,
    	  fds_int32_t _ssd_iops_max,
    	  fds_int32_t _ssd_iops_min,
    	  double      _ssd_capacity,
    	  fds_int32_t _s_latency_max,
    	  fds_int32_t _s_latency_min,
    	  fds_int32_t _d_type,
          const FdspNodeState& _state) :
    node_id(0),
        node_name(_name),
        node_ip_address(_ip),
        control_port(_cp_port),
        data_port(_d_port),
    	disk_iops_max(100000),
    	disk_iops_min(1000),
   	disk_capacity(1000),
    	disk_latency_max(1000),
    	disk_latency_min(100),
    	ssd_iops_max(1000000),
    	ssd_iops_min(10000),
    	ssd_capacity(10000),
    	ssd_latency_max(100),
    	ssd_latency_min(10),
    	disk_type(1),
        node_state(_state) {
        }

    ~NodeInfo() {
    }

    /*
     * TODO: This should have a copy constructor and an
     * assignment operator since they're being used by
     * the maps. The default ones are dangerous.
     */
  };

  class VolumeInfo {
 public:
    std::string vol_name;
    fds_volid_t volUUID;
    VolumeDesc  *properties;
    std::vector<fds_node_name_t> hv_nodes;
  };

class FdsAdminCtrl;

class FdsLocalDomain {

 public:
  /*
   * addmission control 
   */
   FdsAdminCtrl  *admin_ctrl;


  FdsLocalDomain(const std::string& om_prefix, fds_log* om_log);
  ~FdsLocalDomain();

  typedef std::unordered_map<fds_node_name_t, NodeInfo> node_map_t;
  typedef std::unordered_map<std::string, VolumeInfo *> volume_map_t;

  node_map_t currentSmMap;
  node_map_t currentDmMap;
  node_map_t currentShMap;
  volume_map_t volumeMap;

  fds_mutex *dom_mutex;
  /*
   * Updates both DMT and DLT under a single lock.
   */
   static void roundRobinDlt(fds_placement_table* table,
                              const node_map_t& nodeMap,
                              fds_log* callerLog);
  void updateTables();
  void updateDltLocked();
  void updateDmtLocked();

    /*
     * Persistent DLT and DMT histories.
     * TODO: Persistently store cluster map info
     * info that was used to generate the DLT and DMT.
     */
    fds_uint64_t curDltVer;
    fds_uint32_t dltWidth;
    fds_uint32_t dltDepth;
    fds_uint64_t curDmtVer;
    fds_uint32_t dmtWidth;
    fds_uint32_t dmtDepth;
    FdsDlt *curDlt;
    FdsDmt *curDmt;
    Catalog *dltCatalog;
    Catalog *dmtCatalog;
    std::string node_id_to_name[MAX_OM_NODES];


    const int table_type_dlt = 0;
    const int table_type_dmt = 1;
    int current_dlt_version;
    int current_dmt_version;

    int next_free_vol_id;

    float current_throttle_level;

    void copyPropertiesToVolumeDesc(FdspVolDescPtr v_desc,
                                    VolumeDesc *pVol);
    void initOMMsgHdr(const FdspMsgHdrPtr& fdsp_msg);

    /*
     * Get a new node_id to be allocated for a new node
     */
    fds_int32_t getFreeNodeId(const std::string& node_name);

    fds_int32_t getNextFreeVolId();
    /*
     * Broadcast a node event to all DM/SM/HV nodes
     */
    void sendNodeEventToFdsNodes(const NodeInfo& nodeInfo,
                                 FDS_ProtocolInterface::FDSP_NodeState
                                 node_state);
    /*
     * Broadcast create vol ctrl message to all DM/SM Nodes
     */
    void sendCreateVolToFdsNodes(VolumeInfo *pVol);
    /*
     * Broadcast modify vol ctrl message to all SH/DM/SM Nodes
     */
    void sendModifyVolToFdsNodes(VolumeInfo *pVol);
    /*
     * Broadcast delete vol ctrl message to all DM/SM Nodes
     */
    void sendDeleteVolToFdsNodes(VolumeInfo *pVol);
    /*
     * Dump all existing volumes (as a sequence of create vol
     * ctrl messages) to a newly registering SM/DM Node
     */
    void sendAllVolumesToFdsMgrNode(NodeInfo node_info);
    /*
     * Send attach vol ctrl message to a HV node
     */
    void sendAttachVolToHvNode(fds_node_name_t node_name, VolumeInfo *pVol);
    /*
     * Send detach vol ctrl message to a HV node
     */
    void sendDetachVolToHvNode(fds_node_name_t node_name, VolumeInfo *pVol);
    /*
     * Dump all concerned volumes as a sequence of
     * attach vol ctrl messages to a HV node
     */
    void sendAllVolumesToHvNode(fds_node_name_t node_name);
    /*
     * Dump all existing SM/DM nodes info as a sequence
     * of NotifyNodeAdd ctrl messages to a newly registering
     * node
     */
    void sendMgrNodeListToFdsNode(const NodeInfo& n_info);
    /*
     * Broadcast current DLT or DMT to all SM/DM/HV
     * nodes known to OM
     */
    void sendNodeTableToFdsNodes(int table_type);

    /*
     * Broadcast tier policy to all SM nodes.
     */
    void sendTierPolicyToSMNodes(const FDSP_TierPolicyPtr &tier);
    void sendTierAuditPolicyToSMNodes(const FDSP_TierPolicyAuditPtr &tier);

    /*
      Broadcast SetThrottleLevel message to all SH Nodes
    */
    void sendThrottleLevelToHvNodes(float throttle_level);
 
    /* Send response for TestBucket if we are not sending attach volume message */ 
    void sendTestBucketResponseToHvNode(fds_node_name_t node_name, const std::string& bucket_name, fds_bool_t vol_exists);
   
    /*
     * Testing related member functions
     */
    void loadNodesFromFile(const std::string& dltFileName,
                          const std::string& dmtFileName);

    void handlePerfStatsFromAM(const FDSP_VolPerfHistListType& hist_list,
			       const std::string start_timestamp);

  /* parent log */
  fds_log* parent_log;

 private: 

  /* recent history of perf stats OM receives from AM nodes */
  PerfStats* am_stats;

}; 

 class localDomainInfo {
  public:
    std::string loc_domain_name;
    fds_int32_t  loc_dom_id;
    fds_int32_t  glb_dom_id; 
    FdsLocalDomain  *domain_ptr;
  };

}
#endif
