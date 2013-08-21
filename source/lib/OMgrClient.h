#ifndef _OMGRCLIENT_H
#define _OMGRCLIENT_H
#include "include/fds_err.h"
#include "fds_pubsub.h"
#include <unordered_map>
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>

using namespace FDSP_Types;
using namespace FDS_PubSub_Interface;

typedef void (*node_event_handler_t)(int node_id, unsigned int node_ip_addr, int node_state);

typedef struct _node_info_t {

  int node_id;
  unsigned int node_ip_address;
  FDSP_NodeState node_state;

} node_info_t;

typedef std::unordered_map<int,node_info_t> node_map_t; 

namespace fds {

  class OMgrClient {

  private:
    int tennant_id;
    int domain_id;
    node_map_t node_map;
    int dlt_version;
    Node_Table_Type dlt;
    int dmt_version;
    Node_Table_Type dmt;
    

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

    int recvNodeEvent(int node_id, unsigned int node_ip, int node_state);
    int recvDLTUpdate(int dlt_version, const Node_Table_Type& dlt_table);
    int recvDMTUpdate(int dmt_version, const Node_Table_Type& dmt_table);

  };

}


#endif
