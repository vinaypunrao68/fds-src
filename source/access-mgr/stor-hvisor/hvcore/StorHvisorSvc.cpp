/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include <fds_config.hpp>
#include <fds_process.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetSession.h"
#include <dlt.h>
#include <ObjectId.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <fdsp/DMSvc.h>
#include <fdsp/SMSvc.h>
#include <fdsp_utils.h>

#include "requests/requests.h"

extern StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;

std::atomic_uint nextIoReqId;

/**
 * Function called when volume waiting queue is drained.
 * When it's called a volume has just been attached and
 * we can call the callback to tell any waiters that the
 * volume is now ready.
 */
void
StorHvCtrl::attachVolume(AmRequest *amReq) {
    // Get request from qos object
    fds_verify(amReq != NULL);
    AttachVolBlobReq *blobReq = static_cast<AttachVolBlobReq *>(amReq);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_ATTACH_VOL);

    LOGDEBUG << "Attach for volume " << blobReq->volume_name
             << " complete. Notifying waiters";
    blobReq->cb->call(ERR_OK);
}

void
StorHvCtrl::enqueueAttachReq(const std::string& volumeName,
                             CallbackPtr cb) {
    LOGDEBUG << "Attach request for volume " << volumeName;

    // check if volume is already attached
    fds_volid_t volId = invalid_vol_id;
    if (invalid_vol_id != (volId = vol_table->getVolumeUUID(volumeName))) {
        LOGDEBUG << "Volume " << volumeName
                 << " with UUID " << volId
                 << " already attached";
        cb->call(ERR_OK);
        return;
    }

    // Create a noop request to put into wait queue
    AttachVolBlobReq *blobReq = new AttachVolBlobReq(volId, volumeName, cb);

    // Enqueue this request to process the callback
    // when the attach is complete
    vol_table->addBlobToWaitQueue(volumeName, blobReq);

    fds_verify(sendTestBucketToOM(volumeName,
                                  "",  // The access key isn't used
                                  "") == ERR_OK); // The secret key isn't used
}

Error
StorHvCtrl::pushBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);
    Error err(ERR_OK);

    PerfTracer::tracePointBegin(blobReq->e2e_req_perf_ctx); 
    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx); 

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
    fds_volid_t volId = blobReq->io_vol_id;

    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    if ((shVol == NULL) || (shVol->volQueue == NULL)) {
        if (shVol)
            shVol->readUnlock();
        LOGERROR << "Volume and queueus are NOT setup for volume " << volId;
        err = ERR_INVALID_ARG;
        PerfTracer::tracePointEnd(blobReq->qos_perf_ctx); 
        delete blobReq;
        return err;
    }
    /*
     * TODO: We should handle some sort of success/failure here?
     */
    qos_ctrl->enqueueIO(volId, blobReq);
    shVol->readUnlock();

    LOGDEBUG << "Queued IO for vol " << volId;

    return err;
}

void
StorHvCtrl::enqueueBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);

    // check if volume is attached to this AM
    if (invalid_vol_id == (blobReq->io_vol_id = vol_table->getVolumeUUID(blobReq->volume_name))) {
        vol_table->addBlobToWaitQueue(blobReq->volume_name, blobReq);
        fds_verify(sendTestBucketToOM(blobReq->volume_name,
                                      "",  // The access key isn't used
                                      "") == ERR_OK); // The secret key isn't used
        return;
    }

    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx);

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);

    fds_verify(qos_ctrl->enqueueIO(blobReq->io_vol_id, blobReq) == ERR_OK);
}
