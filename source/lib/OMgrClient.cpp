#include "OMgrClient.h"
#include <fds_assert.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TBufferTransports.h>

#include <thread>

using namespace std;
using namespace fds;


OMgrClientRPCI::OMgrClientRPCI(OMgrClient *omc) {
    this->om_client = omc;
}

void OMgrClientRPCI::NotifyAddVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                                  FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL);
  fds_vol_notify_t type = fds_notify_vol_add;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc.volUUID, vdb, type, msg_hdr->result);

}

void OMgrClientRPCI::NotifyModVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                                  FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_MOD_VOL);
  fds_vol_notify_t type = fds_notify_vol_mod;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc.volUUID, vdb, type, msg_hdr->result);
}

void OMgrClientRPCI::NotifyRmVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                                 FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg) {
  assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_RM_VOL);
  fds_vol_notify_t type = fds_notify_vol_rm;
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvNotifyVol(vol_msg->vol_desc.volUUID, vdb, type, msg_hdr->result);

}
      
void OMgrClientRPCI::AttachVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg) {
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvVolAttachState(vol_msg->vol_desc.volUUID, vdb, FDS_VOL_ACTION_ATTACH, msg_hdr->result);
}


void OMgrClientRPCI::DetachVol(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
			       FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg) {
  fds::VolumeDesc *vdb = new fds::VolumeDesc(vol_msg->vol_desc);
  om_client->recvVolAttachState(vol_msg->vol_desc.volUUID, vdb, FDS_VOL_ACTION_DETACH, msg_hdr->result);
}

void OMgrClientRPCI::NotifyNodeAdd(FDSP_MsgHdrTypePtr& msg_hdr, 
				   FDSP_Node_Info_TypePtr& node_info) {
  om_client->recvNodeEvent(node_info->node_id, node_info->node_type,
                           (unsigned int) node_info->ip_lo_addr, node_info->node_state, node_info);
}

void OMgrClientRPCI::NotifyNodeRmv(FDSP_MsgHdrTypePtr& msg_hdr, 
				   FDSP_Node_Info_TypePtr& node_info) {
    om_client->recvNodeEvent(node_info->node_id, node_info->node_type,
                             (unsigned int) node_info->ip_lo_addr, node_info->node_state, node_info); 
}

void OMgrClientRPCI::NotifyDLTUpdate(FDSP_MsgHdrTypePtr& msg_hdr,
				     FDSP_DLT_TypePtr& dlt_info) {
  om_client->recvDLTUpdate(dlt_info->DLT_version, dlt_info->DLT);
}
void OMgrClientRPCI::NotifyStartMigration(FDSP_MsgHdrTypePtr& msg_hdr,
			 FDSP_DLT_Data_TypePtr& dlt_info) {

}

void OMgrClientRPCI::NotifyDMTUpdate(FDSP_MsgHdrTypePtr& msg_hdr,
				     FDSP_DMT_TypePtr& dmt_info) {
  om_client->recvDMTUpdate(dmt_info->DMT_version, dmt_info->DMT);
}

void OMgrClientRPCI::SetThrottleLevel(FDSP_MsgHdrTypePtr& msg_hdr, 
                                      FDSP_ThrottleMsgTypePtr& throttle_msg) {
  om_client->recvSetThrottleLevel((const float) throttle_msg->throttle_level);
}

void
OMgrClientRPCI::TierPolicy(FDSP_TierPolicyPtr &tier)
{
  FDS_PLOG_SEV(om_client->omc_log, fds::fds_log::notification)
    << "OMClient received tier policy for vol "
    << tier->tier_vol_uuid;

    fds_verify(om_client->omc_srv_pol != nullptr);
    om_client->omc_srv_pol->serv_recvTierPolicyReq(tier);
}

void
OMgrClientRPCI::TierPolicyAudit(FDSP_TierPolicyAuditPtr &audit)
{
  FDS_PLOG_SEV(om_client->omc_log, fds::fds_log::notification)
    << "OMClient received tier audit policy for vol "
    << audit->tier_vol_uuid;

    fds_verify(om_client->omc_srv_pol != nullptr);
    om_client->omc_srv_pol->serv_recvTierPolicyAuditReq(audit);
}

void OMgrClientRPCI::NotifyBucketStats(FDSP_MsgHdrTypePtr& msg_hdr,
				       FDSP_BucketStatsRespTypePtr& buck_stats_msg)
{
  fds_verify(msg_hdr->dst_id == FDS_ProtocolInterface::FDSP_STOR_HVISOR);
  om_client->recvBucketStats(msg_hdr, buck_stats_msg);
}

OMgrClient::OMgrClient(FDSP_MgrIdType node_type,
                       const std::string& _omIpStr,
                       fds_uint32_t _omPort,
                       const std::string& _hostIp,
                       fds_uint32_t data_port,
                       const std::string& node_name,
                       fds_log *parent_log,
                       boost::shared_ptr<netSessionTbl> nst) {
  my_node_type = node_type;
  omIpStr      = _omIpStr;
  omConfigPort = _omPort;
  hostIp       = _hostIp;
  my_data_port = data_port;
  my_node_name = node_name;
  node_evt_hdlr = NULL;
  vol_evt_hdlr = NULL;
  throttle_cmd_hdlr = NULL;
  bucket_stats_cmd_hdlr = NULL;
  if (parent_log) {
    omc_log = parent_log;
  } else {
    omc_log = new fds_log("omc", "logs");
  }
  nst_ = nst;
  initRPCComm();
}

OMgrClient::OMgrClient() {
  my_node_type = FDSP_STOR_HVISOR;
  my_node_name = "localhost-sh";
  node_evt_hdlr = NULL;
  vol_evt_hdlr = NULL;
  throttle_cmd_hdlr = NULL;
  bucket_stats_cmd_hdlr = NULL;
  omc_log = new fds_log("omc", "logs");
  initRPCComm();
}

OMgrClient::~OMgrClient()
{
    omrpc_handler_session_->endSession();
    omrpc_handler_thread_->join();
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

int OMgrClient::registerBucketStatsCmdHandler(bucket_stats_cmd_handler_t cmd_hdlr) {
  bucket_stats_cmd_hdlr = cmd_hdlr;
  return 0;
}

int OMgrClient::initRPCComm() {

  int argc = 0;
  char **argv = 0;

  // TODO: Load config using libconfig
  //initData.properties->load("om_client.conf");

  /*
   * If the OM's IP wasn't given via cmdline
   * pull it from the config file.
   */
  if (omIpStr.empty() == true) {
      // TODO: 
      // omIpStr = rpc_props->getProperty("OMgr.IP");
  }

  if (hostIp.empty() == true) {
      // TODO
      // hostIp = rpc_props->getProperty("OMgrClient.MyIPAddr");
  }
  /*
   * If the OM's config port wasn't given via cmdline
   * pull it from the config file
   */
  if (omConfigPort == 0) {
      //TODO
      // omConfigPort = strtoul(rpc_props->getProperty("OMgr.ConfigPort").c_str(),
      //                     NULL,
      //                     0);
  }
  
  return (0);

}  

/**
 * @brief Starts OM RPC handling server.  This function is to be run on a
 * separate thread.  OMgrClient destructor does a join() on this thread
 */
void OMgrClient::start_omrpc_handler()
{
    nst_->listenServer(omrpc_handler_session_);
}

// Call this to setup the (receiving side) endpoint to lister for control path requests from OM.
int OMgrClient::startAcceptingControlMessages() {
  return startAcceptingControlMessages(0);
}

int OMgrClient::startAcceptingControlMessages(fds_uint32_t port_num) {

  my_control_port = port_num;
  if (my_control_port == 0) {
      // TODO: config 
      // my_control_port = rpc_props->getPropertyAsInt("OMgrClient.PortNumber");
      fds_verify(0);
  }
  std::string myIp = netSession::getLocalIp();
  int myIpInt = netSession::ipString2Addr(myIp);
  omrpc_handler_.reset(new OMgrClientRPCI(this));
  // TODO: Ideally createServerSession should take a shared pointer
  // for omrpc_handler_.  Make sure that happens.  Otherwise you
  // end up with a pointer leak.
  omrpc_handler_session_ = 
      nst_->createServerSession<netControlPathServerSession>(myIpInt,
                                my_control_port, 
                                my_node_name,
                                FDSP_ORCH_MGR,
                                omrpc_handler_);

  omrpc_handler_thread_.reset(new std::thread(&OMgrClient::start_omrpc_handler, this));

#if 0
  //om_client_rpc_i.reset(new OMgrClientRPCI(this));
  boost::shared_ptr<OMgrClientRPCI> omc_handler(new OMgrClientRPCI(this));
  boost::shared_ptr<apache::thrift::TProcessor>
          processor(new FDSP_ControlPathReqProcessor(omc_handler));
  boost::shared_ptr<apache::thrift::transport::TServerTransport>
          serverTransport(new apache::thrift::transport::TServerSocket(my_control_port));
  boost::shared_ptr<apache::thrift::transport::TTransportFactory>
          transportFactory(new apache::thrift::transport::TBufferedTransportFactory());
  boost::shared_ptr<apache::thrift::protocol::TProtocolFactory>
          protocolFactory(new apache::thrift::protocol::TBinaryProtocolFactory());

  apache::thrift::server::TSimpleServer
          *server = new apache::thrift::server::TSimpleServer(processor,
                                                              serverTransport,
                                                              transportFactory,
                                                              protocolFactory);

#endif

 
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
int OMgrClient::registerNodeWithOM(const FDS_ProtocolInterface::FDSP_AnnounceDiskCapabilityPtr& 
                                   dInfo) {

  try {
      omclient_prx_session_ = nst_->startSession<netOMControlPathClientSession>(omIpStr,
                                    omConfigPort,
                                    FDSP_ORCH_MGR,
                                    1, /* number of channels */
                                   boost::shared_ptr<FDSP_OMControlPathRespIf>()/* TODO:  pass in response path server pointer */); 
      
      // Just return if the om ptr is NULL because
      // FDS-net doesn't throw the exception we're
      // trying to catch. We should probably return
      // an error and let the caller decide what to do.
      // fds_verify(omclient_prx_session_ != nullptr);
      if (omclient_prx_session_ == NULL) {
          FDS_PLOG_SEV(omc_log, fds::fds_log::critical) << "OMClient unable to register node with OrchMgr. Please check if OrchMgr is up and restart.";
          return 0;
      }
      om_client_prx = omclient_prx_session_->getClient();  // NOLINT
#if 0
   boost::shared_ptr<apache::thrift::transport::TTransport>
           socket(new apache::thrift::transport::TSocket(omIpStr, omConfigPort));
   boost::shared_ptr<apache::thrift::transport::TTransport>
           transport(new apache::thrift::transport::TBufferedTransport(socket));
   boost::shared_ptr<apache::thrift::protocol::TProtocol>
           protocol(new apache::thrift::protocol::TBinaryProtocol(transport));
  
   om_client_prx = new FDS_ProtocolInterface::FDSP_OMControlPathReqClient(protocol);
   transport->open();
#endif
      
   FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
   initOMMsgHdr(msg_hdr);
   FDSP_RegisterNodeTypePtr reg_node_msg(new FDSP_RegisterNodeType);
   reg_node_msg->node_type = my_node_type;
   reg_node_msg->node_name = my_node_name;
   reg_node_msg->ip_hi_addr = 0;
   reg_node_msg->ip_lo_addr = fds::str_to_ipv4_addr(hostIp); //my_address!
   reg_node_msg->control_port = my_control_port;
   reg_node_msg->data_port = my_data_port; // for now
   /* init the disk info */
   
   reg_node_msg->disk_info.disk_iops_max =  dInfo->disk_iops_max;
   reg_node_msg->disk_info.disk_iops_min =  dInfo->disk_iops_min;
   reg_node_msg->disk_info.disk_capacity = dInfo->disk_capacity;
   reg_node_msg->disk_info.disk_latency_max = dInfo->disk_latency_max;
   reg_node_msg->disk_info.disk_latency_min = dInfo->disk_latency_min;
   reg_node_msg->disk_info.ssd_iops_max =  dInfo->ssd_iops_max;
   reg_node_msg->disk_info.ssd_iops_min =  dInfo->ssd_iops_min;
   reg_node_msg->disk_info.ssd_capacity = dInfo->ssd_capacity;
   reg_node_msg->disk_info.ssd_latency_max = dInfo->ssd_latency_min;
   reg_node_msg->disk_info.ssd_latency_min = dInfo->ssd_latency_min;
   reg_node_msg->disk_info.disk_type = dInfo->disk_type;

   FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient registering local node " 
						     << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr) 
						     << " control port:" << reg_node_msg->control_port 
						     << " data port:" << reg_node_msg->data_port
						     << " with Orchaestration Manager at "
                                                     << omIpStr << ":" << omConfigPort;

  om_client_prx->RegisterNode(msg_hdr, reg_node_msg);

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

    FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    initOMMsgHdr(msg_hdr);
    FDSP_PerfstatsTypePtr perf_stats_msg(new FDSP_PerfstatsType);
    perf_stats_msg->node_type = my_node_type;
    perf_stats_msg->start_timestamp = start_ts;
    perf_stats_msg->slot_len_sec = stat_slot_len;
    perf_stats_msg->vol_hist_list = hist_list;  

    FDS_PLOG_SEV(omc_log, fds::fds_log::normal) << "OMClient pushing perfstats to OM at "
                                                << omIpStr << ":" << omConfigPort
						<< " start ts " << start_ts;

    om_client_prx->NotifyPerfstats(msg_hdr, perf_stats_msg);

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

    FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    initOMMsgHdr(msg_hdr);
    FDSP_TestBucketPtr test_buck_msg(new FDSP_TestBucket);
    test_buck_msg->bucket_name = bucket_name;
    test_buck_msg->vol_info = *vol_info;
    test_buck_msg->attach_vol_reqd = attach_vol_reqd;
    test_buck_msg->accessKeyId = accessKeyId;
    test_buck_msg->secretAccessKey;

    FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient sending test bucket request to OM at "
                                                      << omIpStr << ":" << omConfigPort;

    om_client_prx->TestBucket(msg_hdr, test_buck_msg);
  }
  catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to send testBucket request to OM. Check if OM is up and restart.";
    return -1;
  }

  return 0;
}

int OMgrClient::pushGetBucketStatsToOM(fds_uint32_t req_cookie)
{
  try {
    FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    initOMMsgHdr(msg_hdr);
    msg_hdr->req_cookie = req_cookie;

    FDSP_GetDomainStatsTypePtr get_stats_msg(new FDSP_GetDomainStatsType());
    get_stats_msg->domain_id = 1; /* this is ignored in OM */

    FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient sending get bucket stats request to OM at "
                                                      << omIpStr << ":" << omConfigPort;


    om_client_prx->GetDomainStats(msg_hdr, get_stats_msg);
  }
  catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to send GetBucketStats request to OM. Check if OM is up and restart.";
    return -1;
  }

  return 0;
}

int OMgrClient::pushCreateBucketToOM(const FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr& volInfo)
{
  try {
         FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
         initOMMsgHdr(msg_hdr);
  
         FDSP_CreateVolTypePtr volData(new FDSP_CreateVolType());

         volData->vol_name = volInfo->vol_name;
         volData->vol_info.vol_name = volInfo->vol_name;
         volData->vol_info.tennantId = volInfo->tennantId;
         volData->vol_info.localDomainId = volInfo->localDomainId;
         volData->vol_info.globDomainId = volInfo->globDomainId;

         volData->vol_info.capacity = volInfo->capacity;
         volData->vol_info.maxQuota = volInfo->maxQuota;
         volData->vol_info.volType = volInfo->volType;

         volData->vol_info.defReplicaCnt = volInfo->defReplicaCnt;
         volData->vol_info.defWriteQuorum = volInfo->defWriteQuorum;
         volData->vol_info.defReadQuorum = volInfo->defReadQuorum;
         volData->vol_info.defConsisProtocol = volInfo->defConsisProtocol;

         volData->vol_info.volPolicyId = 50; //  default policy 
         volData->vol_info.archivePolicyId = volInfo->archivePolicyId;
         volData->vol_info.placementPolicy = volInfo->placementPolicy;
         volData->vol_info.appWorkload = volInfo->appWorkload;

    	 om_client_prx->CreateBucket(msg_hdr, volData);
  } catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to push  the create bucket request to OM. Check if OM is up and restart.";
    return -1;
  }

   /* do attach volume for this bucket */
  try {
      FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
      initOMMsgHdr(msg_hdr);

      FDSP_AttachVolCmdTypePtr volData(new FDSP_AttachVolCmdType());

      volData->vol_name = volInfo->vol_name; 
      om_client_prx->AttachBucket(msg_hdr, volData);
  } catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to push  the attach  bucket to  OM. Check if OM is up and restart.";
    return -1;
  }

  return 0;
}

int OMgrClient::pushModifyBucketToOM(const std::string& bucket_name,
				     const FDS_ProtocolInterface::FDSP_VolumeDescTypePtr& vol_desc)
{
  try {

    FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    initOMMsgHdr(msg_hdr);
    FDSP_ModifyVolTypePtr mod_vol_msg(new FDSP_ModifyVolType());
    mod_vol_msg->vol_name = bucket_name;
    mod_vol_msg->vol_uuid = 0; /* make sure that uuid is not checked, because we don't know it here */
    mod_vol_msg->vol_desc = *vol_desc;

    FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient sending modify bucket request to OM at "
                                                      << omIpStr << ":" << omConfigPort;

    om_client_prx->ModifyBucket(msg_hdr, mod_vol_msg);
  }
  catch (...) {
    FDS_PLOG_SEV(omc_log, fds::fds_log::error) << "OMClient unable to send ModifyBucket request to OM. Check if OM is up and restart.";
    return -1;
  }

  return 0;
}

int OMgrClient::pushDeleteBucketToOM(const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr& volInfo)
{
  try {
        FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
        initOMMsgHdr(msg_hdr);

 	FDSP_DeleteVolTypePtr volData(new FDSP_DeleteVolType());
   	volData->vol_name  = volInfo->vol_name;
  	volData->domain_id = volInfo->domain_id;
    	om_client_prx->DeleteBucket(msg_hdr, volData);

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

int OMgrClient::recvBucketStats(const FDSP_MsgHdrTypePtr& msg_hdr, 
				const FDSP_BucketStatsRespTypePtr& stats_msg)
{
  FDS_PLOG_SEV(omc_log, fds::fds_log::notification) << "OMClient received buckets' stats with timestamp  " 
						    << stats_msg->timestamp;

  if (bucket_stats_cmd_hdlr) {
    bucket_stats_cmd_hdlr(msg_hdr, stats_msg);
  }

  return 0;
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

int OMgrClient::getDLTNodesForDoidKey(unsigned char doid_key,
                                      fds_int32_t *node_ids,
                                      fds_int32_t *n_nodes) {
  
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
