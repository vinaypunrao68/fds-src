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

    if (volrecv->pending == 0) {
        // we are done, notify DataMgr that will activate qos queue
        recvdVolMetaLocked(volid);
    }
    // do not access volrecv after this point!

    return err;
}

//
// Enqueue forwarded update into volume's shadow queue
//
Error CatSyncReceiver::enqueueFwdUpdate(DmIoFwdCat* fwdReq) {
    Error err(ERR_OK);
    LOGDEBUG << "Will queue fwd cat update to shadow queue: " << *fwdReq;

    fds_mutex::scoped_lock l(cat_recv_lock);

    // we must have receiver created for this volume
    fds_verify(vol_recv_map.count(fwdReq->volId) > 0);
    VolReceiverPtr volrecv = vol_recv_map[fwdReq->volId];
    // fwd update can arrive in any state, even before we unblock
    // shadow queeu and set state to FWD_INPROG

    // enqueue request into shadow queue
    err = dm_req_handler->enqueueMsg(shadowVolUuid(fwdReq->volId), fwdReq);
    if (err.ok()) {
        volrecv->pending++;
    }
    return err;
}

void
CatSyncReceiver::fwdUpdateReqDone(fds_volid_t volume_id) {
    Error err(ERR_OK);
    LOGTRACE << "commited forwarded update for vol " << std::hex
             << volume_id << std::dec;

    {  // scoped lock
        fds_mutex::scoped_lock l(cat_recv_lock);

        // we must have receiver create for this volume
        fds_verify(vol_recv_map.count(volume_id) > 0);
        VolReceiverPtr volrecv = vol_recv_map[volume_id];
        fds_verify(volrecv->pending > 0);
        volrecv->pending--;

        if (volrecv->state == VSYNC_RECV_FWD_FINISHING) {
            // shadow queue is empty, we finished getting vol meta for volid
            LOGDEBUG << "FWD_FINISHING vol " << std::hex << volume_id
                     << std::dec << " pending " << volrecv->pending;
            if (volrecv->pending == 0) {
                recvdVolMetaLocked(volume_id);
            }
        }
    }  // end of scoped lock
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
          shadowVolQueue(shadowQueue),
          pending(0) {
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
