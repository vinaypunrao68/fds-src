#include "OMgrClient.h"

using namespace std;
using namespace fds;

class OMgr_SubscriberI : virtual public FDS_OMgr_Subscriber {
public:

  node_event_handler_t node_evt_hdlr;

  virtual void NotifyNodeAdd(const FDS_PubSub_MsgHdrTypePtr& msg_hdr,
			     const FDS_PubSub_Node_Info_TypePtr& node_info,
			     const Ice::Current&) {

    printf("Received Node Add Event for tenant %d domain %d : %d; %x\n", msg_hdr->tennant_id, msg_hdr->domain_id, node_info->node_id, (unsigned int) node_info->node_ip);
    if (node_evt_hdlr) {
      node_evt_hdlr(node_info->node_id, (unsigned int) node_info->node_ip, node_info->node_state); 
    }

  }
  virtual void NotifyNodeRmv(const FDS_PubSub_MsgHdrTypePtr& msg_hdr,
			     const FDS_PubSub_Node_Info_TypePtr& node_info,
			     const Ice::Current&) {
    printf("Received Node Rmv Event : %d\n", node_info->node_id);
    if (node_evt_hdlr) {
      node_evt_hdlr(node_info->node_id, (unsigned int) node_info->node_ip, node_info->node_state); 
    }
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

  OMgr_SubscriberI *om_client = new OMgr_SubscriberI;
  om_client->node_evt_hdlr = this->node_evt_hdlr;
  om_client_i = om_client;

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

int OMgrClient::getNodeInfo(int node_id, unsigned int *node_ip_addr, int *node_state) {
  if (node_id == 1) {
    *node_ip_addr = (unsigned int) 0x0a010ac9;
    *node_state = FDS_Node_Up;
  } else if (node_id == 2) {
    *node_ip_addr = (unsigned int) 0x0a010aca;
    *node_state = FDS_Node_Up;
  } else {
    return (-1);
  }
  return 0;
} 

int OMgrClient::getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes) {
  node_ids[0] = 1;
  node_ids[1] = 2;
  *n_nodes = 2;
  return 0;
}

int OMgrClient::getDMTNodesForVolume(int vol_id, int *node_ids, int *n_nodes) {
  node_ids[0] = 1;
  node_ids[1] = 2;
  *n_nodes = 2;
  return 0;
}
