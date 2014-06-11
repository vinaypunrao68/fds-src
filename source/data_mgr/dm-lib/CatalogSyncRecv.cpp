/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <new>
#include <DataMgr.h>
#include <CatalogSyncRecv.h>

namespace fds {

CatSyncReceiver::CatSyncReceiver(DmIoReqHandler* dm_req_hdlr,
                                 vmeta_recv_done_handler_t done_evt_hdlr)
        : dm_req_handler(dm_req_hdlr),
          done_evt_handler(done_evt_hdlr),
          cat_recv_lock("Catalog Sync Receiver") {
    LOGNORMAL << "Constructing CatSyncReceiver";
}

CatSyncReceiver::~CatSyncReceiver() {
}

//
// Setup receiver for the volume (shadow qos queue, appropriate state, etc)
//
Error CatSyncReceiver::startRecvVolmeta(fds_volid_t volid,
                                        FDS_VolumeQueue* shadowQueue) {
    Error err(ERR_OK);
    LOGDEBUG << "Will allocate receiver for meta of volume "
             << std::hex << volid << std::dec;

    fds_mutex::scoped_lock l(cat_recv_lock);

    // we must not have receiver created for this volume
    fds_verify(vol_recv_map.count(volid) == 0);

    // create receiver for this volume
    VolReceiver* volrecv = new(std::nothrow) VolReceiver(volid, shadowQueue);
    if (volrecv) {
        vol_recv_map[volid] = VolReceiverPtr(volrecv);
    } else {
        LOGERROR << "Failed to allocate volume receiver for volume "
                 << std::hex << volid << std::dec;
        err = ERR_OUT_OF_MEMORY;
    }

    return err;
}

//
// Unblock shadow qos queue so that we start processing forwarded updates
//
void CatSyncReceiver::startProcessFwdUpdates(fds_volid_t volid) {
    LOGDEBUG << "Will activate shadow qos queue for volume "
             << std::hex << volid << std::dec;

    fds_mutex::scoped_lock l(cat_recv_lock);

    // we must have receiver created for this volume
    fds_verify(vol_recv_map.count(volid) > 0);

    // unblock shadow qos queue and change receiver state
    VolReceiverPtr volrecv = vol_recv_map[volid];
    fds_verify(volrecv->state == VSYNC_RECV_SYNCING);
    volrecv->state = VSYNC_RECV_FWD_INPROG;
    volrecv->shadowVolQueue->activate();
}

//
// Called when source DM finished forwarding updates, so change
// volume's receiver state to finishing forwarding, so that when
// shadow qos queue is drained, we will activate volume's qos queue
// If shadow queue is already empty, we activate volume's queue right away.
//
Error CatSyncReceiver::handleFwdDone(fds_volid_t volid) {
    Error err(ERR_OK);
    LOGDEBUG << "Wait until all updates are drained for shadow queue "
             << "of volume " << std::hex << volid << std::dec;

    fds_mutex::scoped_lock l(cat_recv_lock);

    // we must have receiver created for this volume
    fds_verify(vol_recv_map.count(volid) > 0);

    // change vol receiver state to finish fwd
    VolReceiverPtr volrecv = vol_recv_map[volid];
    fds_verify(volrecv->state == VSYNC_RECV_FWD_INPROG);
    volrecv->state = VSYNC_RECV_FWD_FINISHING;

    if (volrecv->shadowVolQueue->volQueue->empty()) {
        // we are done, notify DataMgr that will activate qos queue
        recvdVolMetaLocked(volid);
    }
    // do not access volrecv after this point!

    return err;
}

//
// Enqueue forwarded update into volume's shadow queue
//
Error CatSyncReceiver::enqueueFwdUpdate(dmCatReq* updReq) {
    Error err(ERR_OK);

    LOGDEBUG << "Will queue cat update to shadow queue of vol "
             << std::hex << updReq->volId <<std::dec;

    fds_mutex::scoped_lock l(cat_recv_lock);

    // we must have receiver created for this volume
    fds_verify(vol_recv_map.count(updReq->volId) > 0);
    VolReceiverPtr volrecv = vol_recv_map[updReq->volId];
    fds_verify(volrecv->state != VSYNC_RECV_SYNCING);

    // enqueue request into shadow queue
    err = dm_req_handler->enqueueMsg(shadowVolUuid(updReq->volId), updReq);
    return err;
}

void
CatSyncReceiver::fwdUpdateReqDone(dmCatReq* updCatReq,
                                  blob_version_t blob_version,
                                  const Error& error,
                                  MetaSyncRespHandlerPrx respCli) {
    Error err(ERR_OK);
    LOGTRACE << "commited forwarded update for vol " << std::hex
             << updCatReq->volId << std::dec << " " << error;

    {  // scoped lock
        fds_mutex::scoped_lock l(cat_recv_lock);

        // we must have receiver create for this volume
        fds_verify(vol_recv_map.count(updCatReq->volId) > 0);
        VolReceiverPtr volrecv = vol_recv_map[updCatReq->volId];

        if (volrecv->state == VSYNC_RECV_FWD_FINISHING) {
            // shadow queue is empty, we finished getting vol meta for volid
            if (volrecv->shadowVolQueue->volQueue->empty()) {
                recvdVolMetaLocked(updCatReq->volId);
            }
        }
    }  // end of scoped lock

    // reply back to source DM
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr(new FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr
            update_catalog(new FDSP_UpdateCatalogType);
    DataMgr::InitMsgHdr(msg_hdr);
    update_catalog->obj_list.clear();
    update_catalog->meta_list.clear();

    if (error.ok()) {
        msg_hdr->result  = fpi::FDSP_ERR_OK;
        msg_hdr->err_msg = "Dude, you're good to go!";
    } else {
        msg_hdr->result   = fpi::FDSP_ERR_FAILED;
        msg_hdr->err_msg  = "Something hit the fan...";
        msg_hdr->err_code = error.GetErrno();
        update_catalog->blob_version = blob_version_invalid;
    }
    msg_hdr->msg_code = fpi::FDSP_MSG_UPDATE_CAT_OBJ_RSP;
    msg_hdr->glob_volume_id =  updCatReq->volId;
    msg_hdr->req_cookie =  updCatReq->reqCookie;

    update_catalog->blob_name = updCatReq->blob_name;
    update_catalog->blob_version = blob_version;

    update_catalog->dm_transaction_id = updCatReq->transId;
    update_catalog->dm_operation = updCatReq->transOp;

    try {
        respCli->PushMetaSyncResp(msg_hdr, update_catalog);
        LOGNORMAL << "Sent PushMetaSyncResp Rpc message";
    } catch(...) {
        LOGERROR << "Unable to send PushMetaSyncResp to DM";
        err = ERR_NETWORK_TRANSPORT;
    }

    // TODO(xxx) what do we do with network error? would this
    // be handled inside our session layer? for now ignoring
}

void CatSyncReceiver::recvdVolMetaLocked(fds_volid_t volid) {
    LOGNORMAL << "Finished receiving vol meta for volume "
              << std::hex << volid << std::dec;

    fds_verify(done_evt_handler);
    done_evt_handler(volid, ERR_OK);
    // cleanup receiver
    vol_recv_map.erase(volid);
}

CatSyncReceiver::VolReceiver::VolReceiver(fds_volid_t volid,
                                          FDS_VolumeQueue* shadowQueue)
        : state(VSYNC_RECV_SYNCING),
          shadowVolQueue(shadowQueue) {
    // initially shadow volume queue is blocked, until we
    // get initial meta push via rsync
    shadowVolQueue->stopDequeue();
}

CatSyncReceiver::VolReceiver::~VolReceiver() {
    // unregister and remove shadow qos queue
    if (shadowVolQueue) {
        delete shadowVolQueue;
    }
}

}  // namespace fds
