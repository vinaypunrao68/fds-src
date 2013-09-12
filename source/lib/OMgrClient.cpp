#include "OMgrClient.h"

using namespace std;
using namespace fds;

class OMgr_SubscriberI : virtual public FDS_OMgr_Subscriber {
public:

  OMgrClient *om_client;

  OMgr_SubscriberI(OMgrClient *omc) {
    this->om_client = omc;
  }

  virtual void NotifyNodeAdd(const FDS_PubSub_MsgHdrTypePtr& msg_hdr,
			     const FDSP_Node_Info_TypePtr& node_info,
			     const Ice::Current&) {

    //printf("Received Node Add Event for tenant %d domain %d : %d; %x\n", msg_hdr->tennant_id, msg_hdr->domain_id, node_info->node_id, (unsigned int) node_info->node_ip);
    om_client->recvNodeEvent(node_info->node_id, (unsigned int) node_info->node_ip, node_info->node_state); 

  }
  virtual void NotifyNodeRmv(const FDS_PubSub_MsgHdrTypePtr& msg_hdr,
			     const FDSP_Node_Info_TypePtr& node_info,
			     const Ice::Current&) {
    //printf("Received Node Rmv Event : %d\n", node_info->node_id);
    om_client->recvNodeEvent(node_info->node_id, (unsigned int) node_info->node_ip, node_info->node_state); 

  }

  virtual void NotifyDLTUpdate(const FDS_PubSub_MsgHdrTypePtr& msg_dr,
			       const FDSP_DLT_TypePtr& dlt_info,
			       const Ice::Current&) {
    //printf("Received dlt update: %d\n", dlt_info->DLT_version);
    om_client->recvDLTUpdate(dlt_info->DLT_version, dlt_info->DLT);
  }
  virtual void NotifyDMTUpdate(const FDS_PubSub_MsgHdrTypePtr& msg_dr,
			       const FDSP_DMT_TypePtr& dmt_info,
			       const Ice::Current&) {
    //printf("Received dmt update: %d\n", dmt_info->DMT_version);
    om_client->recvDMTUpdate(dmt_info->DMT_version, dmt_info->DMT);
  }


};

OMgrClientRPCI::OMgrClientRPCI(OMgrClient *omc) {
    this->om_client = omc;
}

void OMgrClientRPCI::NotifyAddVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
			       const Ice::Current&) {


  om_client->recvNotifyVol(msg_hdr->glob_volume_id, NULL, FDS_VOL_ACTION_CREATE);

}

void OMgrClientRPCI::NotifyRmVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
			       const Ice::Current&) {


  om_client->recvNotifyVol(msg_hdr->glob_volume_id, NULL, FDS_VOL_ACTION_DELETE);

}

      
void OMgrClientRPCI::AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
			       const Ice::Current&) {
  om_client->recvVolAttachState(msg_hdr->glob_volume_id, NULL, FDS_VOL_ACTION_ATTACH);
}


void OMgrClientRPCI::DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
			       const Ice::Current&) {
  om_client->recvVolAttachState(msg_hdr->glob_volume_id, NULL, FDS_VOL_ACTION_DETACH);
}


OMgrClient::OMgrClient(FDSP_MgrIdType node_type, fds_log *parent_log) {
  if (parent_log) {
    omc_log = parent_log;
  }
  else {
    omc_log = new fds_log("omc", "logs");
  }
  my_node_type = node_type;
}

OMgrClient::OMgrClient() {
  my_node_type = FDSP_STOR_HVISOR;
  omc_log = new fds_log("omc", "logs");
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
        
int OMgrClient::subscribeToOmEvents(unsigned int om_ip_addr, int tenn_id, int dom_id, int omc_port_num) {

  int argc = 0;
  char **argv = 0;

  this->tennant_id = tenn_id;
  this->domain_id = dom_id;

  // Step 1: Load properties from config file om_client.conf

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  // Step 2: Contact pubsub mgr and subscribe to "OmgrEvents" with an endpoint that can accept messages published by OM

  pubsub_comm = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr pubsub_props = pubsub_comm->getProperties();
  std::string omgr_ip_addr = pubsub_props->getProperty("OMgr.IP");
 
  std::string tcpProxyStr;
  tcpProxyStr = std::string("IceStorm/TopicManager:tcp -h ") + omgr_ip_addr + std::string(" -p 11234");

  // Ice::ObjectPrx obj = pubsub_comm->stringToProxy("IceStorm/TopicManager:tcp -h 10.1.10.201 -p 11234");
  Ice::ObjectPrx obj = pubsub_comm->stringToProxy(tcpProxyStr);
  pubsub_topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);
  pubsub_adapter = pubsub_comm->createObjectAdapterWithEndpoints("OMgrSubscriberAdapter", "tcp");
  om_pubsub_client_i = new OMgr_SubscriberI(this);
  pubsub_proxy = pubsub_adapter->addWithUUID(om_pubsub_client_i)->ice_oneway();

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

  // Step 3:  Setup the (receiving side) endpoint to lister for control path requests from OM.

  rpc_comm = Ice::initialize(argc, argv, initData);

  Ice::PropertiesPtr rpc_props = rpc_comm->getProperties();

  /*
   * Set basic thread properties.
   */
  rpc_props->setProperty("OMgrClient.ThreadPool.Size", "1");
  rpc_props->setProperty("OMgrClient.ThreadPool.SizeMax", "1");
  rpc_props->setProperty("OMgrClient.ThreadPool.SizeWarn", "1");

  if (omc_port_num == 0) {
    /*
     * Pull the port from the config file if it wasn't
     * specified on the command line.
     */
    omc_port_num = rpc_props->getPropertyAsInt("OMgrClient.PortNumber");
  }

  FDS_PLOG(omc_log) << "Orch mgr client using port - " << omc_port_num;
 
  tcpProxyStr = std::string("tcp -p ") + std::to_string(omc_port_num);
  Ice::ObjectAdapterPtr rpc_adapter =rpc_comm->createObjectAdapterWithEndpoints("OrchMgrClient", tcpProxyStr);

  om_client_rpc_i = new OMgrClientRPCI(this);
  rpc_adapter->add(om_client_rpc_i, rpc_comm->stringToIdentity("OrchMgrClient"));

  rpc_adapter->activate();

  // Step 4: Finally(!), we can now register with OM.

  registerNodeWithOM(rpc_comm);

  return 0;
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

int OMgrClient::registerNodeWithOM(Ice::CommunicatorPtr& comm) {

  Ice::PropertiesPtr props = comm->getProperties();
  std::string omgr_ip_addr = props->getProperty("OMgr.IP");
  std::string omgr_config_port = props->getProperty("OMgr.ConfigPort");
  std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") +  omgr_ip_addr + std::string(" -p ") + omgr_config_port;
  FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(comm->stringToProxy(tcpProxyStr));
  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  initOMMsgHdr(msg_hdr);
  FDSP_RegisterNodeTypePtr reg_node_msg = new FDSP_RegisterNodeType;
  reg_node_msg->node_type = my_node_type;
  reg_node_msg->ip_hi_addr = 0;
  reg_node_msg->ip_lo_addr = props->getPropertyAsInt("OmgrClient.MyIPAddr"); //my_address!
  reg_node_msg->control_port = props->getPropertyAsInt("OmgrClient.PortNumber");
  reg_node_msg->data_port = 0; // for now
  fdspConfigPathAPI->RegisterNode(msg_hdr, reg_node_msg);
  return (0);
}

int OMgrClient::recvNodeEvent(int node_id, unsigned int node_ip, int node_state) {
  
  node_info_t& node = node_map[node_id];

  node.node_id = node_id;
  node.node_ip_address = node_ip;
  node.node_state = (FDSP_NodeState) node_state;

  if (this->node_evt_hdlr) {
    this->node_evt_hdlr(node_id, node_ip, node_state);
  }
  return (0);
  
}

int OMgrClient::recvNotifyVol(fds_volid_t vol_id, VolumeDesc *vdb, int vol_action) {

  if (this->vol_evt_hdlr) {
    this->vol_evt_hdlr(vol_id, vdb, vol_action);
  }
  return (0);
  
}

int OMgrClient::recvVolAttachState(fds_volid_t vol_id, VolumeDesc *vdb, int vol_action) {

  if (this->vol_evt_hdlr) {
    this->vol_evt_hdlr(vol_id, vdb, vol_action);
  }
  return (0);
  
}


int OMgrClient::recvDLTUpdate(int dlt_vrsn, const Node_Table_Type& dlt_table) {
  /*
   * TODO: Move these prints to an OM logger.
   */
  // printf("New DLT : num shards - %u\n", dlt_table.size());
  this->dlt_version = dlt_vrsn;
  this->dlt = dlt_table;
  return (0);
}

int OMgrClient::recvDMTUpdate(int dmt_vrsn, const Node_Table_Type& dmt_table) {
  /*
   * TODO: Move these prints to an OM logger.
   */
  // printf("New DMT : num shards - %u\n", dmt_table.size());
  this->dmt_version = dmt_vrsn;
  this->dmt = dmt_table;
  return (0);
}

int OMgrClient::getNodeInfo(int node_id, unsigned int *node_ip_addr, int *node_state) {
  
  node_info_t& node = node_map[node_id];

  if (node.node_id != node_id) {
    return (-1);
  }

  *node_ip_addr = node.node_ip_address;
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
