#ifndef _OMGRCLIENT_H
#define _OMGRCLIENT_H
#include "include/fds_err.h"
#include "include/fds_volume.h"
#include "fdsp/fds_pubsub.h"
#include "fdsp/FDSP.h"
#include "util/Log.h"
#include <unordered_map>
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>

using namespace FDSP_Types;
using namespace FDS_PubSub_Interface;
using namespace FDS_ProtocolInterface;

#define FDS_VOL_ACTION_NONE   0
#define FDS_VOL_ACTION_CREATE 1
#define FDS_VOL_ACTION_MODIFY 2
#define FDS_VOL_ACTION_ATTACH 3
#define FDS_VOL_ACTION_DETACH 4
#define FDS_VOL_ACTION_DELETE 5

typedef struct _node_info_t {

  int node_id;
  unsigned int node_ip_address;
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
    MAX
  } fds_vol_notify_t;
  
  typedef void (*node_event_handler_t)(int node_id, unsigned int node_ip_addr, int node_state);
  typedef void (*volume_event_handler_t)(fds::fds_volid_t volume_id, fds::VolumeDesc *vdb, fds_vol_notify_t vol_action);


  class OMgrClient {

  private:
    int tennant_id;
    int domain_id;
    FDSP_MgrIdType my_node_type;
    node_map_t node_map;
    int dlt_version;
    Node_Table_Type dlt;
    int dmt_version;
    Node_Table_Type dmt;
    
    fds_log *omc_log;

    node_event_handler_t node_evt_hdlr;
    volume_event_handler_t vol_evt_hdlr;

    Ice::CommunicatorPtr pubsub_comm;
    IceStorm::TopicManagerPrx pubsub_topicManager;
    Ice::ObjectAdapterPtr pubsub_adapter;
    FDS_OMgr_SubscriberPtr om_pubsub_client_i;
    Ice::ObjectPrx pubsub_proxy;
    IceStorm::TopicPrx pubsub_topic;

    Ice::CommunicatorPtr rpc_comm;
    Ice::ObjectAdapterPtr rpc_adapter;
    FDS_ProtocolInterface::FDSP_ControlPathReqPtr om_client_rpc_i;
    void initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);
    int initRPCComm();


  public:

    OMgrClient();
    OMgrClient(FDSP_MgrIdType node_type, fds_log *parent_log);
    int initialize();
    int registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr);
    int registerEventHandlerForVolEvents(volume_event_handler_t vol_event_hdlr);
    int subscribeToOmEvents(unsigned int om_ip_addr, int tennant_id, int domain_id, int omc_port_num=0);
    int startAcceptingControlMessages();
    int startAcceptingControlMessages(fds_uint32_t port_num);
    int registerNodeWithOM();
    int getNodeInfo(int node_id, unsigned int *node_ip_addr, int *node_state);
    int getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes);
    int getDMTNodesForVolume(int vol_id, int *node_ids, int *n_nodes);

    int recvNodeEvent(int node_id, unsigned int node_ip, int node_state);
    int recvDLTUpdate(int dlt_version, const Node_Table_Type& dlt_table);
    int recvDMTUpdate(int dmt_version, const Node_Table_Type& dmt_table);

    int recvNotifyVol(fds_volid_t vol_id,
                      VolumeDesc *vdb,
                      fds_vol_notify_t vol_action);
    int recvVolAttachState(fds_volid_t vol_id, VolumeDesc *vdb, int vol_action);

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

      
    void AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
		   const Ice::Current&);
      
    void DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
		   const Ice::Current&);


  };

}


#endif
