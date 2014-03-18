/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "DataMgr.h"

namespace fds {
extern DataMgr *dataMgr;

Error DataMgr::vol_handler(fds_volid_t vol_uuid,
                           VolumeDesc *desc,
                           fds_vol_notify_t vol_action,
                           fds_bool_t check_only,
                           FDS_ProtocolInterface::FDSP_ResultType result) {
    Error err(ERR_OK);
    FDS_PLOG(dataMgr->GetLog()) << "Received vol notif from OM for "
                                << desc->getName() << ":"
                                << std::hex << vol_uuid << std::dec;
    
    if (vol_action == fds_notify_vol_add) {
        /*
         * TODO: Actually take a volume string name, not
         * just the volume number.
         */
        err = dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                        std::to_string(vol_uuid),
                                        vol_uuid, desc);
    } else if (vol_action == fds_notify_vol_rm) {
        err = dataMgr->_process_rm_vol(vol_uuid, check_only);
    } else if (vol_action == fds_notify_vol_mod) {
        err = dataMgr->_process_mod_vol(vol_uuid, *desc);
    } else {
        assert(0);
    }
    return err;
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
                                fds_volid_t vol_uuid,
                                VolumeDesc *desc) {
    Error err(ERR_OK);

    /*
     * Verify that we don't already know about this volume
     */
    vol_map_mtx->lock();
    if (volExistsLocked(vol_uuid) == true) {
        err = Error(ERR_DUPLICATE);
        vol_map_mtx->unlock();
        FDS_PLOG(dataMgr->GetLog()) << "Received add request for existing vol uuid "
                                    << std::hex << vol_uuid << std::dec;
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

Error DataMgr::_process_rm_vol(fds_volid_t vol_uuid, fds_bool_t check_only) {
  Error err(ERR_OK);

  /*
   * Make sure we already know about this volume
   */
  vol_map_mtx->lock();
  if (volExistsLocked(vol_uuid) == false) {
    FDS_PLOG(dataMgr->GetLog()) << "Received Delete request for " << vol_uuid;
    err = ERR_INVALID_ARG;
    vol_map_mtx->unlock();
    return err;
  }

  VolumeMeta *vm = vol_meta_map[vol_uuid];

  if (vm->getVcat()->DbEmpty() == true) {
    FDS_PLOG(dataMgr->GetLog()) << "Volume is NOT Empty" << vol_uuid;
    err = ERR_VOL_NOT_EMPTY;
    vol_map_mtx->unlock();
    return err;
  }

  // if notify delete asked to only check if deleting volume
  // was ok; so we return with success here; DM will get 
  // another notify volume delete with check_only ==false to
  // actually cleanup all other datastructures for this volume
  if (!check_only) {
      vol_meta_map.erase(vol_uuid);
      dataMgr->qosCtrl->deregisterVolume(vol_uuid);
      delete vm->dmVolQueue;
      delete vm;
      FDS_PLOG(dataMgr->GetLog()) << "Removed vol meta for vol uuid "
                                  << vol_uuid;
  } else {
      FDS_PLOG(dataMgr->GetLog()) << "Notify volume rm check only, did not "
                                  << " remove vol meta for vol " << std::hex
                                  << vol_uuid << std::dec;
  }

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

DataMgr::DataMgr(int argc, char *argv[], Platform *platform, Module **vec)
    : PlatformProcess(argc, argv, "fds.dm.", "dm.log", platform, vec),
      port_num(0),
      cp_port_num(0),
      omConfigPort(0),
      use_om(true),
      numTestVols(10),
      runMode(NORMAL_MODE),
      scheduleRate(4000),
      num_threads(DM_TP_THREADS)
{
    // If we're in test mode, don't daemonize.
    // TODO(Andrew): We probably want another config field and
    // not to override test_mode
    fds_bool_t noDaemon = conf_helper_.get_abs<bool>("fds.dm.test_mode", false);
    if (noDaemon == false) {
        daemonize();
    }
    dm_log = g_fdslog;
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

DataMgr::~DataMgr()
{
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
    delete qosCtrl;
}

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
    std::string node_name = "_DM_" + ip;
    // TODO: Ideally createServerSession should take a shared pointer
    // for datapath_handler.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    // TODO: Figure out who cleans up datapath_session_
    metadatapath_session = nstable->\
                           createServerSession<netMetaDataPathServerSession>(
                               myIpInt,
                               port_num,
                               node_name,
                               FDSP_STOR_HVISOR,
                               metadatapath_handler);
}

void DataMgr::setup()
{
    fds::DmDiskInfo     *info;
    fds::DmDiskQuery     in;
    fds::DmDiskQueryOut  out;
    fds_bool_t      useTestMode = false;

    /*
     * Invoke FdsProcess setup so that it can setup the signal hander and
     * execute the module vector for us
     */

    runMode = NORMAL_MODE;

    PlatformProcess::setup();

    // Get config values from that platform lib.
    //
    cp_port_num  = plf_mgr->plf_get_my_ctrl_port();
    port_num     = plf_mgr->plf_get_my_data_port();
    omConfigPort = plf_mgr->plf_get_om_ctrl_port();
    omIpStr      = *plf_mgr->plf_get_om_ip();

    use_om = !(conf_helper_.get_abs<bool>("fds.dm.no_om", false));
    useTestMode = conf_helper_.get_abs<bool>("fds.dm.test_mode", false);
    int sev_level = conf_helper_.get_abs<int>("fds.dm.log_severity", 0);

    GetLog()->setSeverityFilter(( fds_log::severity_level)sev_level);
    if (useTestMode == true) {
        runMode = TEST_MODE;
    }
    fds_assert((port_num != 0) && (cp_port_num != 0));
    FDS_PLOG(dm_log) << "Data Manager using port " << port_num
                     << "Data Manager using control port " << cp_port_num;

    /* Set up FDSP RPC endpoints */
    nstable = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_DATA_MGR));
    myIp = util::get_local_ip();
    assert(myIp.empty() == false);
    std::string node_name = "_DM_" + myIp;

    FDS_PLOG(dm_log) << "Data Manager using IP:"
                     << myIp << " and node name " << node_name;

    setup_metadatapath_server(myIp);

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
                                dm_log,
                                nstable);
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
      omClient->registerNodeWithOM(plf_mgr);
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
          vol_handler(testVolId, testVdb, fds_notify_vol_add, false, FDS_ProtocolInterface::FDSP_ERR_OK); 
          
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
                                  msg_hdr->dst_port, msg_hdr->session_uuid, msg_hdr->req_cookie);

  if (!err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Error Queueing the blob list request to per volume Queue";
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = err.GetErrno();

    /*
     * Reverse the msg direction and send the response.
     */
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_VOL_BLOB_LIST_RSP;
    dataMgr->swapMgrId(msg_hdr);
    FDS_ProtocolInterface::FDSP_GetVolumeBlobListRespTypePtr
            blobListResp(new FDS_ProtocolInterface::FDSP_GetVolumeBlobListRespType);
    blobListResp->num_blobs_in_resp = 0;
    blobListResp->end_of_list = true;
    dataMgr->respMapMtx.read_lock();
    dataMgr->respHandleCli(msg_hdr->session_uuid)->GetVolumeBlobListResp(*msg_hdr,
									  *blobListResp);
    dataMgr->respMapMtx.read_unlock();

    FDS_PLOG_SEV(dataMgr->GetLog(), fds::fds_log::error) << "Sending async blob list error response with "
                                                         << "volume id: " << msg_hdr->glob_volume_id;
  }  
}

/**
 * Applies a list of offset/objectId changes to an existing blob.
 * This checks that the change either modifies existing offsets
 * or appends to the blob. No sparse blobs allowed yet.
 * It also ensures that each object size, with the exception of
 * the last object, is the same.
 *
 * @param[in]  List of modified offsets
 * @param[out] The blob node to modify
 * @return The result of the application
 */
Error
DataMgr::applyBlobUpdate(const BlobObjectList &offsetList, BlobNode *bnode) {
    Error err(ERR_OK);
    fds_verify(offsetList.size() != 0);

    // Iterate over each offset.
    // For now, we're requiring that the list
    // be sorted
    for (fds_uint32_t i = 0; i < offsetList.size(); i++) {
        // Check that the offset doesn't make the blob sparse.
        // The offset should either be less than the existing
        // blob size or extend only from the end.
        fds_uint64_t offset = offsetList[i].offset;
        fds_verify(bnode->blob_size >= offset);

        fds_uint64_t size = offsetList[i].size;
        fds_verify(size > 0);

        // Check if we're appending or not
        if (offset == bnode->blob_size) {
            // We're appending. In order to append, the
            // previous offset's object must be complete
            // (i.e., at the max object size)
            // otherwise it'd be a sparse blob.
            if (bnode->blob_size > 0) {
                // Check that tail object's size matches
                // the head object's size.
                // TODO(Andrew): We're assuming the first
                // object fixes the object size for the
                // blob. This should actually be based on
                // the volume defined max object size.
                fds_verify(bnode->obj_list[0].size ==
                           (bnode->obj_list.back()).size);

                // Check that the new object's size is
                // not larger than the existing object's size.
                fds_verify(size <= bnode->obj_list[0].size);
            }

            // Append to the end
            bnode->obj_list.pushBack(offsetList[i]);
            bnode->blob_size += size;
        } else {
            fds_uint32_t maxBlobSize = 0;
            fds_uint32_t blobOffsetIndex = 0;

            // We should already have at least one entry
            fds_verify(bnode->blob_size > 0);

            // Check the size of the new object update.
            // TODO(Andrew): We're assuming when the first
            // object is written, that fixes the max object
            // size for the blob. This should actually be
            // based on volume defined max obejct size.
            // TODO(Andrew): Verify that the size is equal
            // to the max blob size, unless we're overwriting
            // the last object, in which case, it must be at
            // least as large as that object.
            maxBlobSize = bnode->obj_list[0].size;
            fds_verify(size <= maxBlobSize);

            // Determine the offset's index into the BlobObjectList
            // vector.
            // TODO(Andrew): Handle unaligned offset updates. For
            // now, just assert of offset is aligned.
            fds_verify((offset % maxBlobSize) == 0);
            blobOffsetIndex = offset / maxBlobSize;

            // Update blob in place. Get old object id and
            // then overwrite it
            BlobObjectInfo oldBlobObj = bnode->obj_list[blobOffsetIndex];
            bnode->obj_list[blobOffsetIndex] = offsetList[i];
            if (blobOffsetIndex == (bnode->obj_list.size() - 1)) {
                // We're modifying the last index, so we may need to bump the size
                fds_verify(size >= oldBlobObj.size);
                if (size > oldBlobObj.size) {
                    bnode->blob_size += (size - oldBlobObj.size);
                }
            }

            // Delete ref to old object at offset.
            // TODO(Andrew): Intergrate with SM-agent data path
            // interface.
        }
    }
    // Bump the version since we modified the blob node
    // TODO(Andrew): We should actually be checking the
    // volume's versioning before we bump
    bnode->version++;

    return err;
}

/**
 * Processes a catalog update.
 * The blob node is allocated inside of this function and returned
 * to the caller via a parameter. The bnode is only expected to be
 * allocated if ERR_OK is returned.
 *
 * @param[in]  updCatReq catalog update request
 * @param[out] bnode the blob node that was processed
 * @return The result of the processing
 */
Error
DataMgr::updateCatalogProcess(const dmCatReq  *updCatReq, BlobNode **bnode) {
    Error err(ERR_OK);
    
    LOGNORMAL << "Processing update catalog request with "
              << "volume id: " << updCatReq->volId
              << ", blob name: "
              << updCatReq->blob_name
              << ", Trans ID "
              << updCatReq->transId
              << ", OP ID " << updCatReq->transOp
              << ", journ TXID " << updCatReq->reqCookie;

    // Check if this blob exists already and what its current version is.
    *bnode = NULL;
    err = _process_query(updCatReq->volId,
                         updCatReq->blob_name,
                         *bnode);
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // If this blob doesn't already exist, allocate a new one
        LOGDEBUG << "No blob found with name " << updCatReq->blob_name
                 << ", so allocating a new one";
        *bnode = new BlobNode();
        fds_verify(bnode != NULL);
        err = ERR_OK;
    } else {
        LOGDEBUG << "Located existing blob " << updCatReq->blob_name;
        fds_verify(*bnode != NULL);
        fds_verify((*bnode)->version != blob_version_invalid);
    }
    fds_verify(err == ERR_OK);

    /*
     * For now, just treat this as an open
     */
    if (updCatReq->transOp ==
        FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN) {
        // Apply the updates to the blob
        BlobObjectList offsetList(updCatReq->fdspUpdCatReqPtr->obj_list);
        err = applyBlobUpdate(offsetList, (*bnode));
        fds_verify(err == ERR_OK);

        // Currently, this processes the entire blob at once.
        // Any modifications to it need to be made before hand.
        err = dataMgr->_process_open((fds_volid_t)updCatReq->volId,
                                     updCatReq->blob_name,
                                     updCatReq->transId,
                                     (const BlobNode *&)(*bnode));
    } else if (updCatReq->transOp ==
               FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
        err = dataMgr->_process_commit(updCatReq->volId,
                                       updCatReq->blob_name,
                                       updCatReq->transId,
                                       (const BlobNode *&)*bnode);
    } else {
        fds_panic("Unknown catalog operation!");
    }

    return err;
}

void
DataMgr::updateCatalogBackend(dmCatReq  *updCatReq) {
  Error err(ERR_OK);

  BlobNode *bnode = NULL;
  err = updateCatalogProcess(updCatReq, &bnode);
  if (err == ERR_OK) {
      fds_verify(bnode != NULL);
  }

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
    msg_hdr->err_code = err.GetErrno();
  }

  msg_hdr->src_ip_lo_addr =  updCatReq->dstIp;
  msg_hdr->dst_ip_lo_addr =  updCatReq->srcIp;
  msg_hdr->src_port =  updCatReq->dstPort;
  msg_hdr->dst_port =  updCatReq->srcPort;
  msg_hdr->glob_volume_id =  updCatReq->volId;
  msg_hdr->req_cookie =  updCatReq->reqCookie;

  update_catalog->blob_name = updCatReq->blob_name;
  // Return the now current version of the blob
  if (bnode != NULL) {
      // The bnode may be NULL if didn't successfully modifiy
      // it and are returning error.
      update_catalog->blob_version = bnode->version;
  } else {
      update_catalog->blob_version = blob_version_invalid;
  }
  update_catalog->dm_transaction_id = updCatReq->transId;
  update_catalog->dm_operation = updCatReq->transOp;
  /*
   * Reverse the msg direction and send the response.
   */
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;

  dataMgr->respMapMtx.read_lock();
  dataMgr->respHandleCli(updCatReq->session_uuid)->UpdateCatalogObjectResp(*msg_hdr, *update_catalog);
  dataMgr->respMapMtx.read_unlock();

  FDS_PLOG(dataMgr->GetLog()) << "Sending async update catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << update_catalog->blob_name
                              << ", blob version: "
                              << update_catalog->blob_version
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
  
  if (bnode != NULL) {
      delete bnode;
  }

  qosCtrl->markIODone(*updCatReq);
  delete updCatReq;

}


Error
DataMgr::updateCatalogInternal(FDSP_UpdateCatalogTypePtr updCatReq, 
			       fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
			       fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

    
    /*
     * allocate a new update cat log  class and  queue  to per volume queue.
     */
  dmCatReq *dmUpdReq = new DataMgr::dmCatReq(volId, updCatReq->blob_name,
					     updCatReq->dm_transaction_id, updCatReq->dm_operation,srcIp,
					     dstIp,srcPort,dstPort, session_uuid, reqCookie, FDS_CAT_UPD,
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
  dataMgr->respHandleCli(msg_hdr->session_uuid)->UpdateCatalogObjectResp(
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
				       msg_hdr->dst_port,msg_hdr->session_uuid, msg_hdr->req_cookie);

  if (!err.ok()) {
    FDS_PLOG(dataMgr->GetLog()) << "Error Queueing the update Catalog request to Per volume Queue";
    		msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    		msg_hdr->err_msg  = "Something hit the fan...";
    		msg_hdr->err_code = err.GetErrno();

  		/*
   		 * Reverse the msg direction and send the response.
   		*/
  		msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
  		dataMgr->swapMgrId(msg_hdr);
                dataMgr->respMapMtx.read_lock();
  		dataMgr->respHandleCli(msg_hdr->session_uuid)->UpdateCatalogObjectResp(*msg_hdr,
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
    msg_hdr->err_code = err.GetErrno();
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
  respHandleCli(listBlobReq->session_uuid)->GetVolumeBlobListResp(*msg_hdr, *blobListResp);
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
  
  // Check if blob version we have matches the specific
  // version requested or if no version was specified
  if ((err.ok()) && ((qryCatReq->blob_version == blob_version_invalid) ||
                     (qryCatReq->blob_version == bnode->version))) {
      bnode->ToFdspPayload(query_catalog);
      msg_hdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
      msg_hdr->err_msg = "Dude, you're good to go!";
  } else {
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    msg_hdr->err_msg  = "Something hit the fan...";
    msg_hdr->err_code = err.GetErrno();
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
  dataMgr->respHandleCli(qryCatReq->session_uuid)->QueryCatalogObjectResp(*msg_hdr, *query_catalog);
  dataMgr->respMapMtx.read_unlock();
  FDS_PLOG(dataMgr->GetLog()) << "Sending async query catalog response with "
                              << "volume id: " << msg_hdr->glob_volume_id
                              << ", blob name: "
                              << query_catalog->blob_name
                              << ", version: "
                              << query_catalog->blob_version
                              << ", Trans ID "
                              << query_catalog->dm_transaction_id
                              << ", OP ID " << query_catalog->dm_operation;

  qosCtrl->markIODone(*qryCatReq);
  delete qryCatReq;

}

Error
DataMgr::blobListInternal(const FDSP_GetVolumeBlobListReqTypePtr& blob_list_req,
                          fds_volid_t volId,long srcIp,long dstIp,fds_uint32_t srcPort,
                          fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

  /*
   * allocate a new query cat log  class and  queue  to per volume queue.
   */
  dmCatReq *dmListReq = new DataMgr::dmCatReq(volId, srcIp, dstIp, srcPort, dstPort,
                                              session_uuid, reqCookie, FDS_LIST_BLOB);
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
                              fds_volid_t volId, long srcIp,
                              long dstIp, fds_uint32_t srcPort,
			      fds_uint32_t dstPort, std::string session_uuid,
                              fds_uint32_t reqCookie) {
    fds::Error err(fds::ERR_OK);
    
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    dmCatReq *dmQryReq = new DataMgr::dmCatReq(volId, qryCatReq->blob_name,
                                               qryCatReq->dm_transaction_id,
                                               qryCatReq->dm_operation,srcIp,
                                               dstIp, srcPort,dstPort,session_uuid,
                                               reqCookie, FDS_CAT_QRY, NULL);
    // Set the version
    // TODO(Andrew): Have a better constructor so that I can
    // set it that way.
    dmQryReq->setBlobVersion(qryCatReq->blob_version);

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
				      msg_hdr->dst_port, msg_hdr->session_uuid, msg_hdr->req_cookie);
  	if (!err.ok()) {
    		msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    		msg_hdr->err_msg  = "Something hit the fan...";
    		msg_hdr->err_code = err.GetErrno();

  		/*
   		* Reverse the msg direction and send the response.
   		*/
  		msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_RSP;
  		dataMgr->swapMgrId(msg_hdr);
                dataMgr->respMapMtx.read_lock();
  		dataMgr->respHandleCli(msg_hdr->session_uuid)->QueryCatalogObjectResp(*msg_hdr, 
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

/**
 * "Deletes" a blob in a volume. Deleting may be a lazy or soft
 * delete or a hard delete, depending on the volume's paramters
 * and the blob's version.
 *
 * @param[in]  The DM delete request struct
 * @param[out] The blob node deleted or which represents the delete.
 * The pointer is only valid if err is OK.
 * @return The result of the delete
 */
Error
DataMgr::deleteBlobProcess(const dmCatReq  *delCatReq, BlobNode **bnode) {
    Error err(ERR_OK);
    LOGNORMAL << "Processing delete request with "
              << "volume id: " << delCatReq->volId
              << ", blob name: " << delCatReq->blob_name
              << ", Trans ID " << delCatReq->transId
              << ", OP ID " << delCatReq->transOp
              << ", journ TXID " << delCatReq->reqCookie;

    // Check if this blob exists already and what its current version is
    // TODO(Andrew): The query should eventually manage multiple volumes
    // rather than overwriting. The key can be blob name and version.
    *bnode = NULL;
    err = _process_query(delCatReq->volId,
                         delCatReq->blob_name,
                         *bnode);
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // If this blob doesn't already exist, allocate a new one
        LOGDEBUG << "No blob found with name " << delCatReq->blob_name
                 << ", so nothing to delete";
        err = ERR_BLOB_NOT_FOUND;
        return err;
    } else {
        fds_verify(*bnode != NULL);
        LOGDEBUG << "Located existing blob " << (*bnode)->blob_name
                 << " with version " << (*bnode)->version;
        fds_verify((*bnode)->version != blob_version_invalid);

        // The current version is a delete marker, it's
        // already deleted so return not found.
        if ((*bnode)->version == blob_version_deleted) {
            err = ERR_BLOB_NOT_FOUND;
            return err;
        }
    }
    fds_verify(err == ERR_OK);

    // Allocate a delete marker blob node. The
    // marker is just a place holder marking the
    // blobs is deleted. It doesn't actually point
    // to any offsets or object ids.
    (*bnode) = new BlobNode();
    fds_verify((*bnode) != NULL);
    (*bnode)->version = blob_version_deleted;

    /*
     * TODO(Andrew): For now, just write the marker bnode
     * to disk. We'll eventually need open/commit if we're
     * going to use a 2-phase commit, like puts().
     */

    // Currently, this processes the entire blob at once.
    // Any modifications to it need to be made before hand.
    err = dataMgr->_process_open((fds_volid_t)delCatReq->volId,
                                 delCatReq->blob_name,
                                 delCatReq->transId,
                                 (const BlobNode *&)(*bnode));
    return err;
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
        msg_hdr->err_code = err.GetErrno();
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
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_BLOB_RSP;

    delete_catalog->blob_name = delCatReq->blob_name;

    dataMgr->respMapMtx.read_lock();
    dataMgr->respHandleCli(delCatReq->session_uuid)->DeleteCatalogObjectResp(*msg_hdr, *delete_catalog);
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
			      fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
  fds::Error err(fds::ERR_OK);

    
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
  dmCatReq *dmDelReq = new DataMgr::dmCatReq(volId, delCatReq->blob_name, srcIp,0,0,
					     dstIp, srcPort,dstPort,session_uuid, reqCookie, FDS_DELETE_BLOB, NULL); 

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
				      msg_hdr->dst_port, msg_hdr->session_uuid, msg_hdr->req_cookie);
  	if (!err.ok()) {
    		msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
    		msg_hdr->err_msg  = "Error Enqueue delete Cat request";
    		msg_hdr->err_code = err.GetErrno();

  		/*
   		* Reverse the msg direction and send the response.
   		*/
  		msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DELETE_BLOB_RSP;
  		dataMgr->swapMgrId(msg_hdr);
                dataMgr->respMapMtx.read_lock();
  		dataMgr->respHandleCli(msg_hdr->session_uuid)->DeleteCatalogObjectResp(*msg_hdr, 
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

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDSP_ERR_OK;
}


}  // namespace fds
