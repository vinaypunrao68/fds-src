/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <set>
#include <util/Log.h>
#include <util/timeutils.h>
#include <fds_assert.h>
#include <fds_typedefs.h>
#include <lib/OMgrClient.h>
#include <CatalogSync.h>
#include <DataMgr.h>
#include <VolumeMeta.h>
#include <DMSvcHandler.h>

namespace fds {

/****** CatalogSync implementation ******/

CatalogSync::CatalogSync(const NodeUuid& uuid,
                         OMgrClient* omclient,
                         DmIoReqHandler* dm_req_hdlr)
        : node_uuid(uuid),
          state(CSSTATE_READY),
          om_client(omclient),
          dm_req_handler(dm_req_hdlr) {
    state = ATOMIC_VAR_INIT(CSSTATE_READY);
    vols_done = ATOMIC_VAR_INIT(0);
}

CatalogSync::~CatalogSync() {
}

/**
 * On return, set 'volumes' will be empty
 */
Error CatalogSync::startSync(std::set<fds_volid_t>* volumes,
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
    Error err = issueVolSyncStateMsg(volid, false);
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

void CatalogSync::handleVolumeDone(fds_volid_t volid) {
    fds_verify(sync_volumes.count(volid) > 0);
    sync_volumes.erase(volid);
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

Error CatalogSync::forwardCatalogUpdate(DmIoCommitBlobTx *commitBlobReq,
                                        blob_version_t blob_version,
                                        const BlobObjList::const_ptr& blob_obj_list,
                                        const MetaDataList::const_ptr& meta_list) {
    Error err(ERR_OK);
    csStateType cur_state = std::atomic_load(&state);
    fds_verify(hasVolume(commitBlobReq->volId));
    fds_verify((cur_state == CSSTATE_DELTA_SYNC) ||
               (cur_state == CSSTATE_FORWARD_ONLY));

    LOGDEBUG << "Forwarding cat update for vol " << std::hex << commitBlobReq->volId
             << std::dec << " blob " << commitBlobReq->blob_name;

    ForwardCatalogMsgPtr fwdMsg(new ForwardCatalogMsg());
    fwdMsg->volume_id = commitBlobReq->volId;
    fwdMsg->blob_name = commitBlobReq->blob_name;
    fwdMsg->blob_version = blob_version;
    blob_obj_list->toFdspPayload(fwdMsg->obj_list);
    meta_list->toFdspPayload(fwdMsg->meta_list);

    // send forward cat update, and pass commitBlobReq as context so we can
    // reply to AM on fwd cat update response
    auto asyncCatUpdReq = gSvcRequestPool->newEPSvcRequest(this->node_uuid.toSvcUuid());
    asyncCatUpdReq->setPayload(FDSP_MSG_TYPEID(fpi::ForwardCatalogMsg), fwdMsg);
    asyncCatUpdReq->setTimeoutMs(5000);
    asyncCatUpdReq->onResponseCb(RESPONSE_MSG_HANDLER(CatalogSync::fwdCatalogUpdateMsgResp,
                                                      commitBlobReq));
    asyncCatUpdReq->invoke();

    return err;
}

void CatalogSync::fwdCatalogUpdateMsgResp(DmIoCommitBlobTx *commitReq,
                                          EPSvcRequest* req,
                                          const Error& error,
                                          boost::shared_ptr<std::string> payload) // NOLINT                                                                                      
{
    LOGDEBUG << "Received fwd cat update response" << *commitReq << " " << error;
    commitReq->dmio_commit_blob_tx_resp_cb(error, commitReq);
}

/***** CatalogSyncManager implementation ******/

CatalogSyncMgr::CatalogSyncMgr(fds_uint32_t max_jobs,
                               DmIoReqHandler* dm_req_hdlr)
        : Module("CatalogSyncMgr"),
          sync_in_progress(false),
          max_sync_inprogress(max_jobs),
          dm_req_handler(dm_req_hdlr),
          cat_sync_lock("Catalog Sync lock"),
          dmtclose_ts(util::getTimeStampNanos()) {
    LOGNORMAL << "Constructing CatalogSyncMgr";
}

CatalogSyncMgr::~CatalogSyncMgr() {
}

/**
 * Module start up code
 */
void CatalogSyncMgr::mod_startup()
{
}

/**
 * Module shutdown code
 */
void CatalogSyncMgr::mod_shutdown()
{
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

        // Tell CatalogSync to start the sync process
        // TODO(xxx) only start max_syn_inprogress syncs, but for now starting all
        catsync->startSync(&vols,
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

Error CatalogSyncMgr::forwardCatalogUpdate(DmIoCommitBlobTx *commitBlobReq,
                                           blob_version_t blob_version,
                                           const BlobObjList::const_ptr& blob_obj_list,
                                           const MetaDataList::const_ptr& meta_list) {
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
        if ((cit->second)->hasVolume(commitBlobReq->volId)) {
            LOGDEBUG << "FORWARD:sync catalog update for volume "
                     << std::hex << commitBlobReq->volId << std::dec;
            err = (cit->second)->forwardCatalogUpdate(commitBlobReq, blob_version,
                                                      blob_obj_list, meta_list);
            found_volume = true;
            break;
        }
    }

    // must be called only for volumes for which sync is in progress!
    // otherwise make sure that VolumeMeta has correct forwarding state/flag
    fds_verify(found_volume);

    return err;
}

//
// Sends msg to dst DM that push meta finished, so that DM can open
// up volume 'volid' qos queue
//
Error CatalogSyncMgr::issueServiceVolumeMsg(fds_volid_t volid) {
    Error err(ERR_OK);
    fds_mutex::scoped_lock l(cat_sync_lock);
    fds_verify(sync_in_progress);
    for (CatSyncMap::const_iterator cit = cat_sync_map.cbegin();
         cit != cat_sync_map.cend();
         ++cit) {
        if ((cit->second)->hasVolume(volid)) {
            err = (cit->second)->issueVolSyncStateMsg(volid, true);
            if (!err.ok()) return err;
        }
    }
    return err;
}

//
// returns true if CatalogSync finished forwarding meta for all volumes
// and sent DMT close ack
//
fds_bool_t CatalogSyncMgr::finishedForwardVolmeta(fds_volid_t volid) {
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
                (cit->second)->handleVolumeDone(volid);
                if (((cit->second)->emptyVolume())) {
                    cat_sync_map.erase(cit);
                    LOGDEBUG << "cat sync map erase: " << std::hex
                             << volid << std::dec;
                    // TODO(Anna) fix for case when DM pushes meta
                    // for same volume to multiple dest DMs
                    break;
                }
            }
        }
        send_dmt_close_ack = cat_sync_map.empty();
        LOGDEBUG << "send_dmt_close_ack :  " << send_dmt_close_ack;
        sync_in_progress = !send_dmt_close_ack;
    }  // end of scoped lock

    if (send_dmt_close_ack) {
        if (dataMgr->sendDmtCloseCb != nullptr) {
            dataMgr->sendDmtCloseCb();
        } else {
            LOGDEBUG << "sendDmtCloseCb called while ptr was NULL!!!";
        }
    }

    return send_dmt_close_ack;
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

// sets dmt close time to now
void CatalogSyncMgr::setDmtCloseNow() {
    dmtclose_ts = util::getTimeStampNanos();
}

/**
 * Send message that rsync has finished. From Master DM to new
 * DM. Uses VolSyncStateMsg.
 *  
 * @param[in] volId volume ID of catalog volume
 * @param[in] forward_complete boolean specifying whether forwarding is
 * complete (true = forwarding complete; false = second rsync
 * complete)
 *
 * @return Error code
 */
Error CatalogSync::issueVolSyncStateMsg(fds_volid_t volId,
                                        fds_bool_t forward_complete)
{
    Error err(ERR_OK);
    VolSyncStateMsgPtr fwdMsg(new VolSyncStateMsg());

    fwdMsg->volume_id = volId;
    fwdMsg->forward_complete = forward_complete;

    LOGDEBUG << "Sending VolSyncStateMsg: " << std::hex
             << volId << std::dec << " fwd_complete: " << forward_complete;

    auto asyncFwdReq = gSvcRequestPool->newEPSvcRequest(this->node_uuid.toSvcUuid());
    asyncFwdReq->setPayload(FDSP_MSG_TYPEID(fpi::VolSyncStateMsg), fwdMsg);
    asyncFwdReq->setTimeoutMs(5000);
    // TODO(brian): Set callback and do error management here
    asyncFwdReq->invoke();

    return err;
}

}  // namespace fds
