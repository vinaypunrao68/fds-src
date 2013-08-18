#include <stdio.h>
#include "OMgrClient.h"

using namespace fds;

void my_node_event_handler(int node_id, unsigned int node_ip_addr, int node_state) {

  printf("Node id - %d Node ip - %x Node state - %d\n", node_id, node_ip_addr, node_state);

}

int main(int argc, char *argv[]) {

	OMgrClient *om_client;
	size_t n_bytes;
	char *line_ptr = 0;
	char cmd_wd[32];
	int input_field;

	om_client = new OMgrClient();

	om_client->initialize();
	om_client->registerEventHandlerForNodeEvents(my_node_event_handler);
	om_client->subscribeToOmEvents(0x0a010aca, 1, 1);

	while(1) {
	  printf(">");
	  getline(&line_ptr, &n_bytes, stdin);
	  sscanf(line_ptr, "%s", cmd_wd);
	  if (strcmp(cmd_wd, "quit") == 0) {

	    break;

	  } else if (strcmp(cmd_wd, "dltshow") == 0) {

	    int node_ids[8];
	    int n_nodes = 8;
	    int i;

	    sscanf(line_ptr, "%s %d", cmd_wd, &input_field);

	    om_client->getDLTNodesForDoidKey((unsigned char)input_field, node_ids, &n_nodes);
	    printf("DLT Nodes for key %d :\n", input_field);
	    for (i = 0; i < n_nodes; i++) {
	      unsigned int node_ip = 0;
	      int node_state = -1;
	      om_client->getNodeInfo(node_ids[i], &node_ip, &node_state);
	      printf("Node id - %d Node ip - %x Node state - %d\n", node_ids[i], node_ip, node_state);
	    }
	    printf("\n");

	  } else if (strcmp(cmd_wd, "dmtshow") == 0) {

	    int node_ids[8];
	    int n_nodes = 8;
	    int i;

	    sscanf(line_ptr, "%s %d", cmd_wd, &input_field);

	    om_client->getDMTNodesForVolume(input_field, node_ids, &n_nodes);
	    printf("DMT Nodes for volume %d :\n", input_field);
	    for (i = 0; i < n_nodes; i++) {
	      unsigned int node_ip = 0;
	      int node_state = -1;
	      om_client->getNodeInfo(node_ids[i], &node_ip, &node_state);
	      printf("Node id - %d Node ip - %x Node state - %d\n", node_ids[i], node_ip, node_state);
	    }
	    printf("\n");

	  }
	}
}

