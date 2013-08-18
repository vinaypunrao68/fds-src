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
			     const FDS_PubSub_Node_Info_TypePtr& node_info,
			     const Ice::Current&) {

    printf("Received Node Add Event for tenant %d domain %d : %d; %x\n", msg_hdr->tennant_id, msg_hdr->domain_id, node_info->node_id, (unsigned int) node_info->node_ip);
    om_client->recvNodeEvent(node_info->node_id, (unsigned int) node_info->node_ip, node_info->node_state); 

  }
  virtual void NotifyNodeRmv(const FDS_PubSub_MsgHdrTypePtr& msg_hdr,
			     const FDS_PubSub_Node_Info_TypePtr& node_info,
			     const Ice::Current&) {
    printf("Received Node Rmv Event : %d\n", node_info->node_id);
    om_client->recvNodeEvent(node_info->node_id, (unsigned int) node_info->node_ip, node_info->node_state); 

  }

  virtual void NotifyDLTUpdate(const FDS_PubSub_MsgHdrTypePtr& msg_dr,
			       const FDS_PubSub_DLT_TypePtr& dlt_info,
			       const Ice::Current&) {
    printf("Received dlt update: %d\n", dlt_info->DLT_version);
    om_client->recvDLTUpdate(dlt_info->DLT_version, dlt_info->DLT);
  }
  virtual void NotifyDMTUpdate(const FDS_PubSub_MsgHdrTypePtr& msg_dr,
			       const FDS_PubSub_DMT_TypePtr& dmt_info,
			       const Ice::Current&) {
    printf("Received dmt update: %d\n", dmt_info->DMT_version);
    om_client->recvDMTUpdate(dmt_info->DMT_version, dmt_info->DMT);
  }


};


int OMgrClient::initialize() {
  return fds::ERR_OK;
}

int OMgrClient::registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr) {
  this->node_evt_hdlr = node_event_hdlr;
  return 0;
}
         
int OMgrClient::subscribeToOmEvents(unsigned int om_ip_addr, int tenn_id, int dom_id) {

  int argc = 0;
  char **argv = 0;

  this->tennant_id = tenn_id;
  this->domain_id = dom_id;

  comm = Ice::initialize(argc, argv);

  Ice::ObjectPrx obj = comm->stringToProxy("IceStorm/TopicManager:tcp -h 10.1.10.202 -p 11234");
  topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);

  adapter = comm->createObjectAdapterWithEndpoints("OMgrSubscriberAdapter", "tcp");

  om_client_i = new OMgr_SubscriberI(this);

  proxy = adapter->addWithUUID(om_client_i)->ice_oneway();

  try {
    topic = topicManager->retrieve("OMgrEvents");
    IceStorm::QoS qos;
    topic->subscribeAndGetPublisher(qos, proxy);
  }
  catch (const IceStorm::NoSuchTopic&) {
    // Error! No topic found!
    cout  << "Topic not found" << endl;
    return 0;
  }

  adapter->activate();

  return 0;
}    

int OMgrClient::recvNodeEvent(int node_id, unsigned int node_ip, int node_state) {
  
  node_info_t& node = node_map[node_id];

  node.node_id = node_id;
  node.node_ip_address = node_ip;
  node.node_state = (FDS_PubSub_NodeState) node_state;

  if (this->node_evt_hdlr) {
    this->node_evt_hdlr(node_id, node_ip, node_state);
  }
  return (0);
  
}

int OMgrClient::recvDLTUpdate(int dlt_vrsn, const Node_Table_Type& dlt_table) {
  printf("New DLT : num shards - %lu\n", dlt_table.size());
  this->dlt_version = dlt_vrsn;
  this->dlt = dlt_table;
  return (0);
}

int OMgrClient::recvDMTUpdate(int dmt_vrsn, const Node_Table_Type& dmt_table) {
  printf("New DMT : num shards - %lu\n", dmt_table.size());
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
