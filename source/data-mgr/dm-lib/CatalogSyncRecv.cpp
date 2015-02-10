/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <new>
#include <DataMgr.h>
#include <CatalogSyncRecv.h>

namespace fds {

CatSyncReceiver::CatSyncReceiver(DmIoReqHandler* dm_req_hdlr)
        : dm_req_handler(dm_req_hdlr),
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

    // make sure we don't have previous receiver
    if (vol_recv_map.count(volid) > 0) {
        vol_recv_map.erase(volid);
    }
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
void CatSyncReceiver::handleFwdDone(fds_volid_t volid) {
    LOGDEBUG << "Wait until all updates are drained for shadow queue "
             << "of volume " << std::hex << volid << std::dec;

    fds_mutex::scoped_lock l(cat_recv_lock);

    // we must have receiver created for this volume
    fds_verify(vol_recv_map.count(volid) > 0);

    // change vol receiver state to finish fwd
    VolReceiverPtr volrecv = vol_recv_map[volid];
    fds_verify(volrecv->state == VSYNC_RECV_FWD_INPROG);
    volrecv->state = VSYNC_RECV_FWD_FINISHING;

    LOGNORMAL << "Finished receiving vol meta for volume "
              << std::hex << volid << std::dec;
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
    // shadow queue and set state to FWD_INPROG

    // enqueue request into shadow queue
    err = dm_req_handler->enqueueMsg(shadowVolUuid(fwdReq->volId), fwdReq);
    if (err.ok()) {
        volrecv->pending++;
    }
    return err;
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
