#include <iostream>
#include <unordered_map>

#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <fds_pubsub.h>
#include <FDSP.h>
#include <stdio.h>

#define CMD_NOOP 0
#define CMD_NODE_UP 1
#define CMD_NODE_DOWN 2
#define CMD_DLT_UPDATE 3
#define CMD_DMT_UPDATE 4
#define CMD_VOL_CREATE 5
#define CMD_VOL_ATTACH 6
#define CMD_VOL_DETACH 7

using namespace FDSP_Types;
using namespace FDS_PubSub_Interface;
using namespace FDS_ProtocolInterface;
using namespace std;

typedef struct _node_info_t {

  int node_id;
  unsigned int node_ip_address;
  FDSP_NodeState node_state;

} node_info_t;


typedef std::unordered_map<int,node_info_t> node_map_t;

void construct_node_table_from_file(char *file_name, Node_Table_Type& node_table);
void handle_create_volume(Ice::CommunicatorPtr& comm, int vol_id, const node_map_t& node_map);
void handle_attach_volume(int cmd, Ice::CommunicatorPtr& comm, int vol_id, std::string node_ip_str);

int main(int argc, char* argv[])
{

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("orch_mgr.conf");

  Ice::CommunicatorPtr comm;
  comm = Ice::initialize(argc, argv, initData);

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
    int vol_id;
    char *line_ptr = 0;
    int cmd;
    unsigned int node_ip;
    int node_ip_bytes[4];
    char cmd_wd[16];
    char tmp_file_name[32];
    char node_ip_str[32];
    size_t n_bytes;
    int current_dlt_version = 0;
    int current_dmt_version = 0;
    Node_Table_Type current_dlt_table;
    Node_Table_Type current_dmt_table;
    node_map_t current_node_map;

    while(1) {
      cmd = CMD_NOOP;
      // printf(">");
      std::cout << ">";
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
      } else if (strcmp(cmd_wd, "crtvol") == 0) {
	cmd = CMD_VOL_CREATE;
	sscanf(line_ptr, "crtvol %d", &vol_id);
      } else  if (strcmp(cmd_wd, "attvol") == 0) {
	cmd = CMD_VOL_ATTACH;
	sscanf(line_ptr, "attvol %d %s", &vol_id, node_ip_str);
      } else  if (strcmp(cmd_wd, "detvol") == 0) {
	cmd = CMD_VOL_DETACH;
	sscanf(line_ptr, "detvol %d %s", &vol_id, node_ip_str);
      }

else {
        std::cout << "Unknown command " << cmd_wd
                  << ". Please try again" << std::endl;
	// printf("Unknown command %s. Please try again\n", cmd_wd);
	continue;
      }
      
      try {
	
	FDS_PubSub_Interface::FDS_PubSub_MsgHdrTypePtr msg_hdr_ptr = new FDS_PubSub_MsgHdrType;
	FDSP_Types::FDSP_Node_Info_TypePtr node_info_ptr = new FDSP_Node_Info_Type;
	FDSP_Types::FDSP_DLT_TypePtr dlt_info_ptr = new FDSP_DLT_Type;
	FDSP_Types::FDSP_DMT_TypePtr dmt_info_ptr = new FDSP_DMT_Type;
	node_info_t n_info;


	msg_hdr_ptr->msg_code = FDS_Node_Add;
	msg_hdr_ptr->msg_id = 0;
	msg_hdr_ptr->tennant_id = 1;
	msg_hdr_ptr->domain_id = 1;

	switch(cmd) {

	case CMD_NODE_UP:
	  node_info_ptr->node_id = n_info.node_id = node_id;
	  node_info_ptr->node_state = n_info.node_state = FDS_Node_Up;
	  node_info_ptr->node_ip = n_info.node_ip_address = node_ip;
	  current_node_map[node_id] = n_info;
	  om_client->NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
	  break;

	case CMD_NODE_DOWN:
	  node_info_ptr->node_id = node_id;
	  node_info_ptr->node_ip = 0;
	  node_info_ptr->node_state = FDS_Node_Rmvd;
	  current_node_map.erase(node_id);
	  om_client->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
	  break;

	case CMD_DLT_UPDATE:
	  {
	    current_dlt_version++;
	    dlt_info_ptr->DLT_version = current_dlt_version;
	    construct_node_table_from_file(tmp_file_name, current_dlt_table);
	    dlt_info_ptr->DLT = current_dlt_table;

	    // printf("DLT: %u shards\n", dlt_info_ptr->DLT.size());
            std::cout << "DLT: "<< dlt_info_ptr->DLT.size()
                      << " shards" << std::endl;
	    om_client->NotifyDLTUpdate(msg_hdr_ptr, dlt_info_ptr);
	    break;
	  }

	case CMD_DMT_UPDATE:
	  {
	    current_dmt_version++;
	    dmt_info_ptr->DMT_version = current_dmt_version;
	    construct_node_table_from_file(tmp_file_name, current_dmt_table);
	    dmt_info_ptr->DMT = current_dmt_table;

	    std::cout << "DMT: "<< dmt_info_ptr->DMT.size()
                      << " shards" << std::endl;
	    om_client->NotifyDMTUpdate(msg_hdr_ptr, dmt_info_ptr);
	    break;
	  }

	case CMD_VOL_CREATE:
	  {
	    handle_create_volume(comm, vol_id, current_node_map);
	    break;
	  }
	case CMD_VOL_ATTACH:
        case CMD_VOL_DETACH:
	  {
	    handle_attach_volume(cmd, comm, vol_id, std::string(node_ip_str));
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

void InitOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
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

        msg_hdr->src_id = FDSP_ORCH_MGR;
        msg_hdr->dst_id = FDSP_STOR_HVISOR;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


}

void handle_create_volume(Ice::CommunicatorPtr& comm, int vol_id, const node_map_t& node_map) {

  Ice::PropertiesPtr props = comm->getProperties();

  std::string dm_port = props->getProperty("DataMgr.ControlPort");
  std::string sm_port = props->getProperty("StorMgr.ControlPort");
  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  FDSP_NotifyVolTypePtr vol_msg = new FDSP_NotifyVolType;

  InitOMMsgHdr(msg_hdr);
  msg_hdr->glob_volume_id = vol_id;

  for (auto it = node_map.begin(); it != node_map.end(); ++it) {

    int node_id = it->first;
    const node_info_t& node_info = it->second;
    std::cout << " Making create volume rpc call with node " << node_id << " ip - " << node_info.node_ip_address << endl;
    std::cout << " DM port - " << dm_port << "SM port - " << sm_port << endl;

    std::ostringstream tcpProxyStr;
  
    tcpProxyStr << "OrchMgrClient: tcp -h " << std::to_string(node_info.node_ip_address) << " -p  " << dm_port;
    FDSP_ControlPathReqPrx dmOMClientAPI = FDSP_ControlPathReqPrx::checkedCast(comm->stringToProxy (tcpProxyStr.str()));

    std::ostringstream smTcpProxyStr;
  
    smTcpProxyStr << "OrchMgrClient: tcp -h " << std::to_string(node_info.node_ip_address) << " -p  " << sm_port;
    FDSP_ControlPathReqPrx smOMClientAPI = FDSP_ControlPathReqPrx::checkedCast(comm->stringToProxy (smTcpProxyStr.str()));


    vol_msg->vol_name = std::string("Test Volume");
    dmOMClientAPI->NotifyAddVol(msg_hdr, vol_msg);
    smOMClientAPI->NotifyAddVol(msg_hdr, vol_msg);

  }
}

void handle_attach_volume(int cmd, Ice::CommunicatorPtr& comm, int vol_id, std::string node_ip_str) {

  assert((cmd == CMD_VOL_ATTACH) || (cmd == CMD_VOL_DETACH));

  Ice::PropertiesPtr props = comm->getProperties();

  std::string hvisor_port = props->getProperty("StorHvisor.ControlPort");

  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  FDSP_AttachVolTypePtr vol_msg = new FDSP_AttachVolType;

  InitOMMsgHdr(msg_hdr);
  msg_hdr->glob_volume_id = vol_id;

  std::ostringstream tcpProxyStr;
  
  tcpProxyStr << "OrchMgrClient: tcp -h " << node_ip_str << " -p  " << hvisor_port;
  FDSP_ControlPathReqPrx omClientAPI = FDSP_ControlPathReqPrx::checkedCast(comm->stringToProxy (tcpProxyStr.str()));

  vol_msg->vol_name = std::string("Test Volume");

  if (cmd == CMD_VOL_ATTACH) {
     std::cout << "Making attach volume rpc call with hvisor at " << node_ip_str 
               << " at port " << hvisor_port;
    omClientAPI->AttachVol(msg_hdr, vol_msg);
  }
  else {
     std::cout << "Making detach volume rpc call with hvisor at " << node_ip_str 
               << " at port " << hvisor_port;
     omClientAPI->DetachVol(msg_hdr, vol_msg);
  }
}

void construct_node_table_from_file(char *file_name, Node_Table_Type& node_table) {

  FILE *fp;
  size_t n_bytes = 0;
  char *line_ptr = 0; 
  
  node_table.clear();

  // printf("Table size- %u\n", node_table.size());
  std::cout << "Table size- " << node_table.size() << std::endl;

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

    // printf("Table size- %u, line - %s\n", node_table.size(), line_ptr);
    std::cout << "Table size- " << node_table.size()
              << ", line - " << line_ptr << std::endl;
    line_ptr[0] = 0;

  }

  fclose(fp);
  
}
