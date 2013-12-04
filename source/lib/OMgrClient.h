#ifndef _OMGRCLIENT_H
#define _OMGRCLIENT_H
#include <fds_err.h>
#include <fds_volume.h>
#include "fdsp/FDSP.h"
#include <util/Log.h>
#include <unordered_map>
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <concurrency/RwLock.h>
#include <net-proxies/vol_policy.h>

using namespace FDS_ProtocolInterface;

#define FDS_VOL_ACTION_NONE   0
#define FDS_VOL_ACTION_CREATE 1
#define FDS_VOL_ACTION_DELETE 2
#define FDS_VOL_ACTION_MODIFY 3
#define FDS_VOL_ACTION_ATTACH 4
#define FDS_VOL_ACTION_DETACH 5

typedef struct _node_info_t {

  int node_id;
  unsigned int node_ip_address;
  fds_uint32_t port;
  FDSP_NodeState node_state;

} node_info_t;

typedef std::unordered_map<int,node_info_t> node_map_t;

namespace fds {

  typedef enum {
    fds_notify_vol_default = 0,
    fds_notify_vol_add     = 1,
    fds_notify_vol_rm      = 2,
    fds_notify_vol_mod     = 3,
    fds_notify_vol_attatch = 4,
    fds_notify_vol_detach  = 5, 
    MAX
  } fds_vol_notify_t;
  
  typedef void (*node_event_handler_t)(int node_id,
                                       unsigned int node_ip_addr,
                                       int node_state,
                                       fds_uint32_t node_port,
                                       FDSP_MgrIdType node_type);
  typedef void (*volume_event_handler_t)(fds::fds_volid_t volume_id, 
					 fds::VolumeDesc *vdb, 
					 fds_vol_notify_t vol_action,
					 const FDSP_ResultType result);
  typedef void (*throttle_cmd_handler_t)(const float throttle_level);
  typedef void (*tier_cmd_handler_t)(const FDSP_TierPolicyPtr &tier);
  typedef void (*tier_audit_cmd_handler_t)(const FDSP_TierPolicyAuditPtr &tier);

  class OMgrClient {

  private:
    int tennant_id;
    int domain_id;
    FDSP_MgrIdType my_node_type;
    std::string my_node_name;
    std::string omIpStr;
    fds_uint32_t omConfigPort;
    std::string hostIp;
    fds_uint32_t my_control_port;
    fds_uint32_t my_data_port;
    node_map_t node_map;
    int dlt_version;
    Node_Table_Type dlt;
    int dmt_version;
    Node_Table_Type dmt;
    float current_throttle_level;
    
    fds_rwlock omc_lock; // to protect node_map

    node_event_handler_t node_evt_hdlr;
    volume_event_handler_t vol_evt_hdlr;
    throttle_cmd_handler_t throttle_cmd_hdlr;
    tier_cmd_handler_t       tier_cmd_hdlr;
    tier_audit_cmd_handler_t tier_audit_cmd_hdlr;

#if 0
    Ice::CommunicatorPtr pubsub_comm;
    IceStorm::TopicManagerPrx pubsub_topicManager;
    Ice::ObjectAdapterPtr pubsub_adapter;
    FDS_OMgr_SubscriberPtr om_pubsub_client_i;
    Ice::ObjectPrx pubsub_proxy;
    IceStorm::TopicPrx pubsub_topic;
#endif

    std::string          rpc_srv_id;
    Ice::CommunicatorPtr rpc_comm;
    Ice::ObjectAdapterPtr rpc_adapter;
    FDS_ProtocolInterface::FDSP_ControlPathReqPtr om_client_rpc_i;
    void initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);
    int initRPCComm();

  public:

    OMgrClient();
    OMgrClient(FDSP_MgrIdType node_type,
               const std::string& _omIpStr,
               fds_uint32_t _omPort,
               const std::string& _hostIp,
               fds_uint32_t data_port,
               const std::string& node_name,
               fds_log *parent_log);
    int initialize();
    int registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr);
    int registerEventHandlerForVolEvents(volume_event_handler_t vol_event_hdlr);
    int registerThrottleCmdHandler(throttle_cmd_handler_t throttle_cmd_hdlr);

    // This logging is public for external plugins.  Avoid making this object
    // too big and all methods uses its data as global variables with big lock.
    //
    fds_log        *omc_log;

    // Extneral plugin object to handle policy requests.
    VolPolicyServ  *omc_srv_pol;

    //    int subscribeToOmEvents(unsigned int om_ip_addr, int tennant_id, int domain_id, int omc_port_num=0);
    int startAcceptingControlMessages();
    int startAcceptingControlMessages(fds_uint32_t port_num);
    int registerNodeWithOM(const FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr& diskInfo);
    int pushCreateBucketToOM(const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& volInfo);
    int pushDeleteBucketToOM(const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& volInfo);
    int pushModifyBucketToOM(const std::string& bucket_name,
			     const FDS_ProtocolInterface::FDSP_VolumeDescTypePtr& vol_desc);

    int getNodeInfo(int node_id,
                    unsigned int *node_ip_addr,
                    fds_uint32_t *node_port,
                    int *node_state);
    int getDLTNodesForDoidKey(unsigned char doid_key,
                              fds_int32_t *node_ids,
                              fds_int32_t *n_nodes);
    int getDMTNodesForVolume(int vol_id, int *node_ids, int *n_nodes);
    int pushPerfstatsToOM(const std::string& start_ts,
			  int stat_slot_len, 
			  const FDS_ProtocolInterface::FDSP_VolPerfHistListType& hist_list);
    int testBucket(const std::string& bucket_name,
		   const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& vol_info,
		   fds_bool_t attach_vol_reqd,
		   const std::string& accessKeyId,
		   const std::string& secretAccessKey);

    int recvNodeEvent(int node_id, FDSP_MgrIdType node_type, unsigned int node_ip, int node_state, const FDSP_Node_Info_TypePtr& node_info);
    int recvDLTUpdate(int dlt_version, const Node_Table_Type& dlt_table);
    int recvDMTUpdate(int dmt_version, const Node_Table_Type& dmt_table);

    int recvNotifyVol(fds_volid_t vol_id,
                      VolumeDesc *vdb,
                      fds_vol_notify_t vol_action,
		      FDSP_ResultType);
    int recvVolAttachState(fds_volid_t vol_id, VolumeDesc *vdb, int vol_action, FDSP_ResultType);
    int recvSetThrottleLevel(const float throttle_level);
    int recvTierPolicy(const FDSP_TierPolicyPtr &tier);
    int recvTierPolicyAudit(const FDSP_TierPolicyAuditPtr &audit);
  };

  class OMgrClientRPCI : public FDS_ProtocolInterface::FDSP_ControlPathReq {

  private:
    OMgrClient *om_client;
  

  public:

    OMgrClientRPCI(OMgrClient *om_c);
      
    void NotifyAddVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
		   const Ice::Current&);

    void NotifyRmVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
		   const Ice::Current&);

    void NotifyModVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
		   const Ice::Current&);
      
    void AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
		   const Ice::Current&);
      
    void DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
		   const Ice::Current&);

    void NotifyNodeAdd(const FDSP_MsgHdrTypePtr& msg_hdr, 
		       const FDSP_Node_Info_TypePtr& node_info,
		       const Ice::Current&);

    void NotifyNodeRmv(const FDSP_MsgHdrTypePtr& msg_hdr, 
		       const FDSP_Node_Info_TypePtr& node_info,
		       const Ice::Current&);

    void NotifyDLTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
			 const FDSP_DLT_TypePtr& dlt_info,
			 const Ice::Current&);

    void NotifyDMTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
			 const FDSP_DMT_TypePtr& dmt_info,
			 const Ice::Current&);

    void SetThrottleLevel(const FDSP_MsgHdrTypePtr& msg_hdr, 
					const FDSP_ThrottleMsgTypePtr& throttle_msg, 
					    const Ice::Current&);

    void TierPolicy(const FDSP_TierPolicyPtr &tier, const Ice::Current &);
    void TierPolicyAudit(const FDSP_TierPolicyAuditPtr &audit,
                         const Ice::Current &);
  };

}


#endif
