#include "orchMgr.h"

#include <iostream>


using namespace std;

namespace fds {

OrchMgr *orchMgr;


OrchMgr::OrchMgr()
    : port_num(0){
  om_log = new fds_log("om", "logs");
  om_mutex = new fds_mutex("OrchMgrMutex");
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

  reqCfgHandlersrv = new ReqCfgHandler(this);
  adapter->add(reqCfgHandlersrv, communicator()->stringToIdentity("OrchMgr"));

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
OrchMgr::ReqCfgHandler::ReqCfgHandler(OrchMgr *oMgr) {
  this->orchMgr = oMgr;
}

OrchMgr::ReqCfgHandler::~ReqCfgHandler() {

}

void OrchMgr::copyVolumeInfo(FDS_Volume *pVol, FDSP_VolumeInfoTypePtr v_info) {
  pVol->vol_name = v_info->vol_name;
  pVol->volUUID = v_info->volUUID;
  pVol->capacity = v_info->capacity;
}


void OrchMgr::CreateVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			const FDS_ProtocolInterface::FDSP_CreateVolTypePtr &crt_vol_req) {

  int  vol_id = crt_vol_req->vol_info->volUUID;
  string vol_name = crt_vol_req->vol_info->vol_name;
  
  FDS_PLOG(GetLog()) << "Received CreateVol Req for volume " << vol_name << " ; id - " << vol_id;
  om_mutex->lock();
  if (volumeMap.count(vol_id) != 0) {
    FDS_PLOG(om_log) << "Received CreateVol for existing volume " << vol_id;
    om_mutex->unlock();
  }
  FDS_Volume *new_vol = new FDS_Volume();
  copyVolumeInfo(new_vol, crt_vol_req->vol_info);
  volumeMap[vol_id] = new_vol;
  om_mutex->unlock();

}

void OrchMgr::DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req) {

  int  vol_id = del_vol_req->vol_uuid;
  string vol_name = del_vol_req->vol_name;
  
  FDS_PLOG(GetLog()) << "Received DeleteVol Req for volume " << vol_name << " ; id - " << vol_id;
  om_mutex->lock();
  if (volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received DeleteVol for non-existent volume " << vol_id;
    om_mutex->unlock();
  }
  FDS_Volume *del_vol = volumeMap[vol_id];
  volumeMap.erase(vol_id);
  om_mutex->unlock();

  delete del_vol;

}

void OrchMgr::ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req) {

  		FDS_PLOG(GetLog()) << "Received ModifyVol  Msg";

}

void OrchMgr::CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			   const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req) {

  		FDS_PLOG(GetLog()) << "Received CreatePolicy  Msg";
}

void OrchMgr::DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			   const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req) {
  		FDS_PLOG(GetLog()) << "Received CreatePolicy  Msg";
}

void OrchMgr::ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			   const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req) {

  		FDS_PLOG(GetLog()) << "Received ModifyPolicy  Msg";
}

void OrchMgr::RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			   const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req) {

		std::string     ip_addr_str;
		NodeInfo n_info;
		Ice::Identity ident;
  		std::ostringstream tcpProxyStr;

  		FDS_PLOG(GetLog()) << "Received RegisterNode Msg"
				   << "  Node Id:" << reg_node_req->node_id
				   << "  Node IP:" << reg_node_req->ip_lo_addr
				   << "  Node Type:" << reg_node_req->node_type;

  		ip_addr_str = ipv4_addr_to_str(reg_node_req->ip_lo_addr);

		// build the SM node map
		n_info.node_id = reg_node_req->node_id;
		n_info.node_state = FDS_Node_Up;
		n_info.node_ip_address = reg_node_req->ip_lo_addr;
				
		// create a new  control  communication adaptor
		tcpProxyStr << "OrchMgrClient: tcp -h " << ip_addr_str << " -p  " << reg_node_req->control_port;
   		n_info.cpPrx =  FDSP_ControlPathReqPrx::checkedCast(communicator()->stringToProxy (tcpProxyStr.str())); 

		switch (reg_node_req->node_type)
		{
		case FDSP_STOR_MGR:
		  currentSmMap[reg_node_req->node_id] = n_info;
		  break;

    		case FDSP_DATA_MGR:
		  currentDmMap[reg_node_req->node_id] = n_info;
		  break;
    		case FDSP_STOR_HVISOR:
		  currentShMap[reg_node_req->node_id] = n_info;
		  break;
		default:
		  FDS_PLOG(GetLog()) << "Unknown node type received";

		}

}

void OrchMgr::ReqCfgHandler::CreateVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
				       const FDS_ProtocolInterface::FDSP_CreateVolTypePtr &crt_vol_req,
				       const Ice::Current&) {

  orchMgr->CreateVol(fdsp_msg, crt_vol_req);

}

void OrchMgr::ReqCfgHandler::DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req,
			 const Ice::Current&) {
  orchMgr->DeleteVol(fdsp_msg, del_vol_req);
}
void OrchMgr::ReqCfgHandler::ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req,
			 const Ice::Current&) {
  orchMgr->ModifyVol(fdsp_msg, mod_vol_req);
}
void OrchMgr::ReqCfgHandler::CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req,
			 const Ice::Current&) {
  orchMgr->CreatePolicy(fdsp_msg, crt_pol_req);
}
void OrchMgr::ReqCfgHandler::DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req,
			 const Ice::Current&) {
  orchMgr->DeletePolicy(fdsp_msg, del_pol_req);
}
void OrchMgr::ReqCfgHandler::ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req,
			 const Ice::Current&) {
  orchMgr->ModifyPolicy(fdsp_msg, mod_pol_req);
}

void OrchMgr::ReqCfgHandler::RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			   const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req,
			   const Ice::Current&) {
  orchMgr->RegisterNode(fdsp_msg, reg_node_req);
}


FDS_Volume::FDS_Volume()
{
  tennantId = 1;
  localDomainId = 1;
  globDomainId = 1;
  volUUID = 1;
  volType = FDS_VOL_BLKDEV_TYPE;

  capacity = 0;
  maxQuota = 0;

  replicaCnt = 1;
  writeQuorum = 1;
  readQuorum = 1;
  consisProtocol = FDS_CONS_PROTO_STRONG;

  volPolicyId = 1;
  archivePolicyId = 1;
  placementPolicy = 1;
  appWorkload = FDS_APP_WKLD_JOURNAL_FILESYS;

  backupVolume = 1;
}

FDS_Volume::~FDS_Volume()
{
}

}  // namespace fds


int main(int argc, char *argv[]) {

  fds::orchMgr = new fds::OrchMgr();
  
  fds::orchMgr->main(argc, argv, "orch_mgr.conf");

  delete fds::orchMgr;
}

