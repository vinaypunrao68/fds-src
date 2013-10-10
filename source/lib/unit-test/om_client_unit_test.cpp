#include <stdio.h>
#include "OMgrClient.h"

using namespace fds;

void my_node_event_handler(int node_id,
                           unsigned int node_ip_addr,
                           int node_state,
                           fds_uint32_t port_num,
                           FDS_ProtocolInterface::FDSP_MgrIdType node_type) {

  printf("Recvd node event: Node id - %d Node ip - %x Node state - %d\n", node_id, node_ip_addr, node_state);

}

void my_vol_event_handler(fds::fds_volid_t volume_id, fds::VolumeDesc *vdb, fds_vol_notify_t vol_action) {
  
  printf("Recvd vol event: Vol id - %llu, action -%d\n", (unsigned long long) volume_id, vol_action);

}


int main(int argc, char *argv[]) {

	OMgrClient *om_client;
	size_t n_bytes;
	char *line_ptr = 0;
	char cmd_wd[32];
	int input_field;
	FDSP_MgrIdType node_type = FDSP_STOR_HVISOR;
        std::string omIpStr;
	int control_port = 0;
	std::string node_id = "localhost-sh";

	if (argc > 1) {
	  if (strcmp(argv[1], "-h") == 0) {
	    node_type = FDSP_STOR_HVISOR;
	    node_id = "localhost-sh";
	    control_port = 7902;
	  } else if (strcmp(argv[1], "-d") == 0) {
	    node_type = FDSP_DATA_MGR;
	    node_id = "localhost-dm";
	    control_port = 7900;
	  } if (strcmp(argv[1], "-s") == 0) {
	    node_type = FDSP_STOR_MGR;
	    node_id = "localhost-sm";
	    control_port = 7901;
	  }
	}

        /*
         * Pass 0 as the data port for now
         */
	om_client = new OMgrClient(node_type, omIpStr, 0, 0, node_id, NULL);

	om_client->initialize();
	om_client->registerEventHandlerForNodeEvents(my_node_event_handler);
	om_client->registerEventHandlerForVolEvents(my_vol_event_handler);
	// om_client->subscribeToOmEvents(0x0a010aca, 1, 1);
	om_client->startAcceptingControlMessages(control_port);
	om_client->registerNodeWithOM();

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
              fds_uint32_t node_port = 0;
	      int node_state = -1;
	      om_client->getNodeInfo(node_ids[i],
                                     &node_ip,
                                     &node_port,
                                     &node_state);
	      printf("Node id - %d Node ip - %x Node port - %x Node state - %d\n",
                     node_ids[i],
                     node_ip,
                     node_port,
                     node_state);
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
              fds_uint32_t node_port = 0;
	      int node_state = -1;
	      om_client->getNodeInfo(node_ids[i],
                                     &node_ip,
                                     &node_port,
                                     &node_state);
	      printf("Node id - %d Node ip - %x Node port - %x Node state - %d\n",
                     node_ids[i],
                     node_ip,
                     node_port,
                     node_state);
	    }
	    printf("\n");

	  }
	}
}

