#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <fds_pubsub.h>
#include <stdio.h>

#define CMD_NOOP 0
#define CMD_NODE_UP 1
#define CMD_NODE_DOWN 2

using namespace FDS_PubSub_Interface; 
using namespace std;

int main(int argc, char* argv[])
{

  Ice::CommunicatorPtr comm;
  comm = Ice::initialize(argc, argv);

    Ice::ObjectPrx obj = comm->stringToProxy(
        "IceStorm/TopicManager:tcp -p 11234");
    IceStorm::TopicManagerPrx topicManager =
        IceStorm::TopicManagerPrx::checkedCast(obj);
    IceStorm::TopicPrx topic;
    try {
        topic = topicManager->retrieve("OMgrEvents");
    }
    catch (const IceStorm::NoSuchTopic&) {
        topic = topicManager->create("OMgrEvents");
    }

    Ice::ObjectPrx pub = topic->getPublisher()->ice_oneway();
    FDS_PubSub_Interface::FDS_OMgr_SubscriberPrx om_client = FDS_PubSub_Interface::FDS_OMgr_SubscriberPrx::uncheckedCast(pub);

    int node_id;
    char *line_ptr = 0;
    int cmd;
    unsigned int node_ip;
    int node_ip_bytes[4];
    char cmd_wd[16];
    size_t n_bytes;

    while(1) {
      cmd = CMD_NOOP;
      printf(">");
      getline(&line_ptr, &n_bytes, stdin);
      sscanf(line_ptr, "%s", cmd_wd);
      if (strcmp(cmd_wd, "quit") == 0) {
	break;
      } else if (strcmp(cmd_wd, "up") == 0) {
	cmd = CMD_NODE_UP;
	sscanf(line_ptr, "up %d %d.%d.%d.%d", &node_id, &(node_ip_bytes[0]), &(node_ip_bytes[1]), &(node_ip_bytes[2]), &(node_ip_bytes[3]));
	node_ip = node_ip_bytes[0] << 24 | node_ip_bytes[1] << 16 | node_ip_bytes[2] << 8 | node_ip_bytes[3];
      } else if (strcmp(cmd_wd, "down") == 0) {
	cmd = CMD_NODE_DOWN;
	sscanf(line_ptr, "down %d", &node_id);
      }	else {
	printf("Unknown command %s. Please try again\n", cmd_wd);
	continue;
      }
      
      try {
	
	FDS_PubSub_Interface::FDS_PubSub_MsgHdrTypePtr msg_hdr_ptr = new FDS_PubSub_MsgHdrType;
	FDS_PubSub_Interface::FDS_PubSub_Node_Info_TypePtr node_info_ptr = new FDS_PubSub_Node_Info_Type;

	msg_hdr_ptr->msg_code = FDS_Node_Add;
	msg_hdr_ptr->msg_id = 0;

	node_info_ptr->node_id = node_id;
	if (cmd == CMD_NODE_UP) {
	  node_info_ptr->node_state = FDS_Node_Up;
	  node_info_ptr->node_ip = node_ip;
	  om_client->NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
	} else {
	  node_info_ptr->node_state = FDS_Node_Rmvd;
	  om_client->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
	}

      }
      catch (const Ice::Exception& ex) {
	cerr << ex << endl;
      }

    }

}
