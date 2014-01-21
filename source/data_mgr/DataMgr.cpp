/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "DataMgr.h"

namespace fds {

DataMgr *dataMgr;

void DataMgr::vol_handler(fds_volid_t vol_uuid,
                          VolumeDesc *desc,
                          fds_vol_notify_t vol_action,
			  FDS_ProtocolInterface::FDSP_ResultType result) {
  Error err(ERR_OK);
  FDS_PLOG(dataMgr->GetLog()) << "Received vol notif from OM for "
                              << vol_uuid <<  desc->getName();

  /*
   * TODO: Check the vol action to see whether to add
   * or rm the vol
   */
  if (vol_action == fds_notify_vol_add) {
    /*
     * TODO: Actually take a volume string name, not
     * just the volume number.
     */
    err = dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                    std::to_string(vol_uuid),
                                    vol_uuid,desc);
  } else if (vol_action == fds_notify_vol_rm) {
    err = dataMgr->_process_rm_vol(vol_uuid);
  }
  else if (vol_action == fds_notify_vol_mod) {
    FDS_PLOG(dataMgr->GetLog()) << "Received vol modify from OM for "
				<< std::to_string(vol_uuid);
    err = dataMgr->_process_mod_vol(vol_uuid, *desc);
 
  } else {
    assert(0);
  }

  /*
   * TODO: We're dropping the error at the moment.
   * We should respond with the error code to OM.
   */
}

void DataMgr::node_handler(fds_int32_t  node_id,
                           fds_uint32_t node_ip,
                           fds_int32_t  node_st,
                           fds_uint32_t node_port,
                           FDS_ProtocolInterface::FDSP_MgrIdType node_type) {
}

/*
 * Adds the volume if it doesn't exist already.
 * Note this does NOT return error if the volume exists.
 */
Error DataMgr::_add_if_no_vol(const std::string& vol_name,
                              fds_volid_t vol_uuid,VolumeDesc *desc) {
  Error err(ERR_OK);

  /*
   * Check if we already know about this volume
   */
  vol_map_mtx->lock();
  if (volExistsLocked(vol_uuid) == true) {
    FDS_PLOG(dataMgr->GetLog()) << "Received add request for existing vol uuid "
                                << vol_uuid << ", so ignoring.";
    vol_map_mtx->unlock();
    return err;
  }

  vol_map_mtx->unlock();

  err = _add_vol_locked(vol_name, vol_uuid,desc);


  return err;
}

/*
 * Meant to be called holding the vol_map_mtx.
 */
Error DataMgr::_add_vol_locked(const std::string& vol_name,
                               fds_volid_t vol_uuid,VolumeDesc *vdesc) {
  Error err(ERR_OK);

  vol_map_mtx->lock();
  vol_meta_map[vol_uuid] = new VolumeMeta(vol_name,
                                          vol_uuid,
                                          dm_log,vdesc);
  FDS_PLOG(dataMgr->GetLog()) << "Added vol meta for vol uuid and per Volume queue"
                              << vol_uuid;

  VolumeMeta *vm = vol_meta_map[vol_uuid];
  vm->dmVolQueue = new FDS_VolumeQueue(4096, vdesc->iops_max, 2*vdesc->iops_min, vdesc->relativePrio);
  vm->dmVolQueue->activate();
  dataMgr->qosCtrl->registerVolume(vol_uuid, dynamic_cast<FDS_VolumeQueue*>(vm->dmVolQueue));

  vol_map_mtx->unlock();
  return err;
}

Error DataMgr::_process_add_vol(const std::string& vol_name,
                                fds_volid_t vol_uuid,VolumeDesc *desc) {
  Error err(ERR_OK);

  /*
   * Verify that we don't already know about this volume
   */
  vol_map_mtx->lock();
  if (volExistsLocked(vol_uuid) == true) {
    FDS_PLOG(dataMgr->GetLog()) << "Received add request for existing vol uuid "
                                << vol_uuid;
    err = ERR_DUPLICATE;
    vol_map_mtx->unlock();
    return err;
  }

  vol_map_mtx->unlock();

  err = _add_vol_locked(vol_name, vol_uuid,desc);


  return err;
}

Error DataMgr::_process_mod_vol(fds_volid_t vol_uuid, const VolumeDesc& voldesc)
{
  Error err(ERR_OK);

  vol_map_mtx->lock();
  /* make sure volume exists */
  if (volExistsLocked(vol_uuid) == false) {
    FDS_PLOG_SEV(dataMgr->GetLog(), fds::fds_log::error) << "Received modify policy request for "
							 << "non-existant volume [" << vol_uuid 
							 << ", " << voldesc.name << "]" ;
    err = Error(ERR_NOT_FOUND);
    vol_map_mtx->unlock();
    return err;
  }
  VolumeMeta *vm = vol_meta_map[vol_uuid];
  vm->vol_desc->modifyPolicyInfo(2*voldesc.iops_min, voldesc.iops_max, voldesc.relativePrio);
  err = qosCtrl->modifyVolumeQosParams(vol_uuid, 2*voldesc.iops_min, voldesc.iops_max, voldesc.relativePrio);
  vol_map_mtx->unlock();

  FDS_PLOG_SEV(dataMgr->GetLog(), fds::fds_log::notification) << "Modify policy for volume "
							      << voldesc.name << " RESULT: " << err.GetErrstr();

  return err;
}

Error DataMgr::_process_rm_vol(fds_volid_t vol_uuid) {
  Error err(ERR_OK);

  /*
   * Make sure we already know about this volume
   */
  vol_map_mtx->lock();
  if (volExistsLocked(vol_uuid) == false) {
    FDS_PLOG(dataMgr->GetLog()) << "Received add request for "
                                << "non-existant vol uuid " << vol_uuid;
    err = ERR_INVALID_ARG;
    vol_map_mtx->unlock();
    return err;
  }

  VolumeMeta *vm = vol_meta_map[vol_uuid];
  vol_meta_map.erase(vol_uuid);
  dataMgr->qosCtrl->deregisterVolume(vol_uuid);
  delete vm->dmVolQueue;
  delete vm;

  FDS_PLOG(dataMgr->GetLog()) << "Removed vol meta for vol uuid "
                              << vol_uuid;
  vol_map_mtx->unlock();

  return err;
}

Error DataMgr::_process_open(fds_volid_t vol_uuid,
                             std::string blob_name,
                             fds_uint32_t trans_id,
                             const BlobNode*& bnode) {
  Error err(ERR_OK);

  /*
   * Get a local reference to the vol meta.
   */
  vol_map_mtx->lock();
  VolumeMeta *vol_meta = vol_meta_map[vol_uuid];
  vol_map_mtx->unlock();


  /*
   * Check the map to see if we know about the volume
   * and just add it if we don't.
   * TODO: We should not implicitly create the volume here!
   * We should only be creating the volume on OM volume create
   * requests.
   * TODO: Just hack the name as the offset for now.
   */
  err = _add_if_no_vol(stor_prefix +
                       std::to_string(vol_uuid),
                       vol_uuid, vol_meta->vol_desc);
  if (!err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Failed to add vol during open "
                                << "transaction for volume " << vol_uuid;

    return err;
  }


  err = vol_meta->OpenTransaction(blob_name, bnode, vol_meta->vol_desc);

  if (err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Opened transaction for volume "
                                << vol_uuid << ",  blob "
                                << blob_name << " and mapped to bnode "
                                << bnode->ToString();
  } else {
    FDS_PLOG(dataMgr->GetLog()) << "Failed to open transaction for volume "
                                << vol_uuid;
  }

  return err;
}

Error DataMgr::_process_commit(fds_volid_t vol_uuid,
                               std::string blob_name,
                               fds_uint32_t trans_id,
                               const BlobNode*& bnode) {
  Error err(ERR_OK);

  /*
   * Here we should be updating the TVC to reflect the commit
   * and adding back to the VVC. The TVC can be an in-memory
   * update.
   * For now, we don't need to do anything because it was put
   * into the VVC on open.
   */
  FDS_PLOG(dataMgr->GetLog()) << "Committed transaction for volume "
                              << vol_uuid << " , blob "
                              << blob_name << " and mapped to bnode "
                              << bnode->ToString();

  return err;
}

Error DataMgr::_process_abort() {
  Error err(ERR_OK);

  /*
   * TODO: Here we should be determining the state of the
   * transaction and removing the entry from the TVC.
   */

  return err;
}

Error DataMgr::_process_list(fds_volid_t volId,
                             std::list<BlobNode>& bNodeList) {
  Error err(ERR_OK);

  /*
   * Get a local reference to the vol meta.
   */
  vol_map_mtx->lock();
  VolumeMeta *vol_meta = vol_meta_map[volId];
  vol_map_mtx->unlock();
  fds_verify(vol_meta != NULL);

  /*
   * Check the map to see if we know about the volume
   * and just add it if we don't.
   * TODO: We should not implicitly create the volume here!
   * We should only be creating the volume on OM volume create
   * requests.
   * TODO: Just hack the name as the offset for now.
   */
  err = _add_if_no_vol(stor_prefix +
                       std::to_string(volId),
                       volId, vol_meta->vol_desc);
  if (!err.ok()) {
    FDS_PLOG(dm_log) << "Failed to add vol during list blob for volume " << volId;
    return err;
  }

  err = vol_meta->listBlobs(bNodeList);
  if (err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Vol meta list blobs for volume "
                                << volId << " returned " << bNodeList.size()
                                << " blobs";
  } else {
    FDS_PLOG(dataMgr->GetLog()) << "Vol meta list blobs FAILED for volume "
                                << volId;
  }

  return err;
}

Error DataMgr::_process_query(fds_volid_t vol_uuid,
                              std::string blob_name,
                              BlobNode*& bnode) {
  Error err(ERR_OK);
  /*
   * Get a local reference to the vol meta.
   */
  vol_map_mtx->lock();
  VolumeMeta *vol_meta = vol_meta_map[vol_uuid];
  vol_map_mtx->unlock();

  /*
   * Check the map to see if we know about the volume
   * and just add it if we don't.
   * TODO: We should not implicitly create the volume here!
   * We should only be creating the volume on OM volume create
   * requests.
   * TODO: Just hack the name as the offset for now.
   */
  err = _add_if_no_vol(stor_prefix +
                       std::to_string(vol_uuid),
                       vol_uuid, vol_meta->vol_desc);
  if (!err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Failed to add vol during query "
                                << "transaction for volume " << vol_uuid;
    return err;
  }


  err = vol_meta->QueryVcat(blob_name, bnode);

  if (err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Vol meta query for volume "
                                << vol_uuid << " , blob "
                                << blob_name << " found bnode " << bnode->ToString();
  } else {
    FDS_PLOG(dataMgr->GetLog()) << "Vol meta query FAILED for volume "
                                << vol_uuid;
  }

  return err;
}


Error DataMgr::_process_delete(fds_volid_t vol_uuid,
                              std::string blob_name) {
  Error err(ERR_OK);
  /*
   * Get a local reference to the vol meta.
   */
  vol_map_mtx->lock();
  VolumeMeta *vol_meta = vol_meta_map[vol_uuid];
  vol_map_mtx->unlock();

  err = vol_meta->DeleteVcat(blob_name);

  if (err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Vol meta Delete for volume "
                                << vol_uuid << " , blob "
                                << blob_name ;
  } else {
    FDS_PLOG(dataMgr->GetLog()) << "Vol meta delete FAILED for volume "
                                << vol_uuid;
  }

  return err;
}

DataMgr::DataMgr(int argc, char *argv[],
             const std::string &default_config_path,
             const std::string &base_path)
    :
        FdsProcess(argc, argv, default_config_path, base_path),
    Module("Data Manager"),
    port_num(0),
    cp_port_num(0),
    omConfigPort(0),
    use_om(true),
    numTestVols(10),
    runMode(NORMAL_MODE),
    scheduleRate(4000),
    num_threads(DM_TP_THREADS) {
  dm_log = new fds_log("dm", "logs");
  vol_map_mtx = new fds_mutex("Volume map mutex");

  _tp = new fds_threadpool(num_threads);

  /*
   * Comm with OM will be setup during run()
   */
  omClient = NULL;

  /*
   *  init Data Manager  QOS class.
   */
  qosCtrl = new dmQosCtrl(this, 20, FDS_QoSControl::FDS_DISPATCH_WFQ, dm_log);
  qosCtrl->runScheduler();

  FDS_PLOG(dm_log) << "Constructing the Data Manager";
}

DataMgr::~DataMgr() {
  FDS_PLOG(dm_log) << "Destructing the Data Manager";

  /*
   * This will wait for all current threads to
   * complete.
   */
  delete _tp;

  for (std::unordered_map<fds_uint64_t, VolumeMeta*>::iterator
           it = vol_meta_map.begin();
       it != vol_meta_map.end();
       it++) {
    delete it->second;
  }
  vol_meta_map.clear();

  delete omClient;
  delete vol_map_mtx;
  delete dm_log;
  delete qosCtrl;
}

namespace util {
/**
 * @return local ip
 */
std::string get_local_ip()
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.
     * This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
                if (myIp.find("10.1") != std::string::npos)
                    break; /* TODO: more dynamic */
            }
        }
    }

    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }

    return myIp;
}
}

int DataMgr::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void DataMgr::mod_startup() {    
}

void DataMgr::mod_shutdown() {
}

//void DataMgr::runServer() {
  /*
   * TODO: Replace this when we pull ICE out.
   */
  //  this->main(mod_params->p_argc, mod_params->p_argv, "dm_test.conf");
    //  this->run(mod_params->p_argc, mod_params->p_argv);

//}

void DataMgr::run()
{
  try {
      nstable->listenServer(metadatapath_session);
  } 
  catch (...) {
      std::cout << "starting server threw an exception" << std::endl;
  }
}

void DataMgr::setup_metadatapath_server(const std::string &ip)
{
    metadatapath_handler.reset(new ReqHandler());

    int myIpInt = netSession::ipString2Addr(ip);
    std::string node_name = 
        conf_helper_.get<std::string>("prefix") + "_DM_" + ip;
    // TODO: Ideally createServerSession should take a shared pointer
    // for datapath_handler.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    // TODO: Figure out who cleans up datapath_session_
    metadatapath_session = static_cast<netMetaDataPathServerSession*>(
        nstable->createServerSession(myIpInt,
                                     port_num,
                                     node_name,
                                     FDSP_STOR_HVISOR,
                                     metadatapath_handler.get()));
}


void DataMgr::setup(int argc, char* argv[], fds::Module **mod_vec) {

  fds::DmDiskInfo     *info;
  fds::DmDiskQuery     in;
  fds::DmDiskQueryOut  out;
  fds_bool_t      useTestMode = false;

  /*
   * Process the cmdline args.
   */

  /*
   * Invoke FdsProcess setup so that it can setup the signal hander and
   * execute the module vector for us
   */

  runMode = NORMAL_MODE;

  FdsProcess::setup(argc, argv, mod_vec);

  port_num = conf_helper_.get_abs<int>("fds.dm.port");
  cp_port_num = conf_helper_.get_abs<int>("fds.dm.cp_port");
  stor_prefix = conf_helper_.get_abs<std::string>("fds.dm.prefix");
  use_om = !(conf_helper_.get_abs<bool>("fds.dm.no_om"));
  omIpStr = conf_helper_.get_abs<std::string>("fds.dm.om_ip");
  omConfigPort = conf_helper_.get_abs<int>("fds.dm.om_port");
  useTestMode = conf_helper_.get_abs<bool>("fds.dm.test_mode");
  int sev_level = conf_helper_.get_abs<int>("fds.dm.log_severity");
  
  GetLog()->setSeverityFilter(( fds_log::severity_level)sev_level);
  if (useTestMode == true) {
    runMode = TEST_MODE;
  }

  if (port_num == 0) {
      /* set default port */
      port_num = 7902;
  }
  FDS_PLOG(dm_log) << "Data Manager using port " << port_num;

  FDS_PLOG(dm_log) << "Data Manager using storage prefix "
                   << stor_prefix;

  if (cp_port_num == 0) {
    /*
      set default port
     */
    cp_port_num = 7903;
  }
  FDS_PLOG(dm_log) << "Data Manager using control port "
                   << cp_port_num;


  /* Set up FDSP RPC endpoints */
  nstable = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_DATA_MGR));
  myIp = util::get_local_ip();
  assert(myIp.empty() == false);
  std::string node_name = 
          conf_helper_.get<std::string>("prefix") + "_DM_" + myIp;

  FDS_PLOG(dm_log) << "Data Manager using IP:"
                   << myIp << " and node name "
                   << node_name;
  
  setup_metadatapath_server(myIp);

  /*
   * Query  Disk Manager  for disk parameter details 
   */
  fds::DmQuery        &query = fds::DmQuery::dm_query();
  in.dmq_mask = fds::dmq_disk_info;
  query.dm_disk_query(in, &out);
  /* we should be bundling multiple disk  parameters  into one message to OM TBD */ 
  dInfo.reset(new FDSP_AnnounceDiskCapability());
  while (1) {
        info = out.query_pop();
        if (info != nullptr) {
  	    FDS_PLOG(dm_log) << "Max blks capacity: " << info->di_max_blks_cap
            << "Disk type........: " << info->di_disk_type
            << "Max iops.........: " << info->di_max_iops
            << "Min iops.........: " << info->di_min_iops
            << "Max latency (us).: " << info->di_max_latency
            << "Min latency (us).: " << info->di_min_latency;

            if ( info->di_disk_type == FDS_DISK_SATA) {
            	dInfo->disk_iops_max =  info->di_max_iops; /*  max avarage IOPS */
            	dInfo->disk_iops_min =  info->di_min_iops; /* min avarage IOPS */
            	dInfo->disk_capacity = info->di_max_blks_cap;  /* size in blocks */
            	dInfo->disk_latency_max = info->di_max_latency; /* in us second */
            	dInfo->disk_latency_min = info->di_min_latency; /* in us second */
  	    } else if (info->di_disk_type == FDS_DISK_SSD) {
            	dInfo->ssd_iops_max =  info->di_max_iops; /*  max avarage IOPS */
            	dInfo->ssd_iops_min =  info->di_min_iops; /* min avarage IOPS */
            	dInfo->ssd_capacity = info->di_max_blks_cap;  /* size in blocks */
            	dInfo->ssd_latency_max = info->di_max_latency; /* in us second */
            	dInfo->ssd_latency_min = info->di_min_latency; /* in us second */
	    } else 
  	       FDS_PLOG(dm_log) << "Unknown Disk Type " << info->di_disk_type;
			
            delete info;
            continue;
        }
        break;
  }


  if (use_om) {
      FDS_PLOG(dm_log) << " Initialising the OM client ";
      /*
       * Setup communication with OM.
       */
      omClient = new OMgrClient(FDSP_DATA_MGR,
                                omIpStr,
                                omConfigPort,
                                myIp,
                                port_num,
                                node_name,
                                dm_log);
      omClient->initialize();
      omClient->registerEventHandlerForNodeEvents(node_handler);
      omClient->registerEventHandlerForVolEvents(vol_handler);
      /*
       * Brings up the control path interface.
       * This does not require OM to be running and can
       * be used for testing DM by itself.
       */
      omClient->startAcceptingControlMessages(cp_port_num);
      /*
       * Registers the DM with the OM. Uses OM for bootstrapping
       * on start. Requires the OM to be up and running prior.
       */
      omClient->registerNodeWithOM(dInfo);
  }
  
  if (runMode == TEST_MODE) {
    /*
     * Create test volumes.
     */
      std::string testVolName;
      VolumeDesc*  testVdb;
      for (fds_uint32_t testVolId = 1; testVolId < numTestVols + 1; testVolId++) {
          testVolName = "testVol" + std::to_string(testVolId);
          /*
           * We're using the ID as the min/max/priority
           * for the volume QoS.
           */
          testVdb = new VolumeDesc(testVolName,
                                   testVolId,
                                   testVolId,
                                   testVolId * 2,
                                   testVolId);
          fds_assert(testVdb != NULL);
          vol_handler(testVolId, testVdb, fds_notify_vol_add, FDS_ProtocolInterface::FDSP_ERR_OK); 
          
          delete testVdb;
      }
  }
  
}

void DataMgr::swapMgrId(const FDS_ProtocolInterface::
                        FDSP_MsgHdrTypePtr& fdsp_msg) {
  FDS_ProtocolInterface::FDSP_MgrIdType temp_id;
  temp_id = fdsp_msg->dst_id;
  fdsp_msg->dst_id = fdsp_msg->src_id;
  fdsp_msg->src_id = temp_id;

  fds_uint64_t tmp_addr;
  tmp_addr = fdsp_msg->dst_ip_hi_addr;
  fdsp_msg->dst_ip_hi_addr = fdsp_msg->src_ip_hi_addr;
  fdsp_msg->src_ip_hi_addr = tmp_addr;

  tmp_addr = fdsp_msg->dst_ip_lo_addr;
  fdsp_msg->dst_ip_lo_addr = fdsp_msg->src_ip_lo_addr;
  fdsp_msg->src_ip_lo_addr = tmp_addr;

  fds_uint32_t tmp_port;
  tmp_port = fdsp_msg->dst_port;
  fdsp_msg->dst_port = fdsp_msg->src_port;
  fdsp_msg->src_port = tmp_port;
}

fds_log* DataMgr::GetLog() {
  return dm_log;
}

std::string DataMgr::getPrefix() const {
  return stor_prefix;
}

/*
 * Intended to be called while holding the vol_map_mtx.
 */
fds_bool_t DataMgr::volExistsLocked(fds_volid_t vol_uuid) const {
  if (vol_meta_map.count(vol_uuid) != 0) {
    return true;
  }
  return false;
}

fds_bool_t DataMgr::volExists(fds_volid_t vol_uuid) const {
  fds_bool_t result;
  vol_map_mtx->lock();
  result = volExistsLocked(vol_uuid);
  vol_map_mtx->unlock();

  return result;
}

void DataMgr::interrupt_cb(int signum) {
  FDS_PLOG(dataMgr->GetLog()) << " Received signal " 
                              << signum << ". Shutting down communicator";
  exit(0);
}

DataMgr::ReqHandler::ReqHandler() {
}

DataMgr::ReqHandler::~ReqHandler() {
}

void DataMgr::ReqHandler::GetVolumeBlobList(FDSP_MsgHdrTypePtr& msg_hdr, 
                                            FDSP_GetVolumeBlobListReqTypePtr& blobListReq) {

  Error err(ERR_OK);

  err = dataMgr->blobListInternal(blobListReq, msg_hdr->glob_volume_id,
                                  msg_hdr->src_ip_lo_addr, msg_hdr->dst_ip_lo_addr, msg_hdr->src_port,
                                  msg_hdr->dst_port, msg_hdr->src_node_name, msg_hdr->req_cookie);

  if (!err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Error Queueing the blob list request to per volume Queue";
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;

    /*
     * Reverse the msg direction and send the response.
     */
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_VOL_BLOB_LIST_RSP;
    dataMgr->swapMgrId(msg_hdr);
    dataMgr->respMapMtx.read_lock();
    FDS_ProtocolInterface::FDSP_GetVolumeBlobListRespTypePtr
            blobListResp(new FDS_ProtocolInterface::FDSP_GetVolumeBlobListRespType);
    blobListResp->num_blobs_in_resp = 0;
    blobListResp->end_of_list = true;
    dataMgr->respHandleCli(msg_hdr->src_node_name)->GetVolumeBlobListResp(*msg_hdr,
									  *blobListResp);
    dataMgr->respMapMtx.read_unlock();

    FDS_PLOG_SEV(dataMgr->GetLog(), fds::fds_log::error) << "Sending async blob list error response with "
                                                         << "volume id: " << msg_hdr->glob_volume_id;
  }  
}

void
DataMgr::updateCatalogBackend(dmCatReq  *updCatReq) {
  Error err(ERR_OK);

  FDS_PLOG(dataMgr->GetLog()) << "Processing update catalog request with "
                              << "volume id: " << updCatReq->volId
                              << ", blob name: "
                              << updCatReq->blob_name
			      << ", Trans ID "
                              << updCatReq->transId
                              << ", OP ID " << updCatReq->transOp
                              << ", journ TXID " << updCatReq->reqCookie;

  BlobNode *bnode = new BlobNode(updCatReq->fdspUpdCatReqPtr, updCatReq->volId);

  /*
   * For now, just treat this as an open
   */
  if (updCatReq->transOp ==
      FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN) {
    err = dataMgr->_process_open((fds_volid_t)updCatReq->volId,
                                 updCatReq->blob_name,
                                 updCatReq->transId,
                                 (const BlobNode *&)bnode);
  } else if (updCatReq->transOp ==
             FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
    err = dataMgr->_process_commit(updCatReq->volId,
                                   updCatReq->blob_name,
                                   updCatReq->transId,
                                   (const BlobNode *&)bnode);
  } else {
    err = ERR_CAT_QUERY_FAILED;
  }

  if (bnode)
    delete bnode;

  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
  FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr
          update_catalog(new FDSP_UpdateCatalogType);
  DataMgr::InitMsgHdr(msg_hdr);
  update_catalog->obj_list.clear();
  update_catalog->meta_list.clear();

  if (err.ok()) {
    msg_hdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_msg = "Dude, you're good to go!";
  } else {
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
  }

  msg_hdr->src_ip_lo_addr =  updCatReq->dstIp;
  msg_hdr->dst_ip_lo_addr =  updCatReq->srcIp;
  msg_hdr->src_port =  updCatReq->dstPort;
  msg_hdr->dst_port =  updCatReq->srcPort;
  msg_hdr->glob_volume_id =  updCatReq->volId;
  msg_hdr->req_cookie =  updCatReq->reqCookie;

  update_catalog->blob_name = updCatReq->blob_name;
  update_catalog->dm_transaction_id = updCatReq->transId;
  update_catalog->dm_operation = updCatReq->transOp;
  /*
   * Reverse the msg direction and send the response.
   */
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;

  dataMgr->respMapMtx.read_lock();
  dataMgr->respHandleCli(updCatReq->src_node_name)->UpdateCatalogObjectResp(*msg_hdr, *update_catalog);
  dataMgr->respMapMtx.read_unlock();

  FDS_PLOG(dataMgr->GetLog()) << "Sending async update catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << update_catalog->blob_name
                              << ", Trans ID "
                              << update_catalog->dm_transaction_id
                              << ", OP ID " << update_catalog->dm_operation;

  if (update_catalog->dm_operation ==
      FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN) {
    FDS_PLOG(dataMgr->GetLog())
        << "End:Sent update response for trans open request";
  } else if (update_catalog->dm_operation ==
             FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
    FDS_PLOG(dataMgr->GetLog())
        << "End:Sent update response for trans commit request";
  }

  qosCtrl->markIODone(*updCatReq);
  delete updCatReq;

}


Error
DataMgr::updateCatalogInternal(FDSP_UpdateCatalogTypePtr updCatReq, 
			       fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
			       fds_uint32_t dstPort, std::string src_node_name, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

    
    /*
     * allocate a new update cat log  class and  queue  to per volume queue.
     */
  dmCatReq *dmUpdReq = new DataMgr::dmCatReq(volId, updCatReq->blob_name,
					     updCatReq->dm_transaction_id, updCatReq->dm_operation,srcIp,
					     dstIp,srcPort,dstPort, src_node_name, reqCookie, FDS_CAT_UPD,
					     updCatReq); 

  err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(dmUpdReq));
  if (err != ERR_OK) {
    FDS_PLOG(dataMgr->GetLog()) << "Unable to enqueue Update Catalog request "
				<< reqCookie;
    return err;
  }
  else 
    FDS_PLOG(dataMgr->GetLog()) << "Successfully enqueued   update Catalog  request "
				<< reqCookie;

  return err;
}


void DataMgr::ReqHandler::UpdateCatalogObject(FDS_ProtocolInterface::
                                              FDSP_MsgHdrTypePtr &msg_hdr,
                                              FDS_ProtocolInterface::
                                              FDSP_UpdateCatalogTypePtr
                                              &update_catalog) {
  Error err(ERR_OK);

#ifdef FDS_TEST_DM_NOOP
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
  msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
  dataMgr->swapMgrId(msg_hdr);
  dataMgr->respMapMtx.read_lock();
  dataMgr->respHandleCli(msg_hdr->src_node_name)->UpdateCatalogObjectResp(
									  *msg_hdr,
									  *update_catalog);
  dataMgr->respMapMtx.read_unlock();
  FDS_PLOG(dataMgr->GetLog()) << "FDS_TEST_DM_NOOP defined. Set update catalog response right after receiving req.";

  return;
#endif /* FDS_TEST_DM_NOOP */


  FDS_PLOG(dataMgr->GetLog()) << "Processing update catalog request with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob_name: "
                              << update_catalog->blob_name
    // << ", Obj ID: " << oid
                              << ", Trans ID: "
                              << update_catalog->dm_transaction_id
                              << ", OP ID " << update_catalog->dm_operation;

  err = dataMgr->updateCatalogInternal(update_catalog,msg_hdr->glob_volume_id,
				       msg_hdr->src_ip_lo_addr,msg_hdr->dst_ip_lo_addr,msg_hdr->src_port,
				       msg_hdr->dst_port,msg_hdr->src_node_name, msg_hdr->req_cookie);

  if (!err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Error Queueing the update Catalog request to Per volume Queue";
    		msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    		msg_hdr->err_msg  = "Something hit the fan...";
    		msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;

  		/*
   		 * Reverse the msg direction and send the response.
   		*/
  		msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
  		dataMgr->swapMgrId(msg_hdr);
                dataMgr->respMapMtx.read_lock();
  		dataMgr->respHandleCli(msg_hdr->src_node_name)->UpdateCatalogObjectResp(*msg_hdr,
											*update_catalog);
                dataMgr->respMapMtx.read_unlock();

  		FDS_PLOG(dataMgr->GetLog()) << "Sending async update catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << update_catalog->blob_name
                              << ", Trans ID "
                              << update_catalog->dm_transaction_id
                              << ", OP ID " << update_catalog->dm_operation;

  		if (update_catalog->dm_operation ==
      			FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN) {
    			FDS_PLOG(dataMgr->GetLog())
        			<< "Sent update response for trans open request";
  		} else if (update_catalog->dm_operation ==
             		FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
    			FDS_PLOG(dataMgr->GetLog())
        			<< "Sent update response for trans commit request";
  		}
	}
}



void
DataMgr::blobListBackend(dmCatReq *listBlobReq) {
  Error err(ERR_OK);

  std::list<BlobNode> bNodeList;
  err = _process_list(listBlobReq->volId, bNodeList);

  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType());
  FDSP_GetVolumeBlobListRespTypePtr
          blobListResp(new FDSP_GetVolumeBlobListRespType());
  DataMgr::InitMsgHdr(msg_hdr);
  if (err.ok()) {
    msg_hdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_msg = "Dude, you're good to go!";
  } else {
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
  }

  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_VOL_BLOB_LIST_RSP;
  msg_hdr->src_ip_lo_addr = listBlobReq->dstIp;
  msg_hdr->dst_ip_lo_addr = listBlobReq->srcIp;
  msg_hdr->src_port       = listBlobReq->dstPort;
  msg_hdr->dst_port       = listBlobReq->srcPort;
  msg_hdr->glob_volume_id = listBlobReq->volId;
  msg_hdr->req_cookie     = listBlobReq->reqCookie;

  blobListResp->num_blobs_in_resp = bNodeList.size();
  blobListResp->end_of_list       = true;  // TODO: For now, just returning entire list
  blobListResp->iterator_cookie   = 0;
  for (std::list<BlobNode>::iterator it = bNodeList.begin();
       it != bNodeList.end();
       it++) {
    FDSP_BlobInfoType bInfo;
    bInfo.blob_name = (*it).blob_name;
    bInfo.blob_size = (*it).blob_size;
    bInfo.mime_type = (*it).blob_mime_type;
    blobListResp->blob_info_list.push_back(bInfo);
  }
  respMapMtx.read_lock();
  respHandleCli(listBlobReq->src_node_name)->GetVolumeBlobListResp(*msg_hdr, *blobListResp);
  respMapMtx.read_unlock();
  FDS_PLOG_SEV(dm_log, fds::fds_log::normal) << "Sending async blob list response with "
                                             << "volume id: " << msg_hdr->glob_volume_id
                                             << " and " << blobListResp->num_blobs_in_resp
                                             << " blobs";

  qosCtrl->markIODone(*listBlobReq);
  delete listBlobReq;
}

void
DataMgr::queryCatalogBackend(dmCatReq  *qryCatReq) {
  Error err(ERR_OK);
   
  BlobNode *bnode = NULL;
  err = dataMgr->_process_query(qryCatReq->volId,
                                qryCatReq->blob_name,
                                bnode);

  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
  FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_catalog(new FDSP_QueryCatalogType);
  DataMgr::InitMsgHdr(msg_hdr);
  query_catalog->obj_list.clear();
  query_catalog->meta_list.clear();

  if (err.ok()) {
    bnode->ToFdspPayload(query_catalog);
    msg_hdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_msg = "Dude, you're good to go!";
  } else {
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
  }

  if (bnode) {
    delete bnode;
  }

  msg_hdr->src_ip_lo_addr =  qryCatReq->dstIp;
  msg_hdr->dst_ip_lo_addr =  qryCatReq->srcIp;
  msg_hdr->src_port =  qryCatReq->dstPort;
  msg_hdr->dst_port =  qryCatReq->srcPort;
  msg_hdr->glob_volume_id =  qryCatReq->volId;
  msg_hdr->req_cookie =  qryCatReq->reqCookie;
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_RSP;


  query_catalog->blob_name = qryCatReq->blob_name;
  query_catalog->dm_transaction_id = qryCatReq->transId;
  query_catalog->dm_operation = qryCatReq->transOp;

  dataMgr->respMapMtx.read_lock();
  dataMgr->respHandleCli(qryCatReq->src_node_name)->QueryCatalogObjectResp(*msg_hdr, *query_catalog);
  dataMgr->respMapMtx.read_unlock();
  FDS_PLOG(dataMgr->GetLog()) << "Sending async query catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << query_catalog->blob_name
                              << ", Trans ID "
                              << query_catalog->dm_transaction_id
                              << ", OP ID " << query_catalog->dm_operation;

  qosCtrl->markIODone(*qryCatReq);
  delete qryCatReq;

}

Error
DataMgr::blobListInternal(const FDSP_GetVolumeBlobListReqTypePtr& blob_list_req,
                          fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
                          fds_uint32_t dstPort, std::string src_node_name, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

  /*
   * allocate a new query cat log  class and  queue  to per volume queue.
   */
  dmCatReq *dmListReq = new DataMgr::dmCatReq(volId, srcIp, dstIp, srcPort, dstPort,
                                              src_node_name, reqCookie, FDS_LIST_BLOB);
  err = qosCtrl->enqueueIO(dmListReq->getVolId(), static_cast<FDS_IOType*>(dmListReq));
  if (err != ERR_OK) {
    FDS_PLOG(dataMgr->GetLog()) << "Unable to enqueue blob list request "
                                << reqCookie;
    delete dmListReq;
    return err;
  }

  return err;
}

Error
DataMgr::queryCatalogInternal(FDSP_QueryCatalogTypePtr qryCatReq, 
                                 fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
			      fds_uint32_t dstPort, std::string src_node_name, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

    
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
  dmCatReq *dmQryReq = new DataMgr::dmCatReq(volId, qryCatReq->blob_name,
					     qryCatReq->dm_transaction_id, qryCatReq->dm_operation,srcIp,
					     dstIp, srcPort,dstPort,src_node_name, reqCookie, FDS_CAT_QRY, NULL); 

    err = qosCtrl->enqueueIO(dmQryReq->getVolId(), static_cast<FDS_IOType*>(dmQryReq));
    if (err != ERR_OK) {
      FDS_PLOG(dataMgr->GetLog()) << "Unable to enqueue Query Catalog request "
                                     << reqCookie;
      return err;
    }
    else 
    	FDS_PLOG(dataMgr->GetLog()) << "Successfully enqueued  Catalog  request "
                                   << reqCookie;

   return err;
}



void DataMgr::ReqHandler::QueryCatalogObject(FDS_ProtocolInterface::
                                             FDSP_MsgHdrTypePtr &msg_hdr,
                                             FDS_ProtocolInterface::
                                             FDSP_QueryCatalogTypePtr
                                             &query_catalog) {

  Error err(ERR_OK);

  FDS_PLOG(dataMgr->GetLog()) << "Processing query catalog request with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << query_catalog->blob_name
			      << ", Trans ID "
                              << query_catalog->dm_transaction_id
                              << ", OP ID " << query_catalog->dm_operation;


  err = dataMgr->queryCatalogInternal(query_catalog,msg_hdr->glob_volume_id,
				      msg_hdr->src_ip_lo_addr,msg_hdr->dst_ip_lo_addr,msg_hdr->src_port,
				      msg_hdr->dst_port, msg_hdr->src_node_name, msg_hdr->req_cookie);
  	if (!err.ok()) {
    		msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    		msg_hdr->err_msg  = "Something hit the fan...";
    		msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;

  		/*
   		* Reverse the msg direction and send the response.
   		*/
  		msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_RSP;
  		dataMgr->swapMgrId(msg_hdr);
                dataMgr->respMapMtx.read_lock();
  		dataMgr->respHandleCli(msg_hdr->src_node_name)->QueryCatalogObjectResp(*msg_hdr, 
										       *query_catalog);
                dataMgr->respMapMtx.read_unlock();
  		FDS_PLOG(dataMgr->GetLog()) << "Sending async query catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob : "
                              << query_catalog->blob_name
                              << ", Trans ID "
                              << query_catalog->dm_transaction_id
                              << ", OP ID " << query_catalog->dm_operation;
	}
	else 
  	   FDS_PLOG(dataMgr->GetLog()) << "Successfully Enqueued  the query catalog request";
}


void
DataMgr::deleteCatObjBackend(dmCatReq  *delCatReq) {
  Error err(ERR_OK);
   
  BlobNode *bnode = NULL;
  err = dataMgr->_process_delete(delCatReq->volId,
                                delCatReq->blob_name);

  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
  FDS_ProtocolInterface::FDSP_DeleteCatalogTypePtr delete_catalog(new FDSP_DeleteCatalogType);
  DataMgr::InitMsgHdr(msg_hdr);

  if (err.ok()) {
    msg_hdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_msg = "Dude, you're good to go!";
  } else {
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
  }

  if (bnode) {
    delete bnode;
  }

  msg_hdr->src_ip_lo_addr =  delCatReq->dstIp;
  msg_hdr->dst_ip_lo_addr =  delCatReq->srcIp;
  msg_hdr->src_port =  delCatReq->dstPort;
  msg_hdr->dst_port =  delCatReq->srcPort;
  msg_hdr->glob_volume_id =  delCatReq->volId;
  msg_hdr->req_cookie =  delCatReq->reqCookie;
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_CAT_OBJ_RSP;


  delete_catalog->blob_name = delCatReq->blob_name;

  dataMgr->respMapMtx.read_lock();
  dataMgr->respHandleCli(delCatReq->src_node_name)->DeleteCatalogObjectResp(*msg_hdr, *delete_catalog);
  dataMgr->respMapMtx.read_unlock();
  FDS_PLOG(dataMgr->GetLog()) << "Sending async delete catalog obj response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << delete_catalog->blob_name;

  qosCtrl->markIODone(*delCatReq);
  delete delCatReq;

}



Error
DataMgr::deleteCatObjInternal(FDSP_DeleteCatalogTypePtr delCatReq, 
                              fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
			      fds_uint32_t dstPort, std::string src_node_name, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

    
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
  dmCatReq *dmDelReq = new DataMgr::dmCatReq(volId, delCatReq->blob_name, srcIp,0,0,
					     dstIp, srcPort,dstPort,src_node_name, reqCookie, FDS_DELETE_BLOB, NULL); 

    err = qosCtrl->enqueueIO(dmDelReq->getVolId(), static_cast<FDS_IOType*>(dmDelReq));
    if (err != ERR_OK) {
      FDS_PLOG(dataMgr->GetLog()) << "Unable to enqueue Deletye Catalog request "
                                     << reqCookie;
      return err;
    }
    else 
    	FDS_PLOG(dataMgr->GetLog()) << "Successfully enqueued  delete Catalog  request "
                                   << reqCookie;

   return err;

}

void DataMgr::ReqHandler::DeleteCatalogObject(FDS_ProtocolInterface::
                                              FDSP_MsgHdrTypePtr &msg_hdr,
                                              FDS_ProtocolInterface::
                                              FDSP_DeleteCatalogTypePtr
                                              &delete_catalog) {

  Error err(ERR_OK);

  FDS_PLOG(dataMgr->GetLog()) << "Processing Delete catalog request with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << delete_catalog->blob_name;
  err = dataMgr->deleteCatObjInternal(delete_catalog,msg_hdr->glob_volume_id,
				      msg_hdr->src_ip_lo_addr,msg_hdr->dst_ip_lo_addr,msg_hdr->src_port,
				      msg_hdr->dst_port, msg_hdr->src_node_name, msg_hdr->req_cookie);
  	if (!err.ok()) {
    		msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    		msg_hdr->err_msg  = "Error Enqueue delete Cat request";
    		msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;

  		/*
   		* Reverse the msg direction and send the response.
   		*/
  		msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_CAT_OBJ_RSP;
  		dataMgr->swapMgrId(msg_hdr);
                dataMgr->respMapMtx.read_lock();
  		dataMgr->respHandleCli(msg_hdr->src_node_name)->DeleteCatalogObjectResp(*msg_hdr, 
											*delete_catalog);
                dataMgr->respMapMtx.read_unlock();
  		FDS_PLOG(dataMgr->GetLog()) << "Sending async delete catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob : "
                              << delete_catalog->blob_name;
	}
	else 
  	   FDS_PLOG(dataMgr->GetLog()) << "Successfully Enqueued  the Delete catalog request";
}

int scheduleUpdateCatalog(void * _io) {
        fds::DataMgr::dmCatReq *io = (fds::DataMgr::dmCatReq*)_io;

    dataMgr->updateCatalogBackend(io);
    return 0;
}
int scheduleQueryCatalog(void * _io) {
        fds::DataMgr::dmCatReq *io = (fds::DataMgr::dmCatReq*)_io;

    dataMgr->queryCatalogBackend(io);
    return 0;
}

int scheduleDeleteCatObj(void * _io) {
        fds::DataMgr::dmCatReq *io = (fds::DataMgr::dmCatReq*)_io;

    dataMgr->deleteCatObjBackend(io);
    return 0;
}

int scheduleBlobList(void * _io) {
  fds::DataMgr::dmCatReq *io = (fds::DataMgr::dmCatReq*)_io;

  dataMgr->blobListBackend(io);
  return 0;
}

void DataMgr::InitMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
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

        msg_hdr->src_id = FDSP_DATA_MGR;
        msg_hdr->dst_id = FDSP_STOR_MGR;

	msg_hdr->src_node_name = "";

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


}


}  // namespace fds

int main(int argc, char *argv[]) {

    fds::dataMgr = new fds::DataMgr(argc, argv, "dm.conf", "fds.dm.");

    fds::Module *dmVec[] = {
        fds::dataMgr,
        nullptr
    };

    fds::dataMgr->setup(argc, argv, dmVec);
    fds::dataMgr->run();

    delete fds::dataMgr;

    return 0;
  
}
