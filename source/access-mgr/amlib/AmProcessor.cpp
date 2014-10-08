/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <string>
#include <fds_process.h>
#include <AmProcessor.h>

namespace fds {

#define AMPROCESSOR_CB_HANDLER(func, ...) \
    std::bind(&func, this, ##__VA_ARGS__ , std::placeholders::_1)

AmProcessor::AmProcessor(const std::string &modName,
                         AmDispatcher::shared_ptr _amDispatcher,
                         StorHvQosCtrl     *_qosCtrl,
                         StorHvVolumeTable *_volTable,
                         AmTxManager::shared_ptr _amTxMgr,
                         AmCache::shared_ptr _amCache)
        : Module(modName.c_str()),
          amDispatcher(_amDispatcher),
          qosCtrl(_qosCtrl),
          volTable(_volTable),
          txMgr(_amTxMgr),
          amCache(_amCache) {
    randNumGen = RandNumGenerator::unique_ptr(
        new RandNumGenerator(RandNumGenerator::getRandSeed()));
}

AmProcessor::~AmProcessor() {
}

int
AmProcessor::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
AmProcessor::mod_startup() {
}

void
AmProcessor::mod_shutdown() {
}

void
AmProcessor::startBlobTx(AmQosReq *qosReq) {
    // Get the blob request
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    // check if this is a snapshot
    // TODO(Andrew): Why not just let DM reject the IO?
    StorHvVolume *shVol = volTable->getLockedVolume(blobReq->getVolId());
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                      blobReq->cb);
        qosCtrl->markIODone(qosReq);
        cb->call(FDSN_StatusErrorAccessDenied);
        delete blobReq;
    }
    shVol->readUnlock();

    // Generate a random transaction ID to use
    blobReq->txId = BlobTxId(randNumGen->genNumSafe());
    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::startBlobTxCb, qosReq);

    amDispatcher->dispatchStartBlobTx(qosReq);
}

void
AmProcessor::startBlobTxCb(AmQosReq *qosReq,
                           const Error &error) {
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  blobReq->cb);

    if (error.ok()) {
        // Update callback and record new open transaction
        cb->blobTxId  = blobReq->txId;
        fds_verify(txMgr->addTx(blobReq->getVolId(),
                                blobReq->txId,
                                blobReq->dmtVersion,
                                blobReq->getBlobName()) == ERR_OK);
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    cb->call(error.GetErrno());
}

void
AmProcessor::getBlob(AmQosReq *qosReq) {
    // Pull out the Get request
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(
        qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    fds_volid_t volId = blobReq->getVolId();
    StorHvVolume *shVol = volTable->getVolume(volId);
    if ((shVol == NULL) || (!shVol->isValidLocked())) {
        LOGCRITICAL << "getBlob failed to get volume for vol " << volId;
        getBlobCb(qosReq, ERR_INVALID);
        return;
    }

    // TODO(Anna) We are doing update catalog using absolute
    // offsets, so we need to be consistent in query catalog
    // Review this in the next sprint!
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    blobReq->setBlobOffset(blobReq->getBlobOffset() * maxObjSize);
    blobReq->base_vol_id = volTable->getBaseVolumeId(blobReq->getVolId());

    // Check cache for object ID
    Error err = ERR_OK;
    ObjectID::ptr objectId = amCache->getBlobOffsetObject(volId,
                                                          blobReq->getBlobName(),
                                                          blobReq->getBlobOffset(),
                                                          err);
    // ObjectID was found in the cache
    if (err == ERR_OK) {
        // TODO(Andrew): Consider adding this back when we revisit
        // zero length objects
        // fds_verify(*objectId != NullObjectID);

        // Check cache for object data
        boost::shared_ptr<std::string> objectData = amCache->getBlobObject(volId,
                                                                           *objectId,
                                                                           err);
        if (err == ERR_OK) {
            // Data was found in cache, so fill data and callback
            LOGTRACE << "Found cached object " << *objectId;

            // Only return UP-TO the amount of data requested, never more
            GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);
            cb->returnSize = std::min(blobReq->getDataLen(), objectData->size());
            memcpy(cb->returnBuffer, objectData->c_str(), cb->returnSize);

            getBlobCb(qosReq, err);
        } else {
            // We couldn't find the data in the cache even though the id was
            // obtained there. Fallback to retrieving the data from the SM.
            blobReq->setObjId(*objectId);
            blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, qosReq);
            amDispatcher->dispatchGetBlob(qosReq);
        }
    } else {
        blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::queryCatalogCb, qosReq);
        amDispatcher->dispatchQueryCatalog(qosReq);
    }
}

void
AmProcessor::getBlobCb(AmQosReq *qosReq, const Error& error) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);

    // TODO(Andrew): Update the AM's blob object cache here

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    cb->call(error);
    delete blobReq;
}

void
AmProcessor::queryCatalogCb(AmQosReq *qosReq, const Error& error) {
    if (error == ERR_OK) {
        fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(
            qosReq->getBlobReqPtr());
        blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, qosReq);
        amDispatcher->dispatchGetBlob(qosReq);
    } else {
        getBlobCb(qosReq, error);
    }
}

}  // namespace fds
