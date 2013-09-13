#include "orchMgr.h"

#include "include/fds_types.h"
#include <iostream>
#include <unordered_map>

using namespace FDSP_Types;
using namespace std;
using namespace FDS_ProtocolInterface;

typedef struct _node_info_t {

  int node_id;
  unsigned int node_ip_address;
  FDSP_NodeState node_state;
  FDSP_ControlPathReqPrx  cpPrx;
  

} node_info_t;


typedef std::unordered_map<int,node_info_t> node_map_t;

node_map_t currentSmMap;
node_map_t currentDmMap;
node_map_t currentShMap;


namespace fds {

OrchMgr *orchMgr;


OrchMgr::OrchMgr()
    : port_num(0){
  om_log = new fds_log("om", "logs");

  FDS_PLOG(om_log) << "Constructing the Orchestration  Manager";
}

OrchMgr::~OrchMgr() {

  FDS_PLOG(om_log) << "Destructing the Orchestration  Manager";
  delete om_log;
}

int OrchMgr::run(int argc, char* argv[]) {
  /*
   * Process the cmdline args.
   */
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--port=", 7) == 0) {
      port_num = strtoul(argv[i] + 7, NULL, 0);
    } else {
      std::cout << "Invalid argument " << argv[i] << std::endl;
      return -1;
    }
  }

  Ice::PropertiesPtr props = communicator()->getProperties();

  /*
   * Set basic thread properties.
   */
  props->setProperty("OrchMgr.ThreadPool.Size", "50");
  props->setProperty("OrchMgr.ThreadPool.SizeMax", "100");
  props->setProperty("OrchMgr.ThreadPool.SizeWarn", "75");


  std::string orchMgrIPAddress;
  int orchMgrPortNum;


  FDS_PLOG(om_log) << " port num rx " << port_num;
  if (port_num != 0) {
      orchMgrPortNum = port_num;
    } else {
      orchMgrPortNum = props->getPropertyAsInt("OrchMgr.ConfigPort");
    }
    orchMgrIPAddress = props->getProperty("OrchMgr.IPAddress");


  FDS_PLOG(om_log) << "Orchestration Manager using port " << orchMgrPortNum;
  
  
  callbackOnInterrupt();

  /*
   * setup CLI client adaptor interface  this also used for receiving the node up
   * messages from DM, SH and SM 
   */
  std::ostringstream tcpProxyStr;
  tcpProxyStr << "tcp -p " << orchMgrPortNum;
  Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapterWithEndpoints("OrchMgr", tcpProxyStr.str());

  ReqCfgHandlersrv = new reqCfgHandler(communicator());
  adapter->add(ReqCfgHandlersrv, communicator()->stringToIdentity("OrchMgr"));

  adapter->activate();

  communicator()->waitForShutdown();

  return EXIT_SUCCESS;
}

fds_log* OrchMgr::GetLog() {
  return om_log;
}

void
OrchMgr::interruptCallback(int)
{
  FDS_PLOG(orchMgr->GetLog()) << "Shutting down communicator";
  communicator()->shutdown();
}

  // config path request  handler 
OrchMgr::reqCfgHandler::reqCfgHandler(const Ice::CommunicatorPtr& communicator) {

}

OrchMgr::reqCfgHandler::~reqCfgHandler() {

}


void OrchMgr::reqCfgHandler::CreateVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreateVolTypePtr &crt_vol_req,
			 const Ice::Current&) {

  		FDS_PLOG(orchMgr->GetLog()) << "Received CreateVol  Msg";

}

void OrchMgr::reqCfgHandler::DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req,
			 const Ice::Current&) {

  		FDS_PLOG(orchMgr->GetLog()) << "Received DeleteVol  Msg";
}
void OrchMgr::reqCfgHandler::ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req,
			 const Ice::Current&) {

  		FDS_PLOG(orchMgr->GetLog()) << "Received ModifyVol  Msg";

}
void OrchMgr::reqCfgHandler::CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req,
			 const Ice::Current&) {

  		FDS_PLOG(orchMgr->GetLog()) << "Received CreatePolicy  Msg";
}
void OrchMgr::reqCfgHandler::DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req,
			 const Ice::Current&) {

  		FDS_PLOG(orchMgr->GetLog()) << "Received CreatePolicy  Msg";
}
void OrchMgr::reqCfgHandler::ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req,
			 const Ice::Current&) {

  		FDS_PLOG(orchMgr->GetLog()) << "Received ModifyPolicy  Msg";
}

void OrchMgr::reqCfgHandler::RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req,
			 const Ice::Current&) {

		std::string     ip_addr_str;
		node_info_t n_info;
		Ice::Identity ident;
  		std::ostringstream tcpProxyStr;

  		FDS_PLOG(orchMgr->GetLog()) << "Received RegisterNode Msg"
									<< "  Node Id:" << reg_node_req->node_id
									<< "  Node IP:" << reg_node_req->ip_lo_addr
									<< "  Node Type:" << reg_node_req->node_type;

  		ip_addr_str = ipv4_addr_to_str(reg_node_req->ip_lo_addr);

		// create a new  control  communication adaptor
		tcpProxyStr << "OrchMgrClient: tcp -h " << ip_addr_str << " -p  " << reg_node_req->control_port;
   		n_info.cpPrx =  FDSP_ControlPathReqPrx::checkedCast(communicator()->stringToProxy (tcpProxyStr.str())); 
		switch (reg_node_req->node_type)
		{
			case FDSP_STOR_MGR:
				// build the SM node map
				n_info.node_id = reg_node_req->node_id;
         	 	n_info.node_state = FDS_Node_Up;
          		n_info.node_ip_address = reg_node_req->ip_lo_addr;
          		currentSmMap[reg_node_req->node_id] = n_info;

    		case FDSP_DATA_MGR:
				// build the SM node map
				n_info.node_id = reg_node_req->node_id;
         	 	n_info.node_state = FDS_Node_Up;
          		n_info.node_ip_address = reg_node_req->ip_lo_addr;
          		currentDmMap[reg_node_req->node_id] = n_info;
    		case FDSP_STOR_HVISOR:
				// build the SM node map
				n_info.node_id = reg_node_req->node_id;
         	 	n_info.node_state = FDS_Node_Up;
          		n_info.node_ip_address = reg_node_req->ip_lo_addr;
          		currentShMap[reg_node_req->node_id] = n_info;
			default:
  				FDS_PLOG(orchMgr->GetLog()) << "Unknown node type received";

		}

}

}  // namespace fds

int main(int argc, char *argv[]) {

  fds::orchMgr = new fds::OrchMgr();
  
  fds::orchMgr->main(argc, argv, "orch_mgr.conf");

  delete fds::orchMgr;
}
