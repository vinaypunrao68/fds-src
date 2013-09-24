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
#include "fdsp/fdsp_types.h"
#include "fdsp/FDSP.h"
#include "util/Log.h"
#include "util/concurrency/Mutex.h"

namespace fds {

  typedef std::string node_id_t;
  typedef FDSP_Types::FDSP_NodeState FdspNodeState;
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
   * TODO: What's the difference between AttachVol and AttachVolCmd?
   * Can we get rid of one?
   */
  typedef FDS_ProtocolInterface::FDSP_AttachVolTypePtr    FdspAttVolPtr;
  typedef FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr FdspAttVolCmdPtr;
  typedef FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr FdspRegNodePtr;
  typedef FDS_ProtocolInterface::FDSP_NotifyVolTypePtr    FdspNotVolPtr;

  typedef FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr FdspVolInfoPtr;

  class NodeInfo {
  public:
    node_id_t     node_id;
    unsigned int  node_ip_address;
    unsigned int  control_port;
    unsigned int  data_port;
    FdspNodeState node_state;
    ReqCtrlPrx    cpPrx;
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

    void copyVolumeInfoToProperties(FDS_Volume *pVol,
                                    FdspVolInfoPtr v_info);
    void copyPropertiesToVolumeInfo(FdspVolInfoPtr v_info,
                                    FDS_Volume *pVol);
    void initOMMsgHdr(const FdspMsgHdrPtr& fdsp_msg);
    void sendCreateVolToFdsNodes(VolumeInfo *pVol);
    void sendDeleteVolToFdsNodes(VolumeInfo *pVol);
    void sendAttachVolToHVNode(node_id_t node_id, VolumeInfo *pVol);
    void sendDetachVolToHVNode(node_id_t node_id, VolumeInfo *pVol);

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
    };
  };

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCHMGR_H_
