#ifndef _OMGRCLIENT_H
#define _OMGRCLIENT_H
#include "fds_err.h"
#include "fds_pubsub.h"
#include <unordered_map>
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>

using namespace FDS_PubSub_Interface;

typedef void (*node_event_handler_t)(int node_id, unsigned int node_ip_addr, int node_state);

typedef struct _node_info_t {

  int node_id;
  unsigned int node_ip_address;
  FDS_PubSub_NodeState node_state;

} node_info_t;

typedef std::unordered_map<int,node_info_t> node_map_t; 

namespace fds {

  class OMgrClient {

  private:
    int tennant_id;
    int domain_id;
    node_map_t node_map;

    node_event_handler_t node_evt_hdlr;

    Ice::CommunicatorPtr comm;
    IceStorm::TopicManagerPrx topicManager;
    Ice::ObjectAdapterPtr adapter;
    FDS_OMgr_SubscriberPtr om_client_i;
    Ice::ObjectPrx proxy;
    IceStorm::TopicPrx topic;


  public:

    int initialize();
    int registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr);
    int subscribeToOmEvents(unsigned int om_ip_addr, int tennant_id, int domain_id);
    int getNodeInfo(int node_id, unsigned int *node_ip_addr, int *node_state);
    int getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes);
    int getDMTNodesForVolume(int vol_id, int *node_ids, int *n_nodes);

  };

}


#endif
