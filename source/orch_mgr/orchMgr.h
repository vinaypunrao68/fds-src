/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the orchestration  manager component.
 */

#ifndef SOURCE_ORCH_MGR_ORCHMGR_H_
#define SOURCE_ORCH_MGR_ORCHMGR_H_

#include <Ice/Ice.h>

#include <unordered_map>
#include <string>
#include <vector>

#include "include/fds_types.h"
#include "include/fds_err.h"
#include "include/fds_volume.h"
#include "fdsp/FDSP.h"
#include "util/Log.h"
#include "util/concurrency/Mutex.h"

#define MAX_OM_NODES 512

namespace fds {

  typedef std::string fds_node_name_t;
  typedef int fds_node_id_t;
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
   * NOTE: AttVolCmd is the command in the config plane, received at OMfrom CLI/GUI.
   * AttVol is the attach vol message sent from the OM to the HVs in the control plane.
   */
  typedef FDS_ProtocolInterface::FDSP_AttachVolTypePtr    FdspAttVolPtr;
  typedef FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr FdspAttVolCmdPtr;
  typedef FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr FdspRegNodePtr;
  typedef FDS_ProtocolInterface::FDSP_NotifyVolTypePtr    FdspNotVolPtr;

  typedef FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr FdspVolInfoPtr;
  
  class NodeInfo {
 private:
 public:
    fds_node_name_t node_name;
    fds_node_id_t node_id;
    fds_node_type_t node_type;
    fds_uint32_t  node_ip_address;
    fds_uint32_t  control_port;
    fds_uint32_t  data_port;
    FdspNodeState node_state;
    ReqCtrlPrx    cpPrx;

 public:
    NodeInfo() { }
    NodeInfo(const fds_node_id_t& _id,
	     const fds_node_name_t& _name,
	     const fds_node_type_t& _type,
             fds_uint32_t _ip,
             fds_uint32_t _cp_port,
             fds_uint32_t _d_port,
             const FdspNodeState& _state,
             const ReqCtrlPrx& _prx) :
    node_id(_id),
      node_name(_name),
      node_type(_type),
      node_ip_address(_ip),
      control_port(_cp_port),
      data_port(_d_port),
      node_state(_state),
      cpPrx(_prx) {
    }

    ~NodeInfo() {
    }
  };

  class VolumeInfo {
  public:
    std::string vol_name;
    fds_volid_t volUUID;
    FDS_Volume  properties;
    std::vector<fds_node_name_t> hv_nodes;
  };

  typedef std::unordered_map<fds_node_name_t, NodeInfo> node_map_t;
  typedef std::unordered_map<int, VolumeInfo *> volume_map_t;

  class OrchMgr : virtual public Ice::Application {
  private:
    fds_log *om_log;
    ReqCfgHandlerPtr   reqCfgHandlersrv;
    node_map_t currentSmMap;
    node_map_t currentDmMap;
    node_map_t currentShMap;
    volume_map_t volumeMap;
    fds_mutex *om_mutex;
    std::string node_id_to_name[MAX_OM_NODES];

    /*
     * Cmdline configurables
     */
    int port_num;
    std::string stor_prefix;

    void copyVolumeInfoToProperties(FDS_Volume *pVol,
                                    FdspVolInfoPtr v_info);
    void copyPropertiesToVolumeInfo(FdspVolInfoPtr v_info,
                                    FDS_Volume *pVol);
    void initOMMsgHdr(const FdspMsgHdrPtr& fdsp_msg);

    int  getFreeNodeId(std::string& node_name); // Get a new node_id to be allocated for a new node 

    void sendNodeEventToFdsNodes(NodeInfo& nodeInfo, FDS_ProtocolInterface::FDSP_NodeState node_state); // Broadcast a node event to all DM/SM/HV nodes
    void sendCreateVolToFdsNodes(VolumeInfo *pVol); // Broadcast create vol ctrl message to all DM/SM Nodes
    void sendDeleteVolToFdsNodes(VolumeInfo *pVol); // Broadcast delete vol ctrl message to all DM/SM Nodes
    void sendAllVolumesToFdsMgrNode(NodeInfo node_info); // Dump all existing volumes (as a sequence of create vol ctrl messages) to a newly registering SM/DM Node
    void sendAttachVolToHvNode(fds_node_name_t node_name, VolumeInfo *pVol); // Send attach vol ctrl message to a HV node
    void sendDetachVolToHvNode(fds_node_name_t node_name, VolumeInfo *pVol); // Send detach vol ctrl message to a HV node
    void sendAllVolumesToHvNode(fds_node_name_t node_name); // Dump all concerned volumes as a sequence of attach vol ctrl messages to a HV node
    void sendMgrNodeListToFdsNode(NodeInfo& n_info); // Dump all existing SM/DM nodes info as a sequence of NotifyNodeAdd ctrl messages to a newly registering node

  public:
    OrchMgr();
    ~OrchMgr();

    virtual int run(int argc, char* argv[]);
    void interruptCallback(int cb);
    fds_log* GetLog();

    void CreateVol(const FdspMsgHdrPtr& fdsp_msg,
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

    class ReqCfgHandler : public FDS_ProtocolInterface::FDSP_ConfigPathReq {
   private:
      OrchMgr *orchMgr;

   public:
      explicit ReqCfgHandler(OrchMgr *oMgr);
      ~ReqCfgHandler();

      void CreateVol(const FdspMsgHdrPtr& fdsp_msg,
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

      void RegisterNode(const FdspMsgHdrPtr&  fdsp_msg,
                        const FdspRegNodePtr& reg_node_req,
                        const Ice::Current&);

      void AssociateRespCallback(const Ice::Identity& ident,
                                 const Ice::Current& current);
    };
  };

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCHMGR_H_
