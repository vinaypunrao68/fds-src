#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <fds_pubsub.h>
#include <stdio.h>

#define CMD_NOOP 0
#define CMD_NODE_UP 1
#define CMD_NODE_DOWN 2
#define CMD_DLT_UPDATE 3
#define CMD_DMT_UPDATE 4

using namespace FDS_PubSub_Interface; 
using namespace std;

void construct_node_table_from_file(char *file_name, Node_Table_Type& node_table);

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
    char tmp_file_name[32];
    int dlt_version = 0;
    int dmt_version = 0;
    size_t n_bytes;

    while(1) {
      cmd = CMD_NOOP;
      printf(">");
      if (getline(&line_ptr, &n_bytes, stdin) <= 1) {
	continue;
      }
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
      }	else if (strcmp(cmd_wd, "dlt") == 0) {
	cmd = CMD_DLT_UPDATE;
	sscanf(line_ptr, "dlt %s", tmp_file_name);
      }	else if (strcmp(cmd_wd, "dmt") == 0) {
	cmd = CMD_DMT_UPDATE;
	sscanf(line_ptr, "dmt %s", tmp_file_name);
      }else {
	printf("Unknown command %s. Please try again\n", cmd_wd);
	continue;
      }
      
      try {
	
	FDS_PubSub_Interface::FDS_PubSub_MsgHdrTypePtr msg_hdr_ptr = new FDS_PubSub_MsgHdrType;
	FDS_PubSub_Interface::FDS_PubSub_Node_Info_TypePtr node_info_ptr = new FDS_PubSub_Node_Info_Type;
	FDS_PubSub_Interface::FDS_PubSub_DLT_TypePtr dlt_info_ptr = new FDS_PubSub_DLT_Type;
	FDS_PubSub_Interface::FDS_PubSub_DMT_TypePtr dmt_info_ptr = new FDS_PubSub_DMT_Type;
	

	msg_hdr_ptr->msg_code = FDS_Node_Add;
	msg_hdr_ptr->msg_id = 0;
	msg_hdr_ptr->tennant_id = 1;
	msg_hdr_ptr->domain_id = 1;

	switch(cmd) {

	case CMD_NODE_UP:
	  node_info_ptr->node_id = node_id;
	  node_info_ptr->node_state = FDS_Node_Up;
	  node_info_ptr->node_ip = node_ip;
	  om_client->NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
	  break;

	case CMD_NODE_DOWN:
	  node_info_ptr->node_id = node_id;
	  node_info_ptr->node_ip = 0;
	  node_info_ptr->node_state = FDS_Node_Rmvd;
	  om_client->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
	  break;

	case CMD_DLT_UPDATE:
	  {
	    dlt_info_ptr->DLT_version = dlt_version++;
	    construct_node_table_from_file(tmp_file_name, dlt_info_ptr->DLT);
	    printf("DLT: %lu shards\n", dlt_info_ptr->DLT.size());
	    om_client->NotifyDLTUpdate(msg_hdr_ptr, dlt_info_ptr);
	    break;
	  }

	case CMD_DMT_UPDATE:
	  {
	    dmt_info_ptr->DMT_version = dmt_version++;
	    construct_node_table_from_file(tmp_file_name, dmt_info_ptr->DMT);
	    printf("DMT: %lu shards\n", dmt_info_ptr->DMT.size());
	    om_client->NotifyDMTUpdate(msg_hdr_ptr, dmt_info_ptr);
	    break;
	  }

	default:
	  break;
      }

      }
      catch (const Ice::Exception& ex) {
	cerr << ex << endl;
      }

    }

}


void construct_node_table_from_file(char *file_name, Node_Table_Type& node_table) {

  FILE *fp;
  size_t n_bytes = 0;
  char *line_ptr = 0; 
  
  printf("Table size- %lu\n", node_table.size());

  fp = fopen(file_name, "r");
  
  while(!feof(fp)) {
    int n_nodes = 0;
    char *curr_ptr = 0;
    int bytes_read = 0;
    int i;
    Node_List_Type node_vect;

    getline(&line_ptr, &n_bytes, fp);
    sscanf(line_ptr, "%d%n", &n_nodes, &bytes_read);
    if (n_nodes == 0) {
      continue;
    }
    curr_ptr = line_ptr + bytes_read;
    node_vect.reserve(n_nodes);

    for (i = 0; i < n_nodes; i++) {
      int  node_id;
      sscanf(curr_ptr, "%d%n", &node_id, &bytes_read);
      curr_ptr += bytes_read;
      node_vect.push_back(node_id);
    }
    
    node_table.push_back(node_vect);

    printf("Table size- %lu, line - %s\n", node_table.size(), line_ptr);
    line_ptr[0] = 0;

  }

  fclose(fp);
  
}
