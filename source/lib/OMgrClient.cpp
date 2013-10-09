#include "OMgrClient.h"

using namespace std;
using namespace fds;


OMgrClientRPCI::OMgrClientRPCI(OMgrClient *omc) {
    this->om_client = omc;
}

void OMgrClientRPCI::NotifyAddVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                                  const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
                                  const Ice::Current&) {
  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL);
  fds_vol_notify_t type = fds_notify_vol_add;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc->volUUID, vdb, type);

}

void OMgrClientRPCI::NotifyRmVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
			       const Ice::Current&) {

  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_RM_VOL);
  fds_vol_notify_t type = fds_notify_vol_rm;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc->volUUID, vdb, type);

}
      
void OMgrClientRPCI::AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
			       const Ice::Current&) {
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvVolAttachState(vol_msg->vol_desc->volUUID, vdb, FDS_VOL_ACTION_ATTACH);
}


void OMgrClientRPCI::DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
			       const Ice::Current&) {
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvVolAttachState(vol_msg->vol_desc->volUUID, vdb, FDS_VOL_ACTION_DETACH);
}

void OMgrClientRPCI::NotifyNodeAdd(const FDSP_MsgHdrTypePtr& msg_hdr, 
				   const FDSP_Node_Info_TypePtr& node_info,
				   const Ice::Current&){
  om_client->recvNodeEvent(node_info->node_id, node_info->node_type, (unsigned int) node_info->ip_lo_addr, node_info->node_state, node_info);
}

void OMgrClientRPCI::NotifyNodeRmv(const FDSP_MsgHdrTypePtr& msg_hdr, 
				   const FDSP_Node_Info_TypePtr& node_info,
				   const Ice::Current&){
  om_client->recvNodeEvent(node_info->node_id, node_info->node_type, (unsigned int) node_info->ip_lo_addr, node_info->node_state, node_info); 
}

void OMgrClientRPCI::NotifyDLTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
				     const FDSP_DLT_TypePtr& dlt_info,
				     const Ice::Current&){
  om_client->recvDLTUpdate(dlt_info->DLT_version, dlt_info->DLT);
}

void OMgrClientRPCI::NotifyDMTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
				     const FDSP_DMT_TypePtr& dmt_info,
				     const Ice::Current&){
  om_client->recvDMTUpdate(dmt_info->DMT_version, dmt_info->DMT);
}


OMgrClient::OMgrClient(FDSP_MgrIdType node_type,
                       const std::string& _omIpStr,
                       fds_uint32_t data_port,
                       const std::string& node_name,
                       fds_log *parent_log) {
  my_node_type = node_type;
  omIpStr      = _omIpStr;
  my_data_port = data_port;
  my_node_name = node_name;
  if (parent_log) {
    omc_log = parent_log;
  } else {
    omc_log = new fds_log("omc", "logs");
  }

  initRPCComm();
}

OMgrClient::OMgrClient() {
  my_node_type = FDSP_STOR_HVISOR;
  my_node_name = "localhost-sh";
  omc_log = new fds_log("omc", "logs");
  initRPCComm();
}

int OMgrClient::initialize() {
  return fds::ERR_OK;
}

int OMgrClient::registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr) {
  this->node_evt_hdlr = node_event_hdlr;
  return 0;
}
 

int OMgrClient::registerEventHandlerForVolEvents(volume_event_handler_t vol_event_hdlr) {
  this->vol_evt_hdlr = vol_event_hdlr;
  return 0;
}

int OMgrClient::initRPCComm() {

  int argc = 0;
  char **argv = 0;

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  rpc_comm = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr rpc_props = rpc_comm->getProperties();

  /*
   * Set basic thread properties.
   */
  rpc_props->setProperty("OMgrClient.ThreadPool.Size", "1");
  rpc_props->setProperty("OMgrClient.ThreadPool.SizeMax", "1");
  rpc_props->setProperty("OMgrClient.ThreadPool.SizeWarn", "1");

  /*
   * If the OM's IP wasn't given via cmdline
   * pull it from the config file.
   */
  if (omIpStr.empty() == true) {
    omIpStr = rpc_props->getProperty("OMgr.IP");
  }
  
  return (0);

}  

#if 0

int OMgrClient::subscribeToOmEvents(unsigned int om_ip_addr, int tenn_id, int dom_id, int omc_port_num) {

  int argc = 0;
  char **argv = 0;

  this->tennant_id = tenn_id;
  this->domain_id = dom_id;

  try {

  // Load properties from config file om_client.conf

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  // Step 2: Contact pubsub server and get handle to topic mgr 

  pubsub_comm = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr pubsub_props = pubsub_comm->getProperties();
 
  std::string tcpProxyStr;
  tcpProxyStr = std::string("IceStorm/TopicManager:tcp -h ") + omIpStr + std::string(" -p 11234");
  Ice::ObjectPrx obj = pubsub_comm->stringToProxy(tcpProxyStr);
  pubsub_topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);


  // Step 3: Create an ednpoint to accept published messages

  pubsub_adapter = pubsub_comm->createObjectAdapterWithEndpoints("OMgrSubscriberAdapter", "tcp");
  om_pubsub_client_i = new OMgr_SubscriberI(this);
  pubsub_proxy = pubsub_adapter->addWithUUID(om_pubsub_client_i)->ice_oneway();


  // Step 4: Subscribe to topic "OMgrEvents" with subscription endpoint

  try {
    pubsub_topic = pubsub_topicManager->retrieve("OMgrEvents");
    IceStorm::QoS qos;
    pubsub_topic->subscribeAndGetPublisher(qos, pubsub_proxy);
  }
  catch (const IceStorm::NoSuchTopic&) {
    // Error! No topic found!
    cout  << "Topic not found" << endl;
    return 0;
  }

  pubsub_adapter->activate();

  FDS_PLOG(omc_log) << "OMgrClient subscribed to OMgrEvents with Pub Sub server at "
                    << omIpStr << " : 11234";

  }
  catch (...) {
    FDS_PLOG(omc_log) << "OMgrClient could not subscribe to OMgrEvents. Please check if pubsub server is up and restart";
  }

 
  return 0;
}    

#endif

// Call this to setup the (receiving side) endpoint to lister for control path requests from OM.
int OMgrClient::startAcceptingControlMessages() {
  return startAcceptingControlMessages(0);
}

int OMgrClient::startAcceptingControlMessages(fds_uint32_t port_num) {

  Ice::PropertiesPtr rpc_props = rpc_comm->getProperties();
  
  my_control_port = port_num;
  if (my_control_port == 0) {
    my_control_port = rpc_props->getPropertyAsInt("OMgrClient.PortNumber");
  }
 
  std::string tcpProxyStr = std::string("tcp -p ") + std::to_string(my_control_port);
  Ice::ObjectAdapterPtr rpc_adapter =rpc_comm->createObjectAdapterWithEndpoints("OrchMgrClient", tcpProxyStr);

  om_client_rpc_i = new OMgrClientRPCI(this);
  rpc_adapter->add(om_client_rpc_i, rpc_comm->stringToIdentity("OrchMgrClient"));

  rpc_adapter->activate();

  FDS_PLOG(omc_log) << "OMClient accepting control requests at port " << my_control_port;

  return (0);

}

void OMgrClient::initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
	msg_hdr->minor_ver = 0;
	msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
        msg_hdr->msg_id =  1;

        msg_hdr->major_ver = 0xa5;
        msg_hdr->minor_ver = 0x5a;

        msg_hdr->num_objects = 1;
        msg_hdr->frag_len = 0;
        msg_hdr->frag_num = 0;

        msg_hdr->tennant_id = 0;
        msg_hdr->local_domain_id = 0;

        msg_hdr->src_id = my_node_type;
        msg_hdr->dst_id = FDSP_ORCH_MGR;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


}

// Use this to register the local node with OM as a client. Should be called after calling starting subscription endpoint and control path endpoint.
int OMgrClient::registerNodeWithOM() {

  try {

  Ice::PropertiesPtr props = rpc_comm->getProperties();
  std::string omgr_config_port = props->getProperty("OMgr.ConfigPort");
  std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") +  omIpStr + std::string(" -p ") + omgr_config_port;
  FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  initOMMsgHdr(msg_hdr);
  FDSP_RegisterNodeTypePtr reg_node_msg = new FDSP_RegisterNodeType;
  reg_node_msg->node_type = my_node_type;
  reg_node_msg->node_name = my_node_name;
  reg_node_msg->ip_hi_addr = 0;
  reg_node_msg->ip_lo_addr = fds::str_to_ipv4_addr(props->getProperty("OMgrClient.MyIPAddr")); //my_address!
  reg_node_msg->control_port = my_control_port;
  reg_node_msg->data_port = my_data_port; // for now

  FDS_PLOG(omc_log) << "OMClient registering local node " << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr) << " control port:" << reg_node_msg->control_port 
		    << " data port:" << reg_node_msg->data_port
		    << " with Orchaestration Manager at " << tcpProxyStr;

  fdspConfigPathAPI->begin_RegisterNode(msg_hdr, reg_node_msg);

  FDS_PLOG(omc_log) << "OMClient completed node registration with OM";

  }
  catch(...) {
    FDS_PLOG(omc_log) << "OMClient unable to register node with OrchMgr. Please check if OrchMgr is up and restart.";
  }

  return (0);
}

int OMgrClient::recvNodeEvent(int node_id, FDSP_MgrIdType node_type, unsigned int node_ip, int node_state, const FDSP_Node_Info_TypePtr& node_info) {
  
  node_info_t& node = node_map[node_id];

  node.node_id = node_id;
  node.node_ip_address = node_ip;
  node.port = node_info->data_port;
  node.node_state = (FDSP_NodeState) node_state;

  FDS_PLOG(omc_log) << "OMClient received node event for node " << node_id << ", type - " << node_info->node_type << " with ip address " << node_ip << " and state - " << node_state;

  if (this->node_evt_hdlr) {
    this->node_evt_hdlr(node_id,
                        node_ip,
                        node_state,
                        node_info->data_port,
                        node_info->node_type);
  }
  return (0);
  
}

int OMgrClient::recvNotifyVol(fds_volid_t vol_id,
                              VolumeDesc *vdb,
                              fds_vol_notify_t vol_action) {

  FDS_PLOG(omc_log) << "OMClient received volume event for volume " << vol_id << " action - " << vol_action;

  if (this->vol_evt_hdlr) {
    this->vol_evt_hdlr(vol_id, vdb, vol_action);
  }
  return (0);
  
}

int OMgrClient::recvVolAttachState(fds_volid_t vol_id,
                                   VolumeDesc *vdb,
                                   int vol_action) {

  assert((vol_action == FDS_VOL_ACTION_ATTACH) || (vol_action == FDS_VOL_ACTION_DETACH));

  fds_vol_notify_t type = fds_notify_vol_attatch;
  if (vol_action == FDS_VOL_ACTION_DETACH) {
    type = fds_notify_vol_detach;
  }

  FDS_PLOG(omc_log) << "OMClient received volume attach/detach request for volume " << vol_id << " action - " << type;

  if (this->vol_evt_hdlr) {
    this->vol_evt_hdlr(vol_id, vdb, type);
  }
  return (0);
  
}


int OMgrClient::recvDLTUpdate(int dlt_vrsn, const Node_Table_Type& dlt_table) {

  FDS_PLOG(omc_log) << "OMClient received new DLT version  " << dlt_vrsn;

  this->dlt_version = dlt_vrsn;
  this->dlt = dlt_table;
  return (0);
}

int OMgrClient::recvDMTUpdate(int dmt_vrsn, const Node_Table_Type& dmt_table) {

  FDS_PLOG(omc_log) << "OMClient received new DMT version  " << dmt_vrsn;

  this->dmt_version = dmt_vrsn;
  this->dmt = dmt_table;
  return (0);
}

int OMgrClient::getNodeInfo(int node_id,
                            unsigned int *node_ip_addr,
                            fds_uint32_t *node_port,
                            int *node_state) {
  
  node_info_t& node = node_map[node_id];

  if (node.node_id != node_id) {
    return (-1);
  }

  *node_ip_addr = node.node_ip_address;
  *node_port = node.port;
  *node_state = node.node_state;
  
  return 0;
} 

int OMgrClient::getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes) {
  
  int total_shards = this->dlt.size();
  int lookup_key = doid_key % total_shards;
  int total_nodes = this->dlt[lookup_key].size();
  *n_nodes = (total_nodes < *n_nodes)? total_nodes:*n_nodes;
  int i;
  
  for (i = 0; i < *n_nodes; i++) {
    node_ids[i] = this->dlt[lookup_key][i];
  } 

  return 0;
}

int OMgrClient::getDMTNodesForVolume(int vol_id, int *node_ids, int *n_nodes) {

  int total_shards = this->dmt.size();
  int lookup_key = vol_id % total_shards;
  int total_nodes = this->dmt[lookup_key].size();
  *n_nodes = (total_nodes < *n_nodes)? total_nodes:*n_nodes;
  int i;
  
  for (i = 0; i < *n_nodes; i++) {
    node_ids[i] = this->dmt[lookup_key][i];
  } 
  return 0;
}
