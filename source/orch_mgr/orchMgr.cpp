#include "orchMgr.h"

#include <iostream>

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


  if (port_num != 0) {
      orchMgrPortNum = port_num;
    } else {
      orchMgrPortNum = props->getPropertyAsInt("OrchMgr.PortNumber");
    }
    orchMgrIPAddress = props->getProperty("OrchMgr.IPAddress");


  FDS_PLOG(om_log) << "Orchestration Manager using port " << port_num;
  
  callbackOnInterrupt();

  /*
   * setup CLI client adaptor interface  this also used for receiving the node up
   * messages from DM, SH and SM 
   */
  std::ostringstream tcpProxyStr;
  tcpProxyStr << "tcp -p " << port_num;
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

}

void OrchMgr::reqCfgHandler::DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req,
			 const Ice::Current&) {

}
void OrchMgr::reqCfgHandler::ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req,
			 const Ice::Current&) {

}
void OrchMgr::reqCfgHandler::CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req,
			 const Ice::Current&) {

}
void OrchMgr::reqCfgHandler::DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req,
			 const Ice::Current&) {

}
void OrchMgr::reqCfgHandler::ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req,
			 const Ice::Current&) {

}

void OrchMgr::reqCfgHandler::RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req,
			 const Ice::Current&) {



}




}  // namespace fds

int main(int argc, char *argv[]) {

  fds::orchMgr = new fds::OrchMgr();
  
  fds::orchMgr->main(argc, argv, "orchMgr.conf");

  delete fds::orchMgr;
}
