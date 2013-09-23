/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the orchestration  manager component.
 */

#ifndef ORCH_MGR_H
#define ORCH_MGR_H

#include <unordered_map>

#include <Ice/Ice.h>

#include "include/fds_types.h"
#include "include/fds_err.h"
#include "include/fds_volume.h"
#include "fdsp/fdsp_types.h"
#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "util/Log.h"
#include "util/concurrency/Mutex.h"

using namespace FDSP_Types;
using namespace FDS_ProtocolInterface;

namespace fds {

  typedef std::string node_id_t; 

  class NodeInfo {

  public:
    node_id_t node_id;
    unsigned int node_ip_address;
    unsigned int control_port;
    unsigned int data_port;
    FDSP_NodeState node_state;
    FDSP_ControlPathReqPrx  cpPrx;
  
  };

  class VolumeInfo {

  public:
    std::string vol_name;
    fds_volid_t volUUID;
    FDS_Volume  properties;
    std::vector<node_id_t> hv_nodes;
  };

  typedef std::unordered_map<node_id_t, NodeInfo> node_map_t;
  typedef std::unordered_map<int, VolumeInfo *> volume_map_t;

  class OrchMgr : virtual public Ice::Application {
  private:

    fds_log *om_log;
    typedef FDS_ProtocolInterface::FDSP_ConfigPathReqPtr  ReqCfgHandlerPtr;
    ReqCfgHandlerPtr   reqCfgHandlersrv;
    node_map_t currentSmMap;
    node_map_t currentDmMap;
    node_map_t currentShMap;
    volume_map_t volumeMap;
    fds_mutex *om_mutex;

    /*
     * Cmdline configurables
     */
    int port_num;
    std::string stor_prefix;

    void copyVolumeInfoToProperties(FDS_Volume *pVol, FDSP_VolumeInfoTypePtr v_info);
    void copyPropertiesToVolumeInfo(FDSP_VolumeInfoTypePtr v_info, FDS_Volume *pVol);
    void initOMMsgHdr(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg);
    void sendCreateVolToFdsNodes(VolumeInfo *pVol);
    void sendDeleteVolToFdsNodes(VolumeInfo *pVol);
    void sendAttachVolToHVNode(node_id_t node_id, VolumeInfo *pVol);
    void sendDetachVolToHVNode(node_id_t node_id, VolumeInfo *pVol);

  public:
    OrchMgr();
    ~OrchMgr();

    virtual int run(int argc, char* argv[]);
    void interruptCallback(int);
    fds_log* GetLog();

    void CreateVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		   const FDS_ProtocolInterface::FDSP_CreateVolTypePtr &crt_vol_req);
    void DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		   const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req);
    void ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		   const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req);
    void CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		      const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req);
    void DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		      const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req);
    void ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		      const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req);
    void AttachVol(const FDSP_MsgHdrTypePtr& fdsp_msg, 
		       const FDSP_AttachVolCmdTypePtr& atc_vol_req);
    void DetachVol(const FDSP_MsgHdrTypePtr& fdsp_msg, 
		       const FDSP_AttachVolCmdTypePtr& dtc_vol_req);
    void RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
		      const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req);

    class ReqCfgHandler : public FDS_ProtocolInterface::FDSP_ConfigPathReq {

     private:
    	OrchMgr *orchMgr;
	
     public:	
	explicit ReqCfgHandler(OrchMgr *oMgr);
	~ReqCfgHandler();

     	void CreateVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreateVolTypePtr &crt_vol_req,
			 const Ice::Current&);
  	void DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req,
			 const Ice::Current&);
  	void ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req,
			 const Ice::Current&);
 	void CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req,
			 const Ice::Current&);
  	void DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req,
			 const Ice::Current&);
  	void ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req,
			 const Ice::Current&);
	void AttachVol(const FDSP_MsgHdrTypePtr& fdsp_msg, 
		       const FDSP_AttachVolCmdTypePtr& atc_vol_req,
		       const Ice::Current&);
	void DetachVol(const FDSP_MsgHdrTypePtr& fdsp_msg, 
		       const FDSP_AttachVolCmdTypePtr& dtc_vol_req,
		       const Ice::Current&);
	void RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
				const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req,
			 const Ice::Current&);

    }; 

    };
  
}  // namespace fds

#endif  // ORCH_MGR_H
