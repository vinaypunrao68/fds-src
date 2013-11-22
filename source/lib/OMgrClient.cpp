#include "OMgrClient.h"
#include <fds_assert.h>

using namespace std;
using namespace fds;


OMgrClientRPCI::OMgrClientRPCI(OMgrClient *omc) {
    this->om_client = omc;
}

void OMgrClientRPCI::NotifyAddVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                                  const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
                                  const Ice::Current&) {
  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL);
  fds_vol_notify_t type = fds_notify_vol_add;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc->volUUID, vdb, type, msg_hdr->result);

}

void OMgrClientRPCI::NotifyRmVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
			       const Ice::Current&) {

  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_RM_VOL);
  fds_vol_notify_t type = fds_notify_vol_rm;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc->volUUID, vdb, type, msg_hdr->result);

}
      
void OMgrClientRPCI::AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
			       const Ice::Current&) {
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvVolAttachState(vol_msg->vol_desc->volUUID, vdb, FDS_VOL_ACTION_ATTACH, msg_hdr->result);
}


void OMgrClientRPCI::DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
			       const Ice::Current&) {
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvVolAttachState(vol_msg->vol_desc->volUUID, vdb, FDS_VOL_ACTION_DETACH, msg_hdr->result);
}

void OMgrClientRPCI::NotifyNodeAdd(const FDSP_MsgHdrTypePtr& msg_hdr, 
				   const FDSP_Node_Info_TypePtr& node_info,
				   const Ice::Current&){
  om_client->recvNodeEvent(node_info->node_id, node_info->node_type, (unsigned int) node_info->ip_lo_addr, node_info->node_state, node_info);
}

void OMgrClientRPCI::NotifyNodeRmv(const FDSP_MsgHdrTypePtr& msg_hdr, 
				   const FDSP_Node_Info_TypePtr& node_info,
				   const Ice::Current&){
  om_client->recvNodeEvent(node_info->node_id, node_info->node_type, (unsigned int) node_info->ip_lo_addr, node_info->node_state, node_info); 
}

void OMgrClientRPCI::NotifyDLTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
				     const FDSP_DLT_TypePtr& dlt_info,
				     const Ice::Current&){
  om_client->recvDLTUpdate(dlt_info->DLT_version, dlt_info->DLT);
}

void OMgrClientRPCI::NotifyDMTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
				     const FDSP_DMT_TypePtr& dmt_info,
				     const Ice::Current&){
  om_client->recvDMTUpdate(dmt_info->DMT_version, dmt_info->DMT);
}

void OMgrClientRPCI::SetThrottleLevel(const FDSP_MsgHdrTypePtr& msg_hdr, 
					const FDSP_ThrottleMsgTypePtr& throttle_msg, 
					const Ice::Current&) {
  om_client->recvSetThrottleLevel((const float) throttle_msg->throttle_level);
}

void
OMgrClientRPCI::TierPolicy(const FDSP_TierPolicyPtr &tier, const Ice::Current &ice)
{
  FDS_PLOG_SEV(om_client->omc_log, fds::fds_log::notification)
    << "OMClient received tier policy for vol "
    << tier->tier_vol_uuid;

    fds_verify(om_client->omc_srv_pol != nullptr);
    om_client->omc_srv_pol->serv_recvTierPolicyReq(tier);
}

void
OMgrClientRPCI::TierPolicyAudit(const FDSP_TierPolicyAuditPtr &audit,
                                const Ice::Current            &ice)
{
  FDS_PLOG_SEV(om_client->omc_log, fds::fds_log::notification)
    << "OMClient received tier audit policy for vol "
    << audit->tier_vol_uuid;

    fds_verify(om_client->omc_srv_pol != nullptr);
    om_client->omc_srv_pol->serv_recvTierPolicyAuditReq(audit);
}

OMgrClient::OMgrClient(FDSP_MgrIdType node_type,
                       const std::string& _omIpStr,
                       fds_uint32_t _omPort,
                       const std::string& _hostIp,
                       fds_uint32_t data_port,
                       const std::string& node_name,
                       fds_log *parent_log) {
  my_node_type = node_type;
  omIpStr      = _omIpStr;
  omConfigPort = _omPort;
  hostIp       = _hostIp;
  my_data_port = data_port;
  my_node_name = node_name;
  node_evt_hdlr = NULL;
  vol_evt_hdlr = NULL;
  throttle_cmd_hdlr = NULL;
  if (parent_log) {
    omc_log = parent_log;
  } else {
    omc_log = new fds_log("omc", "logs");
  }

  initRPCComm();
}

OMgrClient::OMgrClient() {
  my_node_type = FDSP_STOR_HVISOR;
  my_node_name = "localhost-sh";
  node_evt_hdlr = NULL;
  vol_evt_hdlr = NULL;
  throttle_cmd_hdlr = NULL;
  omc_log = new fds_log("omc", "logs");
  initRPCComm();
}

int OMgrClient::initialize() {
  return fds::ERR_OK;
}

int OMgrClient::registerEventHandlerForNodeEvents(node_event_handler_t node_event_hdlr) {
  this->node_evt_hdlr = node_event_hdlr;
  return 0;
}
 

int OMgrClient::registerEventHandlerForVolEvents(volume_event_handler_t vol_event_hdlr) {
  this->vol_evt_hdlr = vol_event_hdlr;
  return 0;
}

int OMgrClient::registerThrottleCmdHandler(throttle_cmd_handler_t throttle_cmd_hdlr) {
  this->throttle_cmd_hdlr = throttle_cmd_hdlr;
  return 0;
}

int OMgrClient::initRPCComm() {

  int argc = 0;
  char **argv = 0;

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  rpc_comm = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr rpc_props = rpc_comm->getProperties();

  /*
   * Set basic thread properties.
   */
  rpc_props->setProperty("OMgrClient.ThreadPool.Size", "1");
  rpc_props->setProperty("OMgrClient.ThreadPool.SizeMax", "1");
  rpc_props->setProperty("OMgrClient.ThreadPool.SizeWarn", "1");

  /*
   * If the OM's IP wasn't given via cmdline
   * pull it from the config file.
   */
  if (omIpStr.empty() == true) {
    omIpStr = rpc_props->getProperty("OMgr.IP");
  }

  if (hostIp.empty() == true) {
    hostIp = rpc_props->getProperty("OMgrClient.MyIPAddr");
  }
  /*
   * If the OM's config port wasn't given via cmdline
   * pull it from the config file
   */
  if (omConfigPort == 0) {
    omConfigPort = strtoul(rpc_props->getProperty("OMgr.ConfigPort").c_str(),
                           NULL,
                           0);
  }
  
  return (0);

}  

#if 0

int OMgrClient::subscribeToOmEvents(unsigned int om_ip_addr, int tenn_id, int dom_id, int omc_port_num) {

  int argc = 0;
  char **argv = 0;

  this->tennant_id = tenn_id;
  this->domain_id = dom_id;

  try {

  // Load properties from config file om_client.conf

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  // Step 2: Contact pubsub server and get handle to topic mgr 

  pubsub_comm = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr pubsub_props = pubsub_comm->getProperties();
 
  std::string tcpProxyStr;
  tcpProxyStr = std::string("IceStorm/TopicManager:tcp -h ") + omIpStr + std::string(" -p 11234");
  Ice::ObjectPrx obj = pubsub_comm->stringToProxy(tcpProxyStr);
  pubsub_topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);


  // Step 3: Create an ednpoint to accept published messages

  pubsub_adapter = pubsub_comm->createObjectAdapterWithEndpoints("OMgrSubscriberAdapter", "tcp");
  om_pubsub_client_i = new OMgr_SubscriberI(this);
  pubsub_proxy = pubsub_adapter->addWithUUID(om_pubsub_client_i)->ice_oneway();


  // Step 4: Subscribe to topic "OMgrEvents" with subscription endpoint

  try {
    pubsub_topic = pubsub_topicManager->retrieve("OMgrEvents");
    IceStorm::QoS qos;
    pubsub_topic->subscribeAndGetPublisher(qos, pubsub_proxy);
  }
  catch (const IceStorm::NoSuchTopic&) {
    // Error! No topic found!
    cout  << "Topic not found" << endl;
    return 0;
  }

  pubsub_adapter->activate();

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMgrClient subscribed to OMgrEvents with Pub Sub server at "
						    << omIpStr << " : 11234";

  }
  catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMgrClient could not subscribe to OMgrEvents. Please check if pubsub server is up and restart";
  }

 
  return 0;
}    

#endif

// Call this to setup the (receiving side) endpoint to lister for control path requests from OM.
int OMgrClient::startAcceptingControlMessages() {
  return startAcceptingControlMessages(0);
}

int OMgrClient::startAcceptingControlMessages(fds_uint32_t port_num) {

  Ice::PropertiesPtr rpc_props = rpc_comm->getProperties();
  
  my_control_port = port_num;
  if (my_control_port == 0) {
    my_control_port = rpc_props->getPropertyAsInt("OMgrClient.PortNumber");
  }
 
  rpc_srv_id = std::string("tcp -p ") + std::to_string(my_control_port);
  Ice::ObjectAdapterPtr rpc_adapter =rpc_comm->createObjectAdapterWithEndpoints("OrchMgrClient", rpc_srv_id);

  om_client_rpc_i = new OMgrClientRPCI(this);
  rpc_adapter->add(om_client_rpc_i, rpc_comm->stringToIdentity("OrchMgrClient"));

  rpc_adapter->activate();

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient accepting control requests at port " << my_control_port;

  return (0);

}

void OMgrClient::initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
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

        msg_hdr->src_id = my_node_type;
        msg_hdr->dst_id = FDSP_ORCH_MGR;
	msg_hdr->src_node_name = my_node_name;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


}

// Use this to register the local node with OM as a client. Should be called after calling starting subscription endpoint and control path endpoint.
int OMgrClient::registerNodeWithOM(const FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr& dInfo) {

  try {

  Ice::PropertiesPtr props = rpc_comm->getProperties();
  std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") + 
      omIpStr + std::string(" -p ") + std::to_string(omConfigPort);
  FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  initOMMsgHdr(msg_hdr);
  FDSP_RegisterNodeTypePtr reg_node_msg = new FDSP_RegisterNodeType;
  reg_node_msg->node_type = my_node_type;
  reg_node_msg->node_name = my_node_name;
  reg_node_msg->ip_hi_addr = 0;
  reg_node_msg->ip_lo_addr = fds::str_to_ipv4_addr(hostIp); //my_address!
  reg_node_msg->control_port = my_control_port;
  reg_node_msg->data_port = my_data_port; // for now
  /* init the disk info */
   
   reg_node_msg->disk_info =  new FDSP_AnnounceDiskCapability;
   reg_node_msg->disk_info->disk_iops_max =  dInfo->disk_iops_max;
   reg_node_msg->disk_info->disk_iops_min =  dInfo->disk_iops_min;
   reg_node_msg->disk_info->disk_capacity = dInfo->disk_capacity;
   reg_node_msg->disk_info->disk_latency_max = dInfo->disk_latency_max;
   reg_node_msg->disk_info->disk_latency_min = dInfo->disk_latency_min;
   reg_node_msg->disk_info->ssd_iops_max =  dInfo->ssd_iops_max;
   reg_node_msg->disk_info->ssd_iops_min =  dInfo->ssd_iops_min;
   reg_node_msg->disk_info->ssd_capacity = dInfo->ssd_capacity;
   reg_node_msg->disk_info->ssd_latency_max = dInfo->ssd_latency_min;
   reg_node_msg->disk_info->ssd_latency_min = dInfo->ssd_latency_min;
   reg_node_msg->disk_info->disk_type = dInfo->disk_type;

   FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient registering local node " 
						     << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr) 
						     << " control port:" << reg_node_msg->control_port 
						     << " data port:" << reg_node_msg->data_port
						     << " with Orchaestration Manager at " << tcpProxyStr;

  fdspConfigPathAPI->begin_RegisterNode(msg_hdr, reg_node_msg);

  FDS_PLOG_SEV(omc_log, fds::fds_log::debug) << "OMClient completed node registration with OM";

  }
  catch(...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::critical) << "OMClient unable to register node with OrchMgr. Please check if OrchMgr is up and restart.";
  }

  return (0);
}

int OMgrClient::pushPerfstatsToOM(const std::string& start_ts,
				  int stat_slot_len,
				  const FDS_ProtocolInterface::FDSP_VolPerfHistListType& hist_list)
{
  try {
    std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") + 
      omIpStr + std::string(" -p ") + std::to_string(omConfigPort);
    FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
    FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
    initOMMsgHdr(msg_hdr);
    FDSP_PerfstatsTypePtr perf_stats_msg = new FDSP_PerfstatsType;
    perf_stats_msg->node_type = my_node_type;
    perf_stats_msg->start_timestamp = start_ts;
    perf_stats_msg->slot_len_sec = stat_slot_len;
    perf_stats_msg->vol_hist_list = hist_list;  

    FDS_PLOG_SEV(omc_log, fds::fds_log::normal) << "OMClient pushing perfstats to OM at " << tcpProxyStr
						<< " start ts " << start_ts;

    fdspConfigPathAPI->begin_NotifyPerfstats(msg_hdr, perf_stats_msg);

  } catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to push perf stats to OM. Check if OM is up and restart.";
  }

  return 0;
}

int OMgrClient::testBucket(const std::string& bucket_name,
			   const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& vol_info,
			   fds_bool_t attach_vol_reqd,
			   const std::string& accessKeyId,
			   const std::string& secretAccessKey)
{
  try {
    std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") + 
      omIpStr + std::string(" -p ") + std::to_string(omConfigPort);
    FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
    FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
    initOMMsgHdr(msg_hdr);
    FDSP_TestBucketPtr test_buck_msg = new FDSP_TestBucket;
    test_buck_msg->bucket_name = bucket_name;
    test_buck_msg->vol_info = vol_info;
    test_buck_msg->attach_vol_reqd = attach_vol_reqd;
    test_buck_msg->accessKeyId = accessKeyId;
    test_buck_msg->secretAccessKey;

    FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient sending test bucket request to OM at " << tcpProxyStr;

    fdspConfigPathAPI->begin_TestBucket(msg_hdr, test_buck_msg);
  }
  catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to send testBucket request to OM. Check if OM is up and restart.";
    return -1;
  }

  return 0;
}

int OMgrClient::pushCreateBucketToOM(const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& volInfo)
{
  try {
         std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") + 
         		omIpStr + std::string(" -p ") + std::to_string(omConfigPort);
  	 FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
         FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
         initOMMsgHdr(msg_hdr);
  
         FDSP_CreateVolTypePtr volData = new FDSP_CreateVolType();
         volData->vol_info = new FDSP_VolumeInfoType();

         volData->vol_name = volInfo->vol_name;
         volData->vol_info->vol_name = volInfo->vol_name;
         volData->vol_info->tennantId = volInfo->tennantId;
         volData->vol_info->localDomainId = volInfo->localDomainId;
         volData->vol_info->globDomainId = volInfo->globDomainId;

         volData->vol_info->capacity = volInfo->capacity;
         volData->vol_info->maxQuota = volInfo->maxQuota;
         volData->vol_info->volType = volInfo->volType;

         volData->vol_info->defReplicaCnt = volInfo->defReplicaCnt;
         volData->vol_info->defWriteQuorum = volInfo->defWriteQuorum;
         volData->vol_info->defReadQuorum = volInfo->defReadQuorum;
         volData->vol_info->defConsisProtocol = volInfo->defConsisProtocol;

         volData->vol_info->volPolicyId = 50; //  default policy 
         volData->vol_info->archivePolicyId = volInfo->archivePolicyId;
         volData->vol_info->placementPolicy = volInfo->placementPolicy;
         volData->vol_info->appWorkload = volInfo->appWorkload;

    	 fdspConfigPathAPI->begin_CreateVol(msg_hdr, volData);
  } catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to push  the create bucket request to OM. Check if OM is up and restart.";
  }

   /* do attach volume for this bucket */
  try {
    std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") + 
         		omIpStr + std::string(" -p ") + std::to_string(omConfigPort);
  	 FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
         FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
         initOMMsgHdr(msg_hdr);

        FDSP_AttachVolCmdTypePtr volData = new  FDSP_AttachVolCmdType();

   	volData->vol_name = volInfo->vol_name; 
   	volData->node_id = std::string("localhost-sh"); 
  	fdspConfigPathAPI->AttachVol(msg_hdr, volData);
  } catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to push  the attach  bucket to  OM. Check if OM is up and restart.";
  }

  return 0;
}

int OMgrClient::pushDeleteBucketToOM(const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& volInfo)
{
  try {
         std::string tcpProxyStr = std::string("OrchMgr: tcp -h ") + 
         		omIpStr + std::string(" -p ") + std::to_string(omConfigPort);
  	 FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(rpc_comm->stringToProxy(tcpProxyStr));
         FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
         initOMMsgHdr(msg_hdr);

 	FDSP_DeleteVolTypePtr volData = new FDSP_DeleteVolType();
   	volData->vol_name  = volInfo->vol_name;
  	volData->domain_id = volInfo->domain_id;
    	fdspConfigPathAPI->begin_DeleteVol(msg_hdr, volData);

  } catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to push perf stats to OM. Check if OM is up and restart.";
  }

  return 0;
}

int OMgrClient::recvNodeEvent(int node_id, 
			      FDSP_MgrIdType node_type, 
			      unsigned int node_ip, 
			      int node_state,
			      const FDSP_Node_Info_TypePtr& node_info) 
{ 
  omc_lock.write_lock();

  node_info_t& node = node_map[node_id];

  node.node_id = node_id;
  node.node_ip_address = node_ip;
  node.port = node_info->data_port;
  node.node_state = (FDSP_NodeState) node_state;

  omc_lock.write_unlock();

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received node event for node " << node_id 
						    << ", type - " << node_info->node_type 
						    << " with ip address " << node_ip 
						    << " and state - " << node_state;

  if (this->node_evt_hdlr) {
    this->node_evt_hdlr(node_id,
                        node_ip,
                        node_state,
                        node_info->data_port,
                        node_info->node_type);
  }
  return (0);
  
}

int OMgrClient::recvNotifyVol(fds_volid_t vol_id,
                              VolumeDesc *vdb,
                              fds_vol_notify_t vol_action,
			      FDSP_ResultType result) {

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received volume event for volume " << vol_id 
						    << " action - " << vol_action;

  if (this->vol_evt_hdlr) {
    this->vol_evt_hdlr(vol_id, vdb, vol_action, result);
  }
  return (0);
  
}

int OMgrClient::recvVolAttachState(fds_volid_t vol_id,
                                   VolumeDesc *vdb,
                                   int vol_action,
				   FDSP_ResultType result) {

  assert((vol_action == FDS_VOL_ACTION_ATTACH) || (vol_action == FDS_VOL_ACTION_DETACH));

  fds_vol_notify_t type = fds_notify_vol_attatch;
  if (vol_action == FDS_VOL_ACTION_DETACH) {
    type = fds_notify_vol_detach;
  }

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received volume attach/detach request for volume " << vol_id 
						    << " action - " << type;

  if (this->vol_evt_hdlr) {
    this->vol_evt_hdlr(vol_id, vdb, type, result);
  }
  return (0);
  
}


int OMgrClient::recvDLTUpdate(int dlt_vrsn, const Node_Table_Type& dlt_table) {

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received new DLT version  " << dlt_vrsn;

  omc_lock.write_lock();

  this->dlt_version = dlt_vrsn;
  this->dlt = dlt_table;

  omc_lock.write_unlock();

  return (0);
}

int OMgrClient::recvDMTUpdate(int dmt_vrsn, const Node_Table_Type& dmt_table) {

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received new DMT version  " << dmt_vrsn;

  omc_lock.write_lock();

  this->dmt_version = dmt_vrsn;
  this->dmt = dmt_table;

  omc_lock.write_unlock();

  return (0);
}

int OMgrClient::recvSetThrottleLevel(const float throttle_level) {

  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received new throttle level  " << throttle_level;

  this->current_throttle_level = throttle_level;

  if (this->throttle_cmd_hdlr) {
    this->throttle_cmd_hdlr(throttle_level);
  }
  return (0);

}

int OMgrClient::getNodeInfo(int node_id,
                            unsigned int *node_ip_addr,
                            fds_uint32_t *node_port,
                            int *node_state) {
  
  omc_lock.read_lock();

  if (node_map.count(node_id) <= 0) {
    omc_lock.read_unlock();
    return (-1);
  }

  node_info_t& node = node_map[node_id];

  assert(node.node_id == node_id);

  *node_ip_addr = node.node_ip_address;
  *node_port = node.port;
  *node_state = node.node_state;

  omc_lock.read_unlock();
  
  return 0;
} 

int OMgrClient::getDLTNodesForDoidKey(unsigned char doid_key, int *node_ids, int *n_nodes) {
  
  omc_lock.read_lock();

  int total_shards = this->dlt.size();
  int lookup_key = doid_key % total_shards;
  int total_nodes = this->dlt[lookup_key].size();
  *n_nodes = (total_nodes < *n_nodes)? total_nodes:*n_nodes;
  int i;
  
  for (i = 0; i < *n_nodes; i++) {
    node_ids[i] = this->dlt[lookup_key][i];
  } 

  omc_lock.read_unlock();

  return 0;
}

int OMgrClient::getDMTNodesForVolume(int vol_id, int *node_ids, int *n_nodes) {

  omc_lock.read_lock();

  int total_shards = this->dmt.size();
  int lookup_key = vol_id % total_shards;
  int total_nodes = this->dmt[lookup_key].size();
  *n_nodes = (total_nodes < *n_nodes)? total_nodes:*n_nodes;
  int i;
  
  for (i = 0; i < *n_nodes; i++) {
    node_ids[i] = this->dmt[lookup_key][i];
  } 

  omc_lock.read_unlock();

  return 0;
}
