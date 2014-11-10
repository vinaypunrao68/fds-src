/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <TierMigration.h>

namespace fds {

SmTierMigration::SmTierMigration(SmIoReqHandler *data_store)
        : dataStoreReqHandler(data_store),
          writeBackReq(NULL) {
    // default tier config
    tierConfig.flashFullThreshold = 90;
    // TODO(Anna) implement the timer to write back if we don't get
    // enough SSD puts to batch in some time interval; for now
    // setting batch size to one; will fix this before end of sprint
    tierConfig.maxWritebackBatchSize = 1;
    tierConfig.maxPromoteBatchSize = 10;

    createWritebackReq();
}

SmTierMigration::~SmTierMigration() {
    // ok to drop write-back work that we prepared in writeBackReq
    if (writeBackReq) {
        delete writeBackReq;
    }
}

void
SmTierMigration::setTierConfig(const TieringParams& conf) {
    tierConfig.flashFullThreshold = conf.flashFullThreshold;
    tierConfig.maxWritebackBatchSize = conf.maxWritebackBatchSize;
    tierConfig.maxPromoteBatchSize = conf.maxPromoteBatchSize;
}

/**
 * Handles notification about successful PUT to SSD
 * for a hybrid volume.
 */
Error SmTierMigration::notifyHybridVolFlashPut(const ObjectID& oid) {
    Error err(ERR_OK);
    SmIoMoveObjsToTier* reqToEnq = NULL;
    {  // scope for mutex
        fds_mutex::scoped_lock l(writeBackLock);
        fds_verify(writeBackReq != NULL);
        (writeBackReq->oidList).push_back(oid);
        if ((writeBackReq->oidList).size() >= tierConfig.maxWritebackBatchSize) {
            reqToEnq = writeBackReq;
            writeBackReq = NULL;
            createWritebackReq();
        }
    }

    // check if started filling in a new request, and need to schedule
    // qos work for writeback
    if (reqToEnq) {
        err = dataStoreReqHandler->enqueueMsg(FdsSysTaskQueueId, reqToEnq);
        if (!err.ok()) {
            LOGERROR << "Failed to enqueue moveTier request " << err;
            // should be ok to drop writeback requests, should be
            // rare since our max qos queue size is 4K requests; if we
            // see this error message often, try to create a separate qos
            // queue from common system queue (e.g. may compete with GC)
        }
    }
    return err;
}

void SmTierMigration::createWritebackReq() {
    fds_verify(writeBackReq == NULL);
    writeBackReq = new(std::nothrow) SmIoMoveObjsToTier();
    fds_verify(writeBackReq);
    writeBackReq->io_type = FDS_SM_TIER_WRITEBACK_OBJECTS;
    writeBackReq->fromTier = diskio::flashTier;
    writeBackReq->toTier = diskio::diskTier;
    writeBackReq->relocate = false;
    writeBackReq->moveObjsRespCb = NULL;  // ok if writeback fails
}

}  // namespace fds

