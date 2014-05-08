/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <fds_timestamp.h>
#include <DataMgr.h>

namespace fds {
extern DataMgr *dataMgr;

Error DataMgr::vol_handler(fds_volid_t vol_uuid,
                           VolumeDesc *desc,
                           fds_vol_notify_t vol_action,
                           fds_bool_t check_only,
                           FDS_ProtocolInterface::FDSP_ResultType result) {
    Error err(ERR_OK);
    GLOGNORMAL << "Received vol notif from OM for "
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
                              fds_volid_t vol_uuid, VolumeDesc *desc) {
    Error err(ERR_OK);

    /*
     * Check if we already know about this volume
     */
    vol_map_mtx->lock();
    if (volExistsLocked(vol_uuid) == true) {
        LOGNORMAL << "Received add request for existing vol uuid "
                  << vol_uuid << ", so ignoring.";
        vol_map_mtx->unlock();
        return err;
    }

    vol_map_mtx->unlock();

    err = _add_vol_locked(vol_name, vol_uuid, desc);


    return err;
}

/*
 * Meant to be called holding the vol_map_mtx.
 */
Error DataMgr::_add_vol_locked(const std::string& vol_name,
                               fds_volid_t vol_uuid, VolumeDesc *vdesc) {
    Error err(ERR_OK);

    vol_map_mtx->lock();
    vol_meta_map[vol_uuid] = new VolumeMeta(vol_name,
                                            vol_uuid,
                                            GetLog(), vdesc);
    LOGNORMAL << "Added vol meta for vol uuid and per Volume queue"
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
        LOGNORMAL << "Received add request for existing vol uuid "
                  << std::hex << vol_uuid << std::dec;
        return err;
    }
    vol_map_mtx->unlock();

    err = _add_vol_locked(vol_name, vol_uuid, desc);
    return err;
}

Error DataMgr::_process_mod_vol(fds_volid_t vol_uuid, const VolumeDesc& voldesc)
{
    Error err(ERR_OK);

    vol_map_mtx->lock();
    /* make sure volume exists */
    if (volExistsLocked(vol_uuid) == false) {
        LOGERROR << "Received modify policy request for "
                 << "non-existant volume [" << vol_uuid
                 << ", " << voldesc.name << "]";
        err = Error(ERR_NOT_FOUND);
        vol_map_mtx->unlock();
        return err;
    }
    VolumeMeta *vm = vol_meta_map[vol_uuid];
    vm->vol_desc->modifyPolicyInfo(2*voldesc.iops_min, voldesc.iops_max, voldesc.relativePrio);
    err = qosCtrl->modifyVolumeQosParams(vol_uuid, 2*voldesc.iops_min, voldesc.iops_max, voldesc.relativePrio);
    vol_map_mtx->unlock();

    LOGNOTIFY << "Modify policy for volume "
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
        LOGNORMAL << "Received Delete request for:"
                  << std::hex << vol_uuid << std::dec
                  << " that doesn't exist.";
        err = ERR_INVALID_ARG;
        vol_map_mtx->unlock();
        return err;
    }
    vol_map_mtx->unlock();

    fds_bool_t isEmpty = _process_isEmpty(vol_uuid);
    if (isEmpty == false) {
        LOGERROR << "Volume is NOT Empty:"
                 << std::hex << vol_uuid << std::dec;
        err = ERR_VOL_NOT_EMPTY;
        return err;
    }
    // TODO(Andrew): Here we may want to prevent further I/Os
    // to the volume as we're going to remove it but a blob
    // may be written in the mean time.

    // if notify delete asked to only check if deleting volume
    // was ok; so we return with success here; DM will get
    // another notify volume delete with check_only ==false to
    // actually cleanup all other datastructures for this volume
    if (!check_only) {
        // TODO(Andrew): Here we want to delete each blob in the
        // volume and then mark the volume as deleted.
        vol_map_mtx->lock();
        VolumeMeta *vol_meta = vol_meta_map[vol_uuid];
        vol_meta_map.erase(vol_uuid);
        vol_map_mtx->unlock();
        dataMgr->qosCtrl->deregisterVolume(vol_uuid);
        delete vol_meta->dmVolQueue;
        delete vol_meta;
        LOGNORMAL << "Removed vol meta for vol uuid "
                  << vol_uuid;
    } else {
        LOGNORMAL << "Notify volume rm check only, did not "
                  << " remove vol meta for vol " << std::hex
                  << vol_uuid << std::dec;
    }

    vol_map_mtx->unlock();

    return err;
}

Error DataMgr::_process_open(fds_volid_t vol_uuid,
                             std::string blob_name,
                             fds_uint32_t trans_id,
                             const BlobNode* bnode) {
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
        LOGNORMAL << "Failed to add vol during open "
                  << "transaction for volume " << vol_uuid;

        return err;
    }

    err = vol_meta->OpenTransaction(blob_name, bnode, vol_meta->vol_desc);

    if (err.ok()) {
        LOGNORMAL << "Opened transaction for volume "
                  << vol_uuid << ",  blob "
                  << blob_name;
    } else {
        LOGNORMAL << "Failed to open transaction for volume "
                  << vol_uuid;
    }

    return err;
}

Error DataMgr::_process_commit(fds_volid_t vol_uuid,
                               std::string blob_name,
                               fds_uint32_t trans_id,
                               const BlobNode* bnode) {
    Error err(ERR_OK);

    /*
     * Here we should be updating the TVC to reflect the commit
     * and adding back to the VVC. The TVC can be an in-memory
     * update.
     * For now, we don't need to do anything because it was put
     * into the VVC on open.
     */
    LOGNORMAL << "Committed transaction for volume "
              << vol_uuid << " , blob "
              << blob_name << " with transaction id "
              << trans_id;

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

fds_bool_t
DataMgr::_process_isEmpty(fds_volid_t volId) {
    // Get a local reference to the vol meta.
    vol_map_mtx->lock();
    VolumeMeta *vol_meta = vol_meta_map[volId];
    vol_map_mtx->unlock();
    fds_verify(vol_meta != NULL);

    return vol_meta->isEmpty();
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
        LOGNORMAL << "Failed to add vol during list blob for volume " << volId;
        return err;
    }

    err = vol_meta->listBlobs(bNodeList);
    if (err.ok()) {
        LOGNORMAL << "Vol meta list blobs for volume "
                  << volId << " returned " << bNodeList.size()
                  << " blobs";
    } else {
        LOGNORMAL << "Vol meta list blobs FAILED for volume "
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
        LOGNORMAL << "Failed to add vol during query "
                  << "transaction for volume " << vol_uuid;
        return err;
    }


    err = vol_meta->QueryVcat(blob_name, bnode);

    if (err.ok()) {
        LOGNORMAL << "Vol meta query for volume "
                  << vol_uuid << " , blob "
                  << blob_name;
    } else {
        LOGNORMAL << "Vol meta query FAILED for volume "
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
        LOGNORMAL << "Vol meta Delete for volume "
                  << vol_uuid << " , blob "
                  << blob_name;
    } else {
        LOGNORMAL << "Vol meta delete FAILED for volume "
                  << vol_uuid;
    }

    return err;
}

DataMgr::DataMgr(int argc, char *argv[], Platform *platform, Module **vec)
        : PlatformProcess(argc, argv, "fds.dm.", "dm.log", platform, vec),
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

    vol_map_mtx = new fds_mutex("Volume map mutex");

    big_fat_lock = new fds_mutex("big fat mutex");

    _tp = new fds_threadpool(num_threads);

    /*
     * Comm with OM will be setup during run()
     */
    omClient = NULL;

    /*
     *  init Data Manager  QOS class.
     */
    qosCtrl = new dmQosCtrl(this, 20, FDS_QoSControl::FDS_DISPATCH_WFQ, GetLog());
    qosCtrl->runScheduler();

    LOGNORMAL << "Constructing the Data Manager";
}

DataMgr::~DataMgr()
{
    LOGNORMAL << "Destructing the Data Manager";

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
    delete big_fat_lock;
    delete vol_map_mtx;
    delete qosCtrl;
}

int DataMgr::run()
{
    try {
        nstable->listenServer(metadatapath_session);
    }
    catch(...){
        std::cout << "starting server threw an exception" << std::endl;
    }
    return 0;
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
                plf_mgr->plf_get_my_data_port(),
                node_name,
                FDSP_STOR_HVISOR,
                metadatapath_handler);
}

void DataMgr::proc_pre_startup()
{
    fds::DmDiskInfo     *info;
    fds::DmDiskQuery     in;
    fds::DmDiskQueryOut  out;
    fds_bool_t      useTestMode = false;

    runMode = NORMAL_MODE;

    PlatformProcess::proc_pre_startup();

    // Get config values from that platform lib.
    //
    omConfigPort = plf_mgr->plf_get_om_ctrl_port();
    omIpStr      = *plf_mgr->plf_get_om_ip();

    use_om = !(conf_helper_.get_abs<bool>("fds.dm.no_om", false));
    useTestMode = conf_helper_.get_abs<bool>("fds.dm.test_mode", false);
    int sev_level = conf_helper_.get_abs<int>("fds.dm.log_severity", 0);

    GetLog()->setSeverityFilter(( fds_log::severity_level)sev_level);
    if (useTestMode == true) {
        runMode = TEST_MODE;
    }
    LOGNORMAL << "Data Manager using control port " << plf_mgr->plf_get_my_ctrl_port();

    /* Set up FDSP RPC endpoints */
    nstable = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_DATA_MGR));
    myIp = util::get_local_ip();
    assert(myIp.empty() == false);
    std::string node_name = "_DM_" + myIp;

    LOGNORMAL << "Data Manager using IP:"
              << myIp << " and node name " << node_name;

    setup_metadatapath_server(myIp);

    if (use_om) {
        LOGNORMAL << " Initialising the OM client ";
        /*
         * Setup communication with OM.
         */
        omClient = new OMgrClient(FDSP_DATA_MGR,
                                  omIpStr,
                                  omConfigPort,
                                  node_name,
                                  GetLog(),
                                  nstable, plf_mgr);
        omClient->initialize();
        omClient->registerEventHandlerForNodeEvents(node_handler);
        omClient->registerEventHandlerForVolEvents(vol_handler);
        /*
         * Brings up the control path interface.
         * This does not require OM to be running and can
         * be used for testing DM by itself.
         */
        omClient->startAcceptingControlMessages();

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
    LOGNORMAL << " Received signal "
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
        GLOGNORMAL << "Error Queueing the blob list request to per volume Queue";
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
        try { 
            dataMgr->respHandleCli(msg_hdr->session_uuid)->GetVolumeBlobListResp(*msg_hdr,
                                                                             *blobListResp);
        } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
        }
        dataMgr->respMapMtx.read_unlock();

        GLOGERROR << "Sending async blob list error response with "
                  << "volume id: " << msg_hdr->glob_volume_id;
    }
}

/**
 * Checks the current DMT to determine if this DM primary
 * or not for a given volume.
 */
fds_bool_t
DataMgr::amIPrimary(fds_volid_t volUuid) {
    int numNodes = 1;
    fds_uint64_t nodeId;
    int result = omClient->getDMTNodesForVolume(volUuid,
                                                &nodeId,
                                                &numNodes);
    fds_verify(result == 0);
    fds_verify(numNodes == 1);

    NodeUuid primaryUuid(nodeId);
    const NodeUuid *mySvcUuid = plf_mgr->plf_get_my_svc_uuid();
    if (*mySvcUuid == primaryUuid) {
        return true;
    }
    return false;
}

// TODO(Andrew): This is a total hack to get a sane blob
// layout with a maximum object length. Ideally this
// should come from the volume's metadata, not hard coded
// based on what AM is using.
static const fds_uint64_t maxObjSize = 2 * 1024 * 1024;

/**
 * Applies a list of offset/objectId changes to an existing blob.
 * This checks that the change either modifies existing offsets
 * or appends to the blob. No sparse blobs allowed yet.
 * It also ensures that each object size, with the exception of
 * the last object, is the same.
 *
 * @param[in]  The volume the blob is in
 * @param[in]  List of modified offsets
 * @param[out] The blob node to modify
 * @return The result of the application
 */
Error
DataMgr::applyBlobUpdate(fds_volid_t volUuid,
                         const BlobObjectList &offsetList,
                         BlobNode *bnode) {
    Error err(ERR_OK);
    fds_verify(offsetList.size() != 0);

    LOGDEBUG << "Applying update to blob " << *bnode;

    // Iterate over each offset.
    // For now, we're requiring that the list
    // be sorted
    for (fds_uint32_t i = 0; i < offsetList.size(); i++) {

        fds_uint64_t offset = offsetList[i].offset;
        // TODO(Andrew): Expect only updates to aligned offsets
        // Need to handle unaligned updates in the future
        fds_verify((offset % maxObjSize) == 0);

        fds_uint64_t size = offsetList[i].size;

        LOGDEBUG << "Applying update to offset " << offset
                 << " with object id " << offsetList[i].data_obj_id
                 << " and size " << size;


        // fds_verify(size > 0);
        fds_verify(size <= maxObjSize);

        fds_uint32_t blobOffsetIndex = 0;

        // Check if we're modifying a new or old offset
        if (offset < bnode->blob_size) {
            // We're modifying an existing offset

            // Determine the offset's index into the BlobObjectList vector.
            blobOffsetIndex = offset / maxObjSize;

            LOGDEBUG << "Overwriting offset " << offset
                     << " with new size " << size
                     << " at blobList index " << blobOffsetIndex
                     << " and existing offset "
                     << bnode->obj_list[blobOffsetIndex].offset
                     << " and existing size "
                     << bnode->obj_list[blobOffsetIndex].size
                     << " that was sparse " << std::boolalpha
                     << bnode->obj_list[blobOffsetIndex].sparse;

            // Get old blob entry and update blob size
            BlobObjectInfo oldBlobObj = bnode->obj_list[blobOffsetIndex];
            // Update the blob size to reflect new object size
            if (size > oldBlobObj.size) {
                bnode->blob_size += (size - oldBlobObj.size);
            } else if (size < oldBlobObj.size) {
                // If we're shrinking the size of the entry
                // it should be because this entry is going to
                // be the new end of the blob. Otherwise, we'd
                // expect it to be at max object size
                fds_verify(offsetList[i].blob_end == true);
                bnode->blob_size -= (oldBlobObj.size - size);
            }

            // Expunge the old object id
            // TODO(Andrew): We shouldn't actually expunge until the
            // a specific version is garbage collected or if there is
            // no versioning for the volume. For now we expunge since
            // there's no GC and don't want to leak the object.
            if (oldBlobObj.sparse == false) {
                // We check if the entry is sparse because sparse
                // entries don't actually needed to be expunged
                // because there wasn't a real corresponding object id.
                if (runMode != TEST_MODE) {
                    // Only need to expunge if this DM is the primary
                    // for the volume
                    if (amIPrimary(volUuid) == true) {
                        err = expungeObject(bnode->vol_id, oldBlobObj.data_obj_id);
                        fds_verify(err == ERR_OK);
                    }
                }
            }

            // Overwrite the entry in place
            bnode->obj_list[blobOffsetIndex] = offsetList[i];

            // If this offset ends the blob, expunge whatever entries
            // were after it in the previous version
            if (offsetList[i].blob_end == true) {
                for (fds_uint32_t truncIndex = (bnode->obj_list.size() - 1);
                     truncIndex > blobOffsetIndex;
                     truncIndex--) {
                    // Pop the last blob entry from the bnode
                    BlobObjectInfo truncBlobObj = bnode->obj_list[truncIndex];
                    LOGDEBUG << "Truncating entry from blob " << bnode->blob_name
                             << " of size " << bnode->blob_size
                             << " with offset " << truncBlobObj.offset
                             << " and size " << truncBlobObj.size
                             << " and sparse is " << std::boolalpha
                             << truncBlobObj.sparse;
                    fds_verify(truncIndex == (bnode->obj_list.size() - 1));
                    bnode->blob_size -= truncBlobObj.size;

                    // Expunge the entry
                    if (truncBlobObj.sparse == false) {
                        // We check if the entry is sparse because sparse
                        // entries don't actually needed to be expunged
                        // because there wasn't a real corresponding object id.
                        if (runMode != TEST_MODE) {
                            // Only need to expunge if this DM is the primary
                            // for the volume
                            if (amIPrimary(volUuid) == true) {
                                err = expungeObject(bnode->vol_id, truncBlobObj.data_obj_id);
                                fds_verify(err == ERR_OK);
                            }
                        }
                    }

                    bnode->obj_list.popBack();
                }
            }
        } else {
            // We're extending the blob
            LOGDEBUG << "Extending blob " << bnode->blob_name
                     << " from " << bnode->blob_size << " to offset "
                     << offset;

            // Don't need to push if the object size is 0
            // TODO(Andrew): This check is only really needed
            // if we're going to write to offsets with length 0.
            // Remove it when we handle alignment.
            if (size > 0) {
                // Create 'sparse' blob info entries if the offset
                // does not directly append
                // If the blob size is not aligned, start adding
                // sparse entries at the next aligned offset
                fds_uint32_t round = (bnode->blob_size % maxObjSize);
                if (round != 0) {
                    round = (maxObjSize - round);
                }
                for (fds_uint64_t j = (bnode->blob_size + round);
                     j < offset;
                     j += maxObjSize) {
                    // Append a 'sparse' info entry to the end of the blob
                    bnode->obj_list.pushBack(BlobObjectInfo(j, maxObjSize));
                    // Increase the size as the sparse entry still counts
                    // towards the blob's size
                    bnode->blob_size += maxObjSize;
                }

                // Add the entry into its correct range
                // and update the size.
                bnode->obj_list.pushBack(offsetList[i]);
                bnode->blob_size += size;
            }
        }
    }
    // Bump the version since we modified the blob node
    // TODO(Andrew): We should actually be checking the
    // volume's versioning before we bump
    if (bnode->version == blob_version_deleted) {
        bnode->version = blob_version_initial;
    } else {
        bnode->version++;
    }

    LOGDEBUG << "Applied update to blob " << *bnode;

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

    // Grab a big lock around the entire operation so that
    // all updates to the same blob get serialized from disk
    // read, in-memory update, and disk write
    big_fat_lock->lock();

    // Check if this blob exists already and what its current version is.
    *bnode = NULL;
    err = _process_query(updCatReq->volId,
                         updCatReq->blob_name,
                         *bnode);
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // If this blob doesn't already exist, allocate a new one
        LOGDEBUG << "No blob found with name " << updCatReq->blob_name
                 << ", so allocating a new one";
        *bnode = new BlobNode(updCatReq->volId,
                              updCatReq->blob_name);
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

        // check for zero size objects and assign default objectid
        for ( uint i = 0; i < offsetList.size(); i++ ) {
            if ( 0 == offsetList[i].size ) {
                LOGWARN << "obj size is zero. setting id to nullobjectid"
                        << " ["<< i <<"] : " << offsetList[i].data_obj_id;
                offsetList[i].data_obj_id = NullObjectID;
            }
        }

        err = applyBlobUpdate(updCatReq->volId,
                              offsetList,
                              (*bnode));

        fds_verify(err == ERR_OK);
        LOGDEBUG << "Updated blob " << (*bnode)->blob_name
                 << " to size " << (*bnode)->blob_size;

        // Apply any associated blob metadata updates
        for (fds_uint32_t i = 0;
             i < updCatReq->fdspUpdCatReqPtr->meta_list.size();
             i++) {
            LOGNORMAL << "Received and applying metadata update pair key="
                      << updCatReq->fdspUpdCatReqPtr->meta_list[i].key
                      << " value="
                      << updCatReq->fdspUpdCatReqPtr->meta_list[i].value;

            (*bnode)->updateMetadata(updCatReq->fdspUpdCatReqPtr->meta_list[i].key,
                                     updCatReq->fdspUpdCatReqPtr->meta_list[i].value);
        }

        // Currently, this processes the entire blob at once.
        // Any modifications to it need to be made before hand.
        err = dataMgr->_process_open((fds_volid_t)updCatReq->volId,
                                     updCatReq->blob_name,
                                     updCatReq->transId,
                                     (*bnode));
    } else if (updCatReq->transOp ==
               FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
        err = dataMgr->_process_commit(updCatReq->volId,
                                       updCatReq->blob_name,
                                       updCatReq->transId,
                                       (*bnode));
    } else {
        fds_panic("Unknown catalog operation!");
    }

    big_fat_lock->unlock();
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

    // Add the blob's etag to the response
    // TODO(Andrew): We're just setting the etag is the resp
    // if it was given in the req because we don't know at
    // this level if a new etag was set or not and don't
    // want to return an old one
    for (fds_uint32_t i = 0;
         i < updCatReq->fdspUpdCatReqPtr->meta_list.size();
         i++) {
        if (updCatReq->fdspUpdCatReqPtr->meta_list[i].key == "etag") {
            FDS_ProtocolInterface::FDSP_MetaDataPair etagPair;
            etagPair.__set_key(updCatReq->fdspUpdCatReqPtr->meta_list[i].key);
            etagPair.__set_value(updCatReq->fdspUpdCatReqPtr->meta_list[i].value);

            update_catalog->meta_list.push_back(etagPair);
            LOGDEBUG << "Returning etag value " << etagPair.value
                     << " for blob " << updCatReq->blob_name;
        }
    }

    /*
     * Reverse the msg direction and send the response.
     */
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;

    dataMgr->respMapMtx.read_lock();
    try {
        dataMgr->respHandleCli(updCatReq->session_uuid)->UpdateCatalogObjectResp(*msg_hdr, *update_catalog);
    } catch (att::TTransportException& e) {
        GLOGERROR << "error during network call : " << e.what() ;
    }
    dataMgr->respMapMtx.read_unlock();

    LOGDEBUG << "Sending async update catalog response with "
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
        LOGNORMAL << "End:Sent update response for trans open request";
    } else if (update_catalog->dm_operation ==
               FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
        LOGNORMAL << "End:Sent update response for trans commit request";
    }

    if (bnode != NULL) {
        delete bnode;
    }

    qosCtrl->markIODone(*updCatReq);
    delete updCatReq;

}


Error
DataMgr::updateCatalogInternal(FDSP_UpdateCatalogTypePtr updCatReq,
                               fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                               fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    fds::Error err(fds::ERR_OK);

    /*
     * allocate a new update cat log  class and  queue  to per volume queue.
     */
    dmCatReq *dmUpdReq = new DataMgr::dmCatReq(volId, updCatReq->blob_name,
                                               updCatReq->dm_transaction_id, updCatReq->dm_operation, srcIp,
                                               dstIp, srcPort, dstPort, session_uuid, reqCookie, FDS_CAT_UPD,
                                               updCatReq);

    err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(dmUpdReq));
    if (err != ERR_OK) {
        LOGNORMAL << "Unable to enqueue Update Catalog request "
                  << reqCookie << " error " << err.GetErrstr();
        return err;
    }
    else
        LOGNORMAL << "Successfully enqueued   update Catalog  request "
                  << reqCookie;

    return err;
}

void
DataMgr::ReqHandler::StartBlobTx(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msgHdr,
                                 boost::shared_ptr<std::string> &volumeName,
                                 boost::shared_ptr<std::string> &blobName,
                                 FDS_ProtocolInterface::TxDescriptorPtr &txDesc) {
    GLOGDEBUG << "Received start blob transction request for volume "
              << *volumeName << " and blob " << *blobName;

    BlobTxId::const_ptr blobTxDesc = BlobTxId::ptr(new BlobTxId(
        txDesc->txId));
    
    dataMgr->startBlobTxInternal(*volumeName, *blobName, blobTxDesc,
                                 msgHdr->glob_volume_id,
                                 msgHdr->src_ip_lo_addr, msgHdr->dst_ip_lo_addr,  // IP stuff
                                 msgHdr->src_port, msgHdr->dst_port,  // Port stuff
                                 msgHdr->session_uuid, msgHdr->req_cookie);  // Req/state stuff
}

void
DataMgr::ReqHandler::StatBlob(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msgHdr,
                              boost::shared_ptr<std::string> &volumeName,
                              boost::shared_ptr<std::string> &blobName) {
    Error err(ERR_OK);

    GLOGDEBUG << "Received stat blob requested for volume "
              << *volumeName << " and blob " << *blobName;

    err = dataMgr->statBlobInternal(*volumeName, *blobName,
                                    msgHdr->glob_volume_id,
                                    msgHdr->src_ip_lo_addr, msgHdr->dst_ip_lo_addr,  // IP stuff
                                    msgHdr->src_port, msgHdr->dst_port,  // Port stuff
                                    msgHdr->session_uuid, msgHdr->req_cookie);  // Req/state stuff

    // Verify we were able to enqueue the request
    fds_verify(err == ERR_OK);
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
    try { 
        dataMgr->respHandleCli(msg_hdr->session_uuid)->UpdateCatalogObjectResp(
            *msg_hdr,
            *update_catalog);
    } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
     }

    dataMgr->respMapMtx.read_unlock();
    LOGNORMAL << "FDS_TEST_DM_NOOP defined. Set update catalog response right after receiving req.";

    return;
#endif /* FDS_TEST_DM_NOOP */


    GLOGNORMAL << "Processing update catalog request with "
               << "volume id: " << msg_hdr->glob_volume_id
               << ", blob_name: "
               << update_catalog->blob_name
            // << ", Obj ID: " << oid
               << ", Trans ID: "
               << update_catalog->dm_transaction_id
               << ", OP ID " << update_catalog->dm_operation;

    err = dataMgr->updateCatalogInternal(update_catalog, msg_hdr->glob_volume_id,
                                         msg_hdr->src_ip_lo_addr, msg_hdr->dst_ip_lo_addr, msg_hdr->src_port,
                                         msg_hdr->dst_port, msg_hdr->session_uuid, msg_hdr->req_cookie);

    if (!err.ok()) {
        GLOGNORMAL << "Error Queueing the update Catalog request to Per volume Queue";
        msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
        msg_hdr->err_msg  = "Something hit the fan...";
        msg_hdr->err_code = err.GetErrno();

        /*
         * Reverse the msg direction and send the response.
         */
        msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
        dataMgr->swapMgrId(msg_hdr);
        dataMgr->respMapMtx.read_lock();
        try { 
            dataMgr->respHandleCli(msg_hdr->session_uuid)->UpdateCatalogObjectResp(*msg_hdr,
                                                                               *update_catalog);
        } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
        }

        dataMgr->respMapMtx.read_unlock();

        GLOGNORMAL << "Sending async update catalog response with "
                   << "volume id: " << msg_hdr->glob_volume_id
                   << ", blob name: "
                   << update_catalog->blob_name
                   << ", Trans ID "
                   << update_catalog->dm_transaction_id
                   << ", OP ID " << update_catalog->dm_operation;

        if (update_catalog->dm_operation ==
            FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN) {
            GLOGNORMAL << "Sent update response for trans open request";
        } else if (update_catalog->dm_operation ==
                   FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_COMMITED) {
            GLOGNORMAL << "Sent update response for trans commit request";
        }
    }
}

void
DataMgr::startBlobTxBackend(const dmCatReq *startBlobTxReq) {
    LOGDEBUG << "Got blob tx backend for volume "
             << startBlobTxReq->getVolId() << " and blob "
             << startBlobTxReq->blob_name;

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr(new FDSP_MsgHdrType());
    InitMsgHdr(msgHdr);
    msgHdr->result         = FDS_ProtocolInterface::FDSP_ERR_OK;
    msgHdr->err_msg        = "Dude, you're good to go!";
    msgHdr->msg_code       = FDS_ProtocolInterface::FDSP_START_BLOB_TX;
    msgHdr->src_ip_lo_addr = startBlobTxReq->dstIp;
    msgHdr->dst_ip_lo_addr = startBlobTxReq->srcIp;
    msgHdr->src_port       = startBlobTxReq->dstPort;
    msgHdr->dst_port       = startBlobTxReq->srcPort;
    msgHdr->glob_volume_id = startBlobTxReq->volId;
    msgHdr->req_cookie     = startBlobTxReq->reqCookie;
    msgHdr->err_code       = ERR_OK;

    respMapMtx.read_lock();
    respHandleCli(startBlobTxReq->session_uuid)->StartBlobTxResp(msgHdr);
    respMapMtx.read_unlock();
    LOGDEBUG << "Sending stat blob response with "
             << "volume " << startBlobTxReq->volId
             << " and blob " << startBlobTxReq->blob_name;

    qosCtrl->markIODone(*startBlobTxReq);
    delete startBlobTxReq;
}

void
DataMgr::statBlobBackend(const dmCatReq *statBlobReq) {
    Error err(ERR_OK);
    BlobNode *bnode = NULL;

    err = queryCatalogProcess(statBlobReq, &bnode);
    if (err == ERR_OK) {
        fds_verify(bnode != NULL);
        fds_verify(bnode->version != blob_version_invalid);
    }

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr(new FDSP_MsgHdrType());
    FDS_ProtocolInterface::BlobDescriptorPtr blobDesc(
        new FDS_ProtocolInterface::BlobDescriptor());

    if (err == ERR_OK) {
        // Copy the metadata into the blob descriptor to return
        blobDesc->name = bnode->blob_name;
        blobDesc->byteCount = bnode->blob_size;
        for (MetaList::const_iterator it = bnode->meta_list.begin();
             it != bnode->meta_list.end();
             it++) {
            blobDesc->metadata[it->key] = it->value;
        }

        msgHdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
        msgHdr->err_msg = "Dude, you're good to go!";
    } else {
        fds_verify(err == ERR_BLOB_NOT_FOUND);

        msgHdr->result  = FDS_ProtocolInterface::FDSP_ERR_OK;
        msgHdr->err_msg = "Could not find the blob!";
    }

    if (bnode != NULL) {
        delete bnode;
    }

    InitMsgHdr(msgHdr);
    msgHdr->msg_code       = FDS_ProtocolInterface::FDSP_STAT_BLOB;
    msgHdr->src_ip_lo_addr = statBlobReq->dstIp;
    msgHdr->dst_ip_lo_addr = statBlobReq->srcIp;
    msgHdr->src_port       = statBlobReq->dstPort;
    msgHdr->dst_port       = statBlobReq->srcPort;
    msgHdr->glob_volume_id = statBlobReq->volId;
    msgHdr->req_cookie     = statBlobReq->reqCookie;
    msgHdr->err_code       = err.GetErrno();

    respMapMtx.read_lock();
    respHandleCli(statBlobReq->session_uuid)->StatBlobResp(msgHdr, blobDesc);
    respMapMtx.read_unlock();
    LOGNORMAL << "Sending stat blob response with "
              << "volume id: " << msgHdr->glob_volume_id
              << " and blob " << blobDesc->name;

    qosCtrl->markIODone(*statBlobReq);
    delete statBlobReq;
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
    try { 
        respHandleCli(listBlobReq->session_uuid)->GetVolumeBlobListResp(*msg_hdr, *blobListResp);
    } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
    }

    respMapMtx.read_unlock();
    LOGNORMAL << "Sending async blob list response with "
              << "volume id: " << msg_hdr->glob_volume_id
              << " and " << blobListResp->num_blobs_in_resp
              << " blobs";

    qosCtrl->markIODone(*listBlobReq);
    delete listBlobReq;
}

/**
 * Queries a particular blob version.
 *
 * @param[in] The DM query request struct
 * @param[out] The blob node queried.
 * The pointer is only valid if err is OK.
 * @return The result of the query.
 */
Error
DataMgr::queryCatalogProcess(const dmCatReq  *qryCatReq, BlobNode **bnode) {
    Error err(ERR_OK);

    // Lock to prevent reading a blob while it's being
    // modified
    big_fat_lock->lock();

    // TODO(Andrew): All we're doing here is retrieving
    // the latest version. This needs to be fixed when
    // we actually have versioning structures.
    err = _process_query(qryCatReq->volId,
                         qryCatReq->blob_name,
                         (*bnode));
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // We couldn't locate the blob
        LOGDEBUG << "No blob found with name " << qryCatReq->blob_name
                 << ", so nothing to query";
        err = ERR_BLOB_NOT_FOUND;
    } else {
        fds_verify((*bnode) != NULL);
        fds_verify((*bnode)->version != blob_version_invalid);

        if ((*bnode)->version == blob_version_deleted) {
            // The version we located was a delete marker, which
            // is effectively deleted
            LOGDEBUG << "Found blob name " << (*bnode)->blob_name
                     << " delete marker. Returning not found.";
            err = ERR_BLOB_NOT_FOUND;
            delete (*bnode);
            (*bnode) = NULL;
        } else if (qryCatReq->blob_version != blob_version_invalid) {
            // A specific version was requested, check if we found
            // that version
            if (qryCatReq->blob_version == (*bnode)->version) {
                // We found the version, so return it
                LOGDEBUG << "Located requested blob " << (*bnode)->blob_name
                         << " with specific version " << (*bnode)->version;
            } else {
                // The version we found doesn't match the requested
                // version
                LOGDEBUG << "Located requested blob " << (*bnode)->blob_name
                         << " with version " << (*bnode)->version
                         << " that did NOT match requested version "
                         << qryCatReq->blob_version;
                err = ERR_BLOB_NOT_FOUND;
                delete (*bnode);
                (*bnode) = NULL;
            }
        } else {
            // No version was reqested. Just return the most recent
            // version that we found.
            LOGDEBUG << "Located requested blob " << (*bnode)->blob_name
                     << " with version " << (*bnode)->version;
        }
    }

    big_fat_lock->unlock();

    return err;
}

void
DataMgr::queryCatalogBackend(dmCatReq  *qryCatReq) {
    Error err(ERR_OK);
    ObjectID obj_id;

    BlobNode *bnode = NULL;
    err = queryCatalogProcess(qryCatReq, &bnode);
    if (err == ERR_OK) {
        fds_verify(bnode != NULL);
        fds_verify(bnode->version != blob_version_invalid);
    }

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
        if (err == ERR_BLOB_NOT_FOUND) {
            msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_BLOB_NOT_FOUND;
        } else {
            msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_FAILED;
        }
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
    try { 
        dataMgr->respHandleCli(qryCatReq->session_uuid)->QueryCatalogObjectResp(*msg_hdr, *query_catalog);
    } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
    }

    dataMgr->respMapMtx.read_unlock();
    LOGNORMAL << "Sending async query catalog response with "
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
                          fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                          fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    fds::Error err(fds::ERR_OK);

    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    dmCatReq *dmListReq = new DataMgr::dmCatReq(volId, srcIp, dstIp, srcPort, dstPort,
                                                session_uuid, reqCookie, FDS_LIST_BLOB);
    err = qosCtrl->enqueueIO(dmListReq->getVolId(), static_cast<FDS_IOType*>(dmListReq));
    if (err != ERR_OK) {
        LOGNORMAL << "Unable to enqueue blob list request "
                  << reqCookie;
        delete dmListReq;
        return err;
    }

    return err;
}

void
DataMgr::commitBlobTxInternal(BlobTxId::const_ptr blobTxId,
                             fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                             fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    Error err(ERR_OK);

    // The volume and blob will be derived from the transaction id
    dmCatReq *dmCommitTxReq = new DataMgr::dmCatReq(0, "",
                                                    srcIp, dstIp,
                                                    srcPort, dstPort,
                                                    session_uuid, reqCookie,
                                                    FDS_COMMIT_BLOB_TX);
    fds_verify(dmCommitTxReq != NULL);

    // Set the desired version to invalid for now (so we get the newest)
    dmCommitTxReq->setBlobVersion(blob_version_invalid);
    dmCommitTxReq->setBlobTxId(blobTxId);
    err = qosCtrl->enqueueIO(dmCommitTxReq->getVolId(),
                             static_cast<FDS_IOType*>(dmCommitTxReq));
    // Make sure we could enqueue the request
    fds_verify(err == ERR_OK);
}

void
DataMgr::abortBlobTxInternal(BlobTxId::const_ptr blobTxId,
                             fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                             fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    Error err(ERR_OK);

    // The volume and blob will be derived from the transaction id
    dmCatReq *dmAbortTxReq = new DataMgr::dmCatReq(0, "",
                                                   srcIp, dstIp,
                                                   srcPort, dstPort,
                                                   session_uuid, reqCookie,
                                                   FDS_ABORT_BLOB_TX);
    fds_verify(dmAbortTxReq != NULL);

    // Set the desired version to invalid for now (so we get the newest)
    dmAbortTxReq->setBlobVersion(blob_version_invalid);
    dmAbortTxReq->setBlobTxId(blobTxId);
    err = qosCtrl->enqueueIO(dmAbortTxReq->getVolId(),
                             static_cast<FDS_IOType*>(dmAbortTxReq));
    // Make sure we could enqueue the request
    fds_verify(err == ERR_OK);
}

void
DataMgr::startBlobTxInternal(const std::string volumeName, const std::string &blobName,
                             BlobTxId::const_ptr blobTxId,
                             fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                             fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    Error err(ERR_OK);

    dmCatReq *dmStartTxReq = new DataMgr::dmCatReq(volId, blobName,
                                                   srcIp, dstIp,
                                                   srcPort, dstPort,
                                                   session_uuid, reqCookie,
                                                   FDS_START_BLOB_TX);
    fds_verify(dmStartTxReq != NULL);

    // Set the desired version to invalid for now (so we get the newest)
    dmStartTxReq->setBlobVersion(blob_version_invalid);
    dmStartTxReq->setBlobTxId(blobTxId);
    err = qosCtrl->enqueueIO(dmStartTxReq->getVolId(),
                             static_cast<FDS_IOType*>(dmStartTxReq));
    // Make sure we could enqueue the request
    fds_verify(err == ERR_OK);
}

Error
DataMgr::statBlobInternal(const std::string volumeName, const std::string &blobName,
                          fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                          fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    Error err(ERR_OK);

    dmCatReq *dmStatReq = new DataMgr::dmCatReq(volId, blobName,
                                                srcIp, dstIp,
                                                srcPort, dstPort,
                                                session_uuid, reqCookie,
                                                FDS_STAT_BLOB);
    fds_verify(dmStatReq != NULL);

    // Set the desired version to invalid for now (so we get the newest)
    dmStatReq->setBlobVersion(blob_version_invalid);
    err = qosCtrl->enqueueIO(dmStatReq->getVolId(), static_cast<FDS_IOType*>(dmStatReq));

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
                                               qryCatReq->dm_operation, srcIp,
                                               dstIp, srcPort, dstPort, session_uuid,
                                               reqCookie, FDS_CAT_QRY, NULL);
    // Set the version
    // TODO(Andrew): Have a better constructor so that I can
    // set it that way.
    dmQryReq->setBlobVersion(qryCatReq->blob_version);

    err = qosCtrl->enqueueIO(dmQryReq->getVolId(), static_cast<FDS_IOType*>(dmQryReq));
    if (err != ERR_OK) {
        LOGNORMAL << "Unable to enqueue Query Catalog request "
                  << reqCookie;
        return err;
    }
    else
        LOGNORMAL << "Successfully enqueued  Catalog  request "
                  << reqCookie;

    return err;
}



void DataMgr::ReqHandler::QueryCatalogObject(FDS_ProtocolInterface::
                                             FDSP_MsgHdrTypePtr &msg_hdr,
                                             FDS_ProtocolInterface::
                                             FDSP_QueryCatalogTypePtr
                                             &query_catalog) {
    Error err(ERR_OK);
    GLOGNORMAL << "Processing query catalog request with "
               << "volume id: " << msg_hdr->glob_volume_id
               << ", blob name: "
               << query_catalog->blob_name
               << ", Trans ID "
               << query_catalog->dm_transaction_id
               << ", OP ID " << query_catalog->dm_operation;


    err = dataMgr->queryCatalogInternal(query_catalog, msg_hdr->glob_volume_id,
                                        msg_hdr->src_ip_lo_addr, msg_hdr->dst_ip_lo_addr, msg_hdr->src_port,
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
        try {
            dataMgr->respHandleCli(msg_hdr->session_uuid)->QueryCatalogObjectResp(*msg_hdr,
                                                                              *query_catalog);
        } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
        }

        dataMgr->respMapMtx.read_unlock();
        GLOGNORMAL << "Sending async query catalog response with "
                   << "volume id: " << msg_hdr->glob_volume_id
                   << ", blob : "
                   << query_catalog->blob_name
                   << ", Trans ID "
                   << query_catalog->dm_transaction_id
                   << ", OP ID " << query_catalog->dm_operation;
    }
    else {
        GLOGNORMAL << "Successfully Enqueued  the query catalog request";
    }
}

/**
 * Populates an fdsp message header with stock fields.
 *
 * @param[in] Ptr to msg header to modify
 */
void
DataMgr::initSmMsgHdr(FDSP_MsgHdrTypePtr msgHdr) {
    msgHdr->minor_ver = 0;
    msgHdr->msg_id    = 1;

    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->num_objects = 1;
    msgHdr->frag_len    = 0;
    msgHdr->frag_num    = 0;

    msgHdr->tennant_id      = 0;
    msgHdr->local_domain_id = 0;

    msgHdr->src_id = FDSP_DATA_MGR;
    msgHdr->dst_id = FDSP_STOR_MGR;

    msgHdr->src_node_name = *(plf_mgr->plf_get_my_name());

    msgHdr->origin_timestamp = fds::get_fds_timestamp_ms();

    msgHdr->err_code = ERR_OK;
    msgHdr->result   = FDSP_ERR_OK;
}

/**
 * Issues delete calls for an object when it is dereferenced
 * by a blob. Objects should only be expunged whenever a
 * blob's reference to a object is permanently removed.
 *
 * @param[in] The volume in which the obj is being deleted
 * @param[in] The object to expunge
 * return The result of the expunge
 */
Error
DataMgr::expungeObject(fds_volid_t volId, const ObjectID &objId) {
    Error err(ERR_OK);

    // Locate the SMs holding this blob
    DltTokenGroupPtr tokenGroup =
            omClient->getDLTNodesForDoidKey(objId);

    FDSP_MsgHdrTypePtr msgHdr(new FDSP_MsgHdrType);
    initSmMsgHdr(msgHdr);
    msgHdr->msg_code       = FDSP_MSG_DELETE_OBJ_REQ;
    msgHdr->glob_volume_id = volId;
    msgHdr->req_cookie     = 1;
    FDSP_DeleteObjTypePtr delReq(new FDSP_DeleteObjType);
    delReq->data_obj_id.digest.assign((const char *)objId.GetId(),
                                      (size_t)objId.GetLen());
    delReq->dlt_version = omClient->getDltVersion();
    delReq->data_obj_len = 0;  // What is this...?

    // Issue async delete object calls to each SM
    uint errorCount = 0;
    for (fds_uint32_t i = 0; i < tokenGroup->getLength(); i++) {
        try {
            NodeUuid uuid = tokenGroup->get(i);
            NodeAgent::pointer node = plf_mgr->plf_node_inventory()->
                    dc_get_sm_nodes()->agent_info(uuid);
            SmAgent::pointer sm = SmAgent::agt_cast_ptr(node);
            NodeAgentDpClientPtr smClient = sm->get_sm_client();

            msgHdr->session_uuid = sm->get_sm_sess_id();
            smClient->DeleteObject(msgHdr, delReq);
        } catch(const att::TTransportException& e) {
            errorCount++;
            GLOGERROR << "[" << errorCount << "]error during network call : " << e.what();
        }
    }

    if (errorCount >= int(ceil(tokenGroup->getLength()*0.5))) {
        LOGCRITICAL << "too many network errors ["
                    << errorCount << "/" <<tokenGroup->getLength() <<"]";
        return ERR_NETWORK_TRANSPORT;
    }

    return err;
}

/**
 * Permanetly deletes a blob and the objects that
 * it refers to.
 * Expunge differs from delete in that it's a hard delete
 * that frees resources permanently and is not recoverable.
 *
 * @paramp[in] The blob node to expunge
 * @return The result of the expunge
 */
Error
DataMgr::expungeBlob(const BlobNode *bnode) {
    Error err(ERR_OK);

    // Grab some kind of lock on the blob?

    // Iterate the entries in the blob list
    for (BlobObjectList::const_iterator it = bnode->obj_list.cbegin();
         it != bnode->obj_list.cend();
         it++) {
        ObjectID objId = it->data_obj_id;

        if (use_om) {
            err = expungeObject(bnode->vol_id, objId);
            fds_verify(err == ERR_OK);
        } else {
            // No OM means no DLT, which means
            // no way to contact SMs. Just keep going.
            continue;
        }
    }

    // Wait for delete object responses

    return err;
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
              << "volid:" << delCatReq->volId
              << ", blob name:" << delCatReq->blob_name
              << ", txnid:" << delCatReq->transId
              << ", opid:" << delCatReq->transOp
              << ", journTXID:" << delCatReq->reqCookie;

    // Check if this blob exists already and what its current version is
    // TODO(Andrew): The query should eventually manage multiple volumes
    // rather than overwriting. The key can be blob name and version.
    *bnode = NULL;
    err = _process_query(delCatReq->volId,
                         delCatReq->blob_name,
                         *bnode);
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // If this blob doesn't already exist, allocate a new one
        LOGWARN << "No blob found with name " << delCatReq->blob_name
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

    bool fSizeZero = ((*bnode)->blob_size == 0);
    if (fSizeZero) {
        LOGDEBUG << "zero size blob:" << (*bnode)->blob_name;
    }

    if (delCatReq->blob_version == blob_version_invalid) {
        // Allocate a delete marker blob node. The
        // marker is just a place holder marking the
        // blob is deleted. It doesn't actually point
        // to any offsets or object ids.
        // Blobs with delete markers may have older versions
        // garbage collected after some time.
        BlobNode *deleteMarker = new BlobNode(delCatReq->volId,
                                              delCatReq->blob_name);
        fds_verify(deleteMarker != NULL);
        deleteMarker->version   = blob_version_deleted;

        // TODO(Andrew): For now, just write the marker bnode
        // to disk. We'll eventually need open/commit if we're
        // going to use a 2-phase commit, like puts().
        err = _process_open((fds_volid_t)delCatReq->volId,
                            delCatReq->blob_name,
                            delCatReq->transId,
                            deleteMarker);
        fds_verify(err == ERR_OK);

        // Only need to expunge if this DM is the primary
        // for the volume
        if (amIPrimary(delCatReq->volId) == true) {
            if (fSizeZero) {
                LOGWARN << "zero size blob:" << (*bnode)->blob_name
                        << " - not expunging from SM";
            } else {
                err = expungeBlob((*bnode));
                fds_verify(err == ERR_OK);
            }
        }

        // We can delete the bnode we received from our
        // query since we're going to create new blob
        // node to represent to delete marker and return that
        delete (*bnode);
        (*bnode) = deleteMarker;
    } else if (delCatReq->blob_version == (*bnode)->version) {
        // We're explicity deleting this version.
        // We don't use a delete marker and instead
        // just delete this particular version

        // Only need to expunge if this DM is the primary
        // for the volume
        if (amIPrimary(delCatReq->volId) == true) {
            // We dereference count the objects in the blob here
            // prior to freeing the structure.
            // TODO(Andrew): We should persistently mark the blob
            // as being deleted so that we know to clean it up
            // after a crash. We may also need to persist deref
            // state so we don't double dereference an object.
            if (fSizeZero) {
                LOGWARN << "zero size blob:" << (*bnode)->blob_name
                        << " - not expunging from SM";
            } else {
                err = expungeBlob((*bnode));
                fds_verify(err == ERR_OK);
            }
        }

        // Remove the blob node structure
        err = _process_delete(delCatReq->volId,
                              delCatReq->blob_name);
        fds_verify(err == ERR_OK);
    }

    return err;
}

void
DataMgr::deleteCatObjBackend(dmCatReq  *delCatReq) {
    Error err(ERR_OK);

    BlobNode *bnode = NULL;
    // Process the delete blob. The deleted or modified
    // bnode will be allocated and returned on success
    err = deleteBlobProcess(delCatReq, &bnode);

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
    try { 
        dataMgr->respHandleCli(delCatReq->session_uuid)->DeleteCatalogObjectResp(*msg_hdr, *delete_catalog);
    } catch (att::TTransportException& e) {
        GLOGERROR << "error during network call : " << e.what() ;
    }

    dataMgr->respMapMtx.read_unlock();
    LOGNORMAL << "Sending async delete catalog obj response with "
              << "volume id: " << msg_hdr->glob_volume_id
              << ", blob name: "
              << delete_catalog->blob_name;

    qosCtrl->markIODone(*delCatReq);
    delete delCatReq;
}

void DataMgr::getBlobMetaDataBackend(const dmCatReq *request) {
    Error err(ERR_OK);
    std::unique_ptr<dmCatReq> reqPtr(const_cast<dmCatReq*>(request));
    fpi::FDSP_MsgHdrTypePtr msgHeader(new FDSP_MsgHdrType);
    boost::shared_ptr<fpi::FDSP_MetaDataList> metaDataList(new fpi::FDSP_MetaDataList());
    InitMsgHdr(msgHeader);
    request->fillResponseHeader(msgHeader);
    if (!volExists(request->volId)) {
        err = ERR_VOL_NOT_FOUND;
        LOGWARN << "volume not found: " << request->volId;
    } else {
        BlobNode* bNode;
        synchronized(big_fat_lock) {
            // get the current metadata
            err = _process_query(request->volId, request->blob_name, bNode);
            if (err == ERR_OK) {
                fpi::FDSP_MetaDataPair metapair;
                for( auto& meta : bNode->meta_list) {
                    metapair.key = meta.key;
                    metapair.value = meta.value;                    
                    metaDataList->push_back(metapair);
                }
            } else {
                LOGWARN << " error getting blob data: "
                        << " vol: " << request->volId
                        << " blob: " << request->blob_name
                        << " err: " << err;
            }
        }
    }
    
    // send the response
    setResponseError(msgHeader,err);
    read_synchronized(dataMgr->respMapMtx) {
        try {
            respHandleCli(request->session_uuid)->GetBlobMetaDataResp(*msgHeader,request->blob_name,*metaDataList);
        } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
        }
    }
}

void DataMgr::setBlobMetaDataBackend(const dmCatReq *request) {
    Error err(ERR_OK);
    std::unique_ptr<dmCatReq> reqPtr(const_cast<dmCatReq *>(request));
    fpi::FDSP_MsgHdrTypePtr msgHeader(new FDSP_MsgHdrType);

    InitMsgHdr(msgHeader);
    request->fillResponseHeader(msgHeader);
    if (!volExists(request->volId)) {
        err = ERR_VOL_NOT_FOUND;
        GLOGWARN << "volume not found: " << request->volId;
    } else {
        BlobNode* bNode;
        synchronized(big_fat_lock) {
            // get the current metadata
            err = _process_query(request->volId, request->blob_name, bNode);
            if (err == ERR_OK) {
                // add the new metadata
                for( auto& meta : *(request->metadataList)) {
                    bNode->updateMetadata(meta.key,meta.value);
                }
                // write back the metadata
                _process_open(request->volId,
                              request->blob_name,
                              1,
                              bNode);
            } else {
                GLOGWARN << " error getting blob data: "
                        << " vol: " << request->volId
                        << " blob: " << request->blob_name
                        << " err: " << err;
            }
        }
    }
    
    // send the response
    setResponseError(msgHeader,err);
    read_synchronized(dataMgr->respMapMtx) {
        try {
            respHandleCli(request->session_uuid)->SetBlobMetaDataResp(*msgHeader, request->blob_name);
        } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
        }
    }
}

Error
DataMgr::deleteCatObjInternal(FDSP_DeleteCatalogTypePtr delCatReq,
                              fds_volid_t volId, long srcIp, long dstIp, fds_uint32_t srcPort,
                              fds_uint32_t dstPort, std::string session_uuid, fds_uint32_t reqCookie) {
    fds::Error err(fds::ERR_OK);

    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    dmCatReq *dmDelReq = new DataMgr::dmCatReq(volId, delCatReq->blob_name, srcIp, 0, 0,
                                               dstIp, srcPort, dstPort, session_uuid,
                                               reqCookie, FDS_DELETE_BLOB, NULL);
    dmDelReq->blob_version = delCatReq->blob_version;

    err = qosCtrl->enqueueIO(dmDelReq->getVolId(), static_cast<FDS_IOType*>(dmDelReq));
    if (err != ERR_OK) {
        LOGERROR << "Unable to enqueue Deletye Catalog request "
                 << reqCookie;
        return err;
    }
    else {
        LOGDEBUG << "Successfully enqueued  delete Catalog  request "
                 << reqCookie;
    }
    return err;
}

void DataMgr::ReqHandler::DeleteCatalogObject(FDS_ProtocolInterface::
                                              FDSP_MsgHdrTypePtr &msg_hdr,
                                              FDS_ProtocolInterface::
                                              FDSP_DeleteCatalogTypePtr
                                              &delete_catalog) {

    Error err(ERR_OK);

    GLOGNORMAL << "Processing Delete catalog request with "
               << "volume id: " << msg_hdr->glob_volume_id
               << ", blob name: "
               << delete_catalog->blob_name;
    err = dataMgr->deleteCatObjInternal(delete_catalog, msg_hdr->glob_volume_id,
                                        msg_hdr->src_ip_lo_addr, msg_hdr->dst_ip_lo_addr, msg_hdr->src_port,
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
        try { 
            dataMgr->respHandleCli(msg_hdr->session_uuid)->DeleteCatalogObjectResp(*msg_hdr,
                                                                               *delete_catalog);
        } catch (att::TTransportException& e) {
            GLOGERROR << "error during network call : " << e.what() ;
        }

        dataMgr->respMapMtx.read_unlock();
        GLOGNORMAL << "Sending async delete catalog response with "
                   << "volume id: " << msg_hdr->glob_volume_id
                   << ", blob : "
                   << delete_catalog->blob_name;
    } else {
        GLOGNORMAL << "Successfully Enqueued  the Delete catalog request";
    }
}

void DataMgr::ReqHandler::SetBlobMetaData(boost::shared_ptr<FDSP_MsgHdrType>& msgHeader,
                                          boost::shared_ptr<std::string>& volumeName,
                                          boost::shared_ptr<std::string>& blobName,
                                          boost::shared_ptr<FDSP_MetaDataList>& metaDataList) {
    Error err(ERR_OK);

    GLOGDEBUG << " volume:" << *volumeName 
              << " blob:" << *blobName;
    
    RequestHeader reqHeader(msgHeader);
    dmCatReq* request = new DataMgr::dmCatReq(reqHeader, FDS_SET_BLOB_METADATA);
    request->metadataList = metaDataList;
    request->blob_name = *blobName;
    err = dataMgr->qosCtrl->enqueueIO(request->volId,request);
                             
    fds_verify(err == ERR_OK);
}

void DataMgr::ReqHandler::GetBlobMetaData(boost::shared_ptr<FDSP_MsgHdrType>& msgHeader,
                                          boost::shared_ptr<std::string>& volumeName,
                                          boost::shared_ptr<std::string>& blobName) {
    Error err(ERR_OK);
    GLOGDEBUG << " volume:" << *volumeName
             << " blob:" << *blobName;

    RequestHeader reqHeader(msgHeader);
    dmCatReq* request = new DataMgr::dmCatReq(reqHeader, FDS_GET_BLOB_METADATA);
    request->blob_name = *blobName;
    err = dataMgr->qosCtrl->enqueueIO(request->volId,request);

    fds_verify(err == ERR_OK);
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

int scheduleStartBlobTx(void * _io) {
    fds::DataMgr::dmCatReq *io = (fds::DataMgr::dmCatReq*)_io;

    dataMgr->startBlobTxBackend(io);
    return 0;
}

int scheduleStatBlob(void * _io) {
    fds::DataMgr::dmCatReq *io = (fds::DataMgr::dmCatReq*)_io;

    dataMgr->statBlobBackend(io);
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

int scheduleGetBlobMetaData(void* io) {
    dataMgr->getBlobMetaDataBackend((fds::DataMgr::dmCatReq*)io);
    return 0;
}

int scheduleSetBlobMetaData(void* io) {
    dataMgr->setBlobMetaDataBackend((fds::DataMgr::dmCatReq*)io);
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

void DataMgr::setResponseError(fpi::FDSP_MsgHdrTypePtr& msg_hdr, const Error& err) {
    if (err.ok()) {
        msg_hdr->result  = fpi::FDSP_ERR_OK;
        msg_hdr->err_msg = "OK";
    } else {
        msg_hdr->result   = fpi::FDSP_ERR_FAILED;
        msg_hdr->err_msg  = "FDSP_ERR_FAILED";
        msg_hdr->err_code = err.GetErrno();
    }
} 


}  // namespace fds
