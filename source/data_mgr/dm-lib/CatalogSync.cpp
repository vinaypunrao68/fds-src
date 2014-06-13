/*                                                                                                                                          
 * Copyright 2014 Formation Data Systems, Inc.                                                                                              
 */

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_typedefs.h>
#include <lib/OMgrClient.h>
#include <CatalogSync.h>
#include "DataMgr.h"

namespace fds {
    extern DataMgr *dataMgr;

/****** CatalogSync implementation ******/

CatalogSync::CatalogSync(const NodeUuid& uuid,
                         OMgrClient* omclient,
                         DmIoReqHandler* dm_req_hdlr)
        : node_uuid(uuid),
          state(CSSTATE_READY),
          om_client(omclient),
          meta_client(NULL),
          dm_req_handler(dm_req_hdlr) {
    state = ATOMIC_VAR_INIT(CSSTATE_READY);
    vols_done = ATOMIC_VAR_INIT(0);

    // create snap directory.
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    const std::string snap_dir = root->dir_user_repo_snap();

    std::system((const char *)("mkdir "+snap_dir+" ").c_str());
}

CatalogSync::~CatalogSync() {
}

/**
 * On return, set 'volumes' will be empty
 */
Error CatalogSync::startSync(std::set<fds_volid_t>* volumes,
                             netMetaSyncClientSession* client,
                             catsync_done_handler_t done_evt_hdlr) {
    Error err(ERR_OK);
    csStateType expect = CSSTATE_READY;
    if (!std::atomic_compare_exchange_strong(&state, &expect, CSSTATE_INITIAL_SYNC)) {
        LOGNOTIFY << "startSync called in not ready state, must not happen!";
        fds_verify(false);
        return ERR_NOT_READY;
    }

    LOGNORMAL << "Start sync to destination node "
              << std::hex << node_uuid.uuid_get_val() << std::dec;

    // set callback to notify when sync job is done
    meta_client = client;
    done_evt_handler = done_evt_hdlr;
    sync_volumes.swap(*volumes);
    fds_uint32_t counter_zero = 0;
    std::atomic_store(&vols_done, counter_zero);

    // queue qos requests to do db snapshot and do rsync per volume!
    for (std::set<fds_volid_t>::const_iterator cit = sync_volumes.cbegin();
         cit != sync_volumes.cend();
         ++cit) {
        DmIoSnapVolCat* snap_req = new DmIoSnapVolCat();
        snap_req->io_type = FDS_DM_SNAP_VOLCAT;
        snap_req->node_uuid = node_uuid;
        snap_req->volId = *cit;
        snap_req->dmio_snap_vcat_cb = std::bind(
            &CatalogSync::snapDoneCb, this,
            std::placeholders::_1, std::placeholders::_2);

        // enqueue to qos queue
        // TODO(anna) for now enqueueing to vol queue, need to
        // add system queue
        err = dm_req_handler->enqueueMsg(FdsDmSysTaskId, snap_req);
        // TODO(xxx) handle error -- probably queue is full and we
        // should retry later?
        fds_verify(err.ok());
    }

    return err;
}

void CatalogSync::doDeltaSync() {
    Error err(ERR_OK);
    csStateType cur_state = std::atomic_load(&state);
    if (cur_state != CSSTATE_DELTA_SYNC) {
        fds_panic("doDeltaSync is called in wrong state! node_uuid %llx",
                  node_uuid.uuid_get_val());
    }

    LOGNORMAL << "Start delta sync to destination node "
              << std::hex << node_uuid.uuid_get_val() << std::dec;

    // at this point, we must have meta client and done evt handler
    fds_verify(meta_client != NULL);
    fds_verify(done_evt_handler != NULL);

    // reset vols_done so we can also use it
    // for tracking delta sync progress
    fds_uint32_t counter_zero = 0;
    std::atomic_store(&vols_done, counter_zero);

    // queue qos requests to do db snapshot and do second rsync per volume!
    for (std::set<fds_volid_t>::const_iterator cit = sync_volumes.cbegin();
         cit != sync_volumes.cend();
         ++cit) {
        DmIoSnapVolCat* snap_req = new DmIoSnapVolCat();
        snap_req->io_type = FDS_DM_SNAPDELTA_VOLCAT;
        snap_req->node_uuid = node_uuid;
        snap_req->volId = *cit;
        snap_req->dmio_snap_vcat_cb = std::bind(
            &CatalogSync::deltaDoneCb, this,
            std::placeholders::_1, std::placeholders::_2);

        // enqueue to qos queue
        // TODO(anna) for now enqueueing to vol queue, need to
        // add system queue
        err = dm_req_handler->enqueueMsg(FdsDmSysTaskId, snap_req);
        // TODO(xxx) handle error -- probably queue is full and we
        // should start timer to retry
        fds_verify(err.ok());
    }
}


void CatalogSync::snapDoneCb(fds_volid_t volid,
                             const Error& error) {
    // record progress
    fds_uint32_t total_done = recordVolSyncDone(CSSTATE_INITIAL_SYNC);

    if (!error.ok()) {
        LOGERROR << "Rsync finished with error!";
        // TODO(xxx) should we remember the error and continue with
        // other rsync???
        // for now ignoring...
    }

    LOGDEBUG << "Finished rsync for volume " << std::hex << volid
             << " to node " << node_uuid.uuid_get_val() << std::dec
             << ", " << error << "; volumes finished so far "
             << total_done << " out of " << sync_volumes.size();

    if (total_done == sync_volumes.size()) {
        // we are done with the initial sync for this node!
        std::atomic_exchange(&state, CSSTATE_DELTA_SYNC);

        // reset vols_done so we can re-use it
        // for tracking delta sync progress
        fds_uint32_t counter_zero = 0;
        std::atomic_store(&vols_done, counter_zero);

        // notify catsync mgr that we are done
        fds_verify(done_evt_handler);
        done_evt_handler(CATSYNC_INITIAL_SYNC_DONE, volid, om_client, error);
    }
}

void CatalogSync::deltaDoneCb(fds_volid_t volid,
                              const Error& error) {
    // record progress
    fds_uint32_t total_done = recordVolSyncDone(CSSTATE_DELTA_SYNC);

    if (!error.ok()) {
        LOGERROR << "Second Rsync finished with error!";
        // TODO(xxx) should we remember the error and continue with
        // other rsync???
        // for now ignoring...
    }

    LOGDEBUG << "Finished second rsync for volume " << std::hex << volid
             << " to node " << node_uuid.uuid_get_val() << std::dec
             << ", " << error << "; volumes finished so far "
             << total_done << " out of " << sync_volumes.size();

    // Send msg to DM that we finished syncing this volume
    // this means that we are not going to do any rsync for this
    // volume anymore, only forward updates
    Error err = sendMetaSyncDone(volid, false);
    if (!err.ok()) {
        // TODO(xxx) how should we handle this? for now ignoring
    }

    if (total_done == sync_volumes.size()) {
        // we are done with delta rsync for this node!
        std::atomic_exchange(&state, CSSTATE_FORWARD_ONLY);
        // notify catsync mgr that we are done
        fds_verify(done_evt_handler);
        done_evt_handler(CATSYNC_DELTA_SYNC_DONE, volid, om_client, error);
    }
}

/**
 * Record progress of one volume finishing either initial sync or
 * delta sync -- common functionality used by snapDoneCb() and deltaDoneCb()
 */
fds_uint32_t CatalogSync::recordVolSyncDone(csStateType expected_state) {
    fds_uint32_t done_before, total_done;
    fds_uint32_t vols_done_now = 1;  // doing 1 vol at a time

    // we must be in 'expected_state' sync state
    csStateType cur_state = std::atomic_load(&state);
    fds_verify(cur_state == expected_state);

    // account for progress and see if we finished rsyncing all
    // volumes to this node
    done_before = std::atomic_fetch_add(&vols_done, vols_done_now);
    total_done = done_before + vols_done_now;
    fds_verify(total_done <= sync_volumes.size());

    return total_done;
}

Error CatalogSync::handleVolumeDone(fds_volid_t volid) {
    fds_verify(sync_volumes.count(volid) > 0);
    Error err = sendMetaSyncDone(volid, true);
    if (err.ok()) {
        sync_volumes.erase(volid);
    }
    return err;
}

fds_bool_t CatalogSync::isInitialSyncDone() const {
    csStateType cur_state = std::atomic_load(&state);
    return ((cur_state != CSSTATE_READY) &&
            (cur_state != CSSTATE_INITIAL_SYNC));
}

fds_bool_t CatalogSync::isDeltaSyncDone() const {
    csStateType cur_state = std::atomic_load(&state);
    return ((cur_state != CSSTATE_READY) &&
            (cur_state != CSSTATE_INITIAL_SYNC) &&
            (cur_state != CSSTATE_DELTA_SYNC));
}

Error CatalogSync::forwardCatalogUpdate(dmCatReq  *updCatReq) {
    Error err(ERR_OK);
    csStateType cur_state = std::atomic_load(&state);
    fds_verify(hasVolume(updCatReq->volId));
    fds_verify((cur_state == CSSTATE_DELTA_SYNC) ||
               (cur_state == CSSTATE_FORWARD_ONLY));

    LOGNORMAL << "DMT VERSION:  " << updCatReq->fdspUpdCatReqPtr->dmt_version
                           << ":" << dataMgr->omClient->getDMTVersion(); 
    if((uint)updCatReq->fdspUpdCatReqPtr->dmt_version ==
                     dataMgr->omClient->getDMTVersion()) {
        LOGDEBUG << " DMT version matches , Do not  forward  the  request ";
        return ERR_DMT_EQUAL; 
    }

    LOGDEBUG << "Will forward catalog update for volume "
             << std::hex << updCatReq->volId << std::dec;

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr
            updCatalog(new FDSP_UpdateCatalogType);

    DataMgr::InitMsgHdr(msg_hdr);  // init the  message  header
    msg_hdr->dst_id = FDSP_DATA_MGR;
    msg_hdr->glob_volume_id = updCatReq->volId;
    msg_hdr->session_uuid = meta_client->getSessionId();
    msg_hdr->session_cache = updCatReq->session_uuid;
       
    /*
     * init the update  catalog  structu
     */
    updCatalog->blob_name = updCatReq->blob_name; 
    updCatalog->blob_version = updCatReq->blob_version;
    updCatalog->blob_size = updCatReq->fdspUpdCatReqPtr->blob_size;
    updCatalog->blob_mime_type = updCatReq->fdspUpdCatReqPtr->blob_mime_type;
    updCatalog->obj_list = updCatReq->fdspUpdCatReqPtr->obj_list;
    updCatalog->meta_list = updCatReq->fdspUpdCatReqPtr->meta_list;
    updCatalog->dm_operation = FDS_DMGR_TXN_STATUS_OPEN;

    /*
     * send the update catalog to new node
     */
    try {
        meta_client->getClient()->PushMetaSyncReq(msg_hdr, updCatalog);
        LOGNORMAL << "Sent PushMetaSyncReq Rpc message : " << meta_client;
    } catch (...) {
        LOGERROR << "Unable to send PushMetaSyncReq to DM";
        err = ERR_NETWORK_TRANSPORT;
    }
    return err;
}

Error CatalogSync::sendMetaSyncDone(fds_volid_t volid,
                                    fds_bool_t forward_done) {
    Error err(ERR_OK);
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_VolMetaStatePtr vol_meta(new FDSP_VolMetaState);

    DataMgr::InitMsgHdr(msg_hdr);  // init the  message  header
    msg_hdr->dst_id = FDSP_DATA_MGR;

    vol_meta->vol_uuid = volid;
    vol_meta->forward_done = forward_done;

    LOGDEBUG << "Will send MetaSyncDone msg for vol " << std::hex
             << volid << std::dec << " fwd done? " << forward_done;

    try {
        meta_client->getClient()->MetaSyncDone(msg_hdr, vol_meta);
        LOGNORMAL << "Senr MetaSyncDone Rpc message : " << meta_client;
    } catch (...) {
        LOGERROR << "Unable to send MetaSyncDone to DM";
        err = ERR_NETWORK_TRANSPORT;
    }

    return err;
}

/***** CatalogSyncManager implementation ******/

CatalogSyncMgr::CatalogSyncMgr(fds_uint32_t max_jobs,
                               DmIoReqHandler* dm_req_hdlr,
                               netSessionTblPtr netSession)
        : Module("CatalogSyncMgr"),
          sync_in_progress(false),
          max_sync_inprogress(max_jobs),
          dm_req_handler(dm_req_hdlr),
          netSessionTbl(netSession),          
          cat_sync_lock("Catalog Sync lock") {
    LOGNORMAL << "Constructing CatalogSyncMgr";
}

CatalogSyncMgr::~CatalogSyncMgr() {
}

/**
 * Module start up code
 */
void CatalogSyncMgr::mod_startup()
{
    meta_handler.reset(new FDSP_MetaSyncRpc(*this, GetLog()));

    std::string ip = netSession::getLocalIp();
    int port = PlatformProcess::plf_manager()->plf_get_my_metasync_port();
    int myIpInt = netSession::ipString2Addr(ip);
    std::string node_name = "localhost-meta";
    meta_session = netSessionTbl->createServerSession<netMetaSyncServerSession>(
        myIpInt,
        port,
        node_name,
        FDSP_METASYNC_MGR,
        meta_handler);

    LOGNORMAL << "Meta sync path server setup ip: "
              << ip << " port: " << port;

    meta_session->listenServerNb(); 
}


 // create client  session
netMetaSyncClientSession*
CatalogSyncMgr::create_metaSync_client(const NodeUuid& node_uuid, OMgrClient* omclient)
{
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;
    fds_uint32_t sync_port = 0;

    omclient->getNodeInfo(node_uuid.uuid_get_val(), &node_ip, &node_port, &node_state);
    std::string dest_ip = netSessionTbl::ipAddr2String(node_ip);
    sync_port = omclient->getNodeMetaSyncPort(node_uuid);
    LOGDEBUG << "Dest node " << std::hex << node_uuid.uuid_get_val() << std::dec
             << "  = dest IP:" << dest_ip  << " port : " << sync_port;

    /* Create a client rpc session from src to dst */
    netMetaSyncClientSession* meta_client_session =
            netSessionTbl->startSession<netMetaSyncClientSession>(
                    dest_ip,
                    sync_port,
                    FDSP_METASYNC_MGR,
                    1, /* number of channels */
                    meta_handler);

    LOGNORMAL << "Meta sync path Client setup ip: "
              << dest_ip << " port: " << sync_port <<
              "client_session:" << meta_client_session;
    return meta_client_session;
}


/**
 * Module shutdown code
 */
void CatalogSyncMgr::mod_shutdown()
{
    LOGNORMAL << "Ending metadata path sessions";
    netSessionTbl->endSession(meta_session->getSessionTblKey());
}

/**
 * Called when DM receives push meta message from OM to start catalog sync process for list
 * of vols and to which nodes to sync.
 */
Error
CatalogSyncMgr::startCatalogSync(const FDS_ProtocolInterface::FDSP_metaDataList& metaVolList,
                                 OMgrClient* omclient,
                                 const std::string& context) {
    Error err(ERR_OK);
    
    fds_mutex::scoped_lock l(cat_sync_lock);
    // we should not get request to start a sync until
    // close is finished -- OM serializes DM deployment, so return an error
    LOGNORMAL << "startCatalogSync :" << sync_in_progress;
    if (sync_in_progress) {
        return ERR_NOT_READY;
    }
    fds_verify(cat_sync_map.size() == 0);

    // remember context to return with callback
    sync_in_progress = true;
    cat_sync_context = context;

    for (auto metavol : metaVolList) {
        NodeUuid uuid(metavol.node_uuid.uuid);

        // create CatalogSync object to handle syncing vols to node 'uuid'
        fds_verify(cat_sync_map.count(uuid) == 0);
        CatalogSyncPtr catsync(new CatalogSync(uuid, omclient, dm_req_handler));
        cat_sync_map[uuid] = catsync;

        // Get a set of volume ids that this node will need to push to node 'uuid'
        std::set<fds_volid_t> vols;

        for (auto vol : metavol.volList) {
            LOGDEBUG << "Will sync vol " << std::hex << vol
                     << " to node " << uuid.uuid_get_val() << std::dec;
            vols.insert(vol);
        }

        // create client talking to node 'uuid' and pass it to CatalogSync
        netMetaSyncClientSession* meta_client = create_metaSync_client(uuid, omclient);

        // Tell CatalogSync to start the sync process
        // TODO(xxx) only start max_syn_inprogress syncs, but for now starting all
        catsync->startSync(&vols, meta_client,
                           std::bind(&CatalogSyncMgr::syncDoneCb, this,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::placeholders::_3,
                                     std::placeholders::_4));
    }

    return err;
}

Error
CatalogSyncMgr::startCatalogSyncDelta(const std::string& context) {
    Error err(ERR_OK);

    fds_mutex::scoped_lock l(cat_sync_lock);
    // sync must be in progress
    if (!sync_in_progress) {
        LOGWARN << "Will Not start delta sync because catsync not in progress!";
        return ERR_CATSYNC_NOT_PROGRESS;
    }
    fds_verify(cat_sync_map.size() > 0);
    // make sure that all CatalogSync objects finished the initial sync
    for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
         cit != cat_sync_map.cend();
         ++cit) {
        if ((cit->second)->isInitialSyncDone() == false) {
            // make sure this method is called on DMT commit from OM
            fds_panic("Cannot start delta sync if initial sync is not done!!!");
        }
    }
    // update context to return with callback
    cat_sync_context = context;
    // start delta sync on all our CatalogSync objects
    for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
         cit != cat_sync_map.cend();
         ++cit) {
        (cit->second)->doDeltaSync();
    }
    return err;
}

Error CatalogSyncMgr::forwardCatalogUpdate(dmCatReq  *updCatReq) {
    Error err(ERR_OK);
    fds_bool_t found_volume = false;

    fds_mutex::scoped_lock l(cat_sync_lock);
    // noop if sync is in not in progress; or return error?
    if (!sync_in_progress) {
        return err;
    }

    // find CatalogSync that is responsible for this volume
    for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
         cit != cat_sync_map.cend();
         ++cit) {
        if ((cit->second)->hasVolume(updCatReq->volId)) {
            LOGDEBUG << "FORWARD:sync catalog update for volume "
                     << std::hex << updCatReq->volId << std::dec;
            err = (cit->second)->forwardCatalogUpdate(updCatReq);
            found_volume = true;
            break;
        }
    }

    // must be called only for volumes for which sync is in progress!
    // otherwise make sure that VolumeMeta has correct forwarding state/flag
    fds_verify(found_volume);

    return err;
}


void CatalogSyncMgr::finishedForwardVolmeta(fds_volid_t volid) {
    Error err(ERR_OK);
    fds_bool_t send_dmt_close_ack = false;

    {  // begin scoped lock
        fds_mutex::scoped_lock l(cat_sync_lock);
        fds_verify(sync_in_progress);
        for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
             cit != cat_sync_map.cend();
             ++cit) {
            if ((cit->second)->hasVolume(volid)) {
                LOGDEBUG << "DEL-VOL: Map Clean up "
                         << std::hex << volid << std::dec;
                err = (cit->second)->handleVolumeDone(volid);
                if (((cit->second)->emptyVolume())) 
                    cat_sync_map.erase(cit);
                break; 
            }
        }
        send_dmt_close_ack = cat_sync_map.empty();
        sync_in_progress = !send_dmt_close_ack;
    }  // end of scoped lock

    // TODO(xxx) if error, means we couldn't send meta sync done
    // notification to destination DM, so destination DM will not
    // move to the right state of processing requests from AM
    // we should retry? or/and send error to OM?

    if (send_dmt_close_ack) {
        fpi::FDSP_DmtCloseTypePtr dmtCloseAck(new FDSP_DmtCloseType);
        dataMgr->omClient->sendDMTCloseAckToOM(dmtCloseAck, cat_sync_context);
    }
}

/**
 * Called when initial or delta sync is finished for a given
 * volume 'volid'
 */
void CatalogSyncMgr::syncDoneCb(catsync_notify_evt_t event,
                                fds_volid_t volid,
                                OMgrClient* omclient,
                                const Error& error) {
    fds_bool_t send_ack = false;
    {  // check if all cat sync jobs are finished
        fds_mutex::scoped_lock l(cat_sync_lock);
        if (sync_in_progress) {
            send_ack = true;
            for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
                 cit != cat_sync_map.cend();
                 ++cit) {
                if (event == CATSYNC_INITIAL_SYNC_DONE) {
                    if ((cit->second)->isInitialSyncDone() == false) {
                        send_ack = false;
                        break;
                    }
                } else {
                    fds_verify(event == CATSYNC_DELTA_SYNC_DONE);
                    if ((cit->second)->isDeltaSyncDone() == false) {
                        send_ack = false;
                        break;
                    }
                }
            }
        }
    }

    LOGNORMAL << "Sync is finished for volume " << std::hex << volid
              << std::dec << " " << error;

    if ((event == CATSYNC_INITIAL_SYNC_DONE) && send_ack) {
        LOGNORMAL << "PushMeta finished for all volumes";
        omclient->sendDMTPushMetaAck(error, cat_sync_context);
    } else if ((event == CATSYNC_DELTA_SYNC_DONE) && send_ack) {
        LOGNORMAL << "Delta sync finished for all volumes, sending commit ack";
        omclient->sendDMTCommitAck(error, cat_sync_context);
    }
}

void
FDSP_MetaSyncRpc::PushMetaSyncReq(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                         fpi::FDSP_UpdateCatalogTypePtr& meta_req)
{
    Error err(ERR_OK);
    LOGNORMAL << "Received PushMetaSyncReq Rpc message for vol 0x"
              << std::hex  << fdsp_msg->glob_volume_id << std::dec;
    dmCatReq *metaUpdReq = new(std::nothrow) dmCatReq(fdsp_msg->glob_volume_id,
                                                      meta_req->blob_name,
                                                      meta_req->dm_transaction_id,
                                                      meta_req->dm_operation,
                                                      fdsp_msg->src_ip_lo_addr,
                                                      fdsp_msg->dst_ip_lo_addr,
                                                      fdsp_msg->src_port, fdsp_msg->dst_port,
                                                      fdsp_msg->session_uuid, 0,
                                                      FDS_DM_FWD_CAT_UPD, meta_req);
    if (metaUpdReq) {
        err = dataMgr->catSyncRecv->enqueueFwdUpdate(metaUpdReq);
    } else {
        LOGCRITICAL << "Failed to allocate metaUpdReq";
        err = ERR_OUT_OF_MEMORY;
    }

    // TODO(xxx) handle the error!
}

void
FDSP_MetaSyncRpc::PushMetaSyncResp(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                                   fpi::FDSP_UpdateCatalogTypePtr& meta_resp) {
    Error err(ERR_OK);
    LOGDEBUG << "Received PushMetaSyncResp for volume "
             << std::hex << fdsp_msg->glob_volume_id << std::dec;
    dmCatReq *metaUpdResp = new(std::nothrow) dmCatReq(fdsp_msg->glob_volume_id,
                                                      meta_resp->blob_name,
                                                      meta_resp->dm_transaction_id,
                                                      meta_resp->dm_operation,
                                                      fdsp_msg->src_ip_lo_addr,
                                                      fdsp_msg->dst_ip_lo_addr,
                                                      fdsp_msg->src_port, fdsp_msg->dst_port,
                                                      fdsp_msg->session_uuid, 0,
                                                      FDS_DM_FWD_CAT_UPD, meta_resp);
    if (metaUpdResp) {
        dataMgr->sendUpdateCatalogResp(metaUpdResp);
    } else {
        LOGCRITICAL << "Failed to send the  metaUpdResp";
        err = ERR_OUT_OF_MEMORY;
    }
}

void
FDSP_MetaSyncRpc::MetaSyncDone(fpi::FDSP_MsgHdrTypePtr& fdsp_msg,
                               fpi::FDSP_VolMetaStatePtr& vol_meta) {
    LOGNORMAL << "Received MetaSyncDone Rpc message "
              << " forward done? " << vol_meta->forward_done;
    if (!vol_meta->forward_done) {
        // open catalogs so we can start processing updates
        fds_verify(dataMgr->vol_meta_map.count(vol_meta->vol_uuid) > 0);
        VolumeMeta *vm = dataMgr->vol_meta_map[vol_meta->vol_uuid];
        vm->openCatalogs(vol_meta->vol_uuid);
        // start processing forwarded updates
        dataMgr->catSyncRecv->startProcessFwdUpdates(vol_meta->vol_uuid);
    } else {
        dataMgr->catSyncRecv->handleFwdDone(vol_meta->vol_uuid);
    }
}

}  // namespace fds
