/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <string>
#include <ObjectId.h>
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
AmProcessor::getVolumeMetadata(AmQosReq *qosReq) {
    GetVolumeMetaDataReq* volReq = static_cast<GetVolumeMetaDataReq *>(qosReq->getBlobReqPtr());
    fds_verify(true == volReq->magicInUse());

    // Set the processor callback
    volReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::getVolumeMetadataCb, qosReq);
    amDispatcher->dispatchGetVolumeMetadata(qosReq);
}

void
AmProcessor::getVolumeMetadataCb(AmQosReq *qosReq,
                                 const Error &error) {
    GetVolumeMetaDataReq* volReq = static_cast<GetVolumeMetaDataReq *>(qosReq->getBlobReqPtr());
    fds_verify(true == volReq->magicInUse());

    GetVolumeMetaDataCallback::ptr cb =
            SHARED_DYN_CAST(GetVolumeMetaDataCallback, volReq->cb);
    cb->volumeMetaData = volReq->volumeMetadata;
    qosCtrl->markIODone(qosReq);
    cb->call(error);
}

void
AmProcessor::abortBlobTx(AmQosReq *qosReq) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());

    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::abortBlobTxCb, qosReq);

    amDispatcher->dispatchAbortBlobTx(qosReq);
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
AmProcessor::deleteBlob(AmQosReq *qosReq) {
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(qosReq->getBlobReqPtr());
    fds_volid_t volId = blobReq->getVolId();
    StorHvVolume* shVol = volTable->getLockedVolume(volId);

    LOGDEBUG    << " volume:" << volId
                << " blob:" << blobReq->getBlobName()
                << " txn:" << blobReq->txDesc;

    blobReq->base_vol_id = volTable->getBaseVolumeId(volId);
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    if ((shVol == NULL) || (!shVol->isValidLocked())) {
        LOGCRITICAL << "unable to get volume info for vol: " << volId;
        deleteBlobCb(qosReq, FDSN_StatusErrorUnknown);
        shVol->readUnlock();
        return;
    }

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "delete blob on a snapshot is not allowed.";
        deleteBlobCb(qosReq, FDSN_StatusErrorAccessDenied);
        shVol->readUnlock();
        return;
    }
    shVol->readUnlock();

    // Update the tx manager with the delete op
    txMgr->updateTxOpType(*(blobReq->txDesc), blobReq->getIoType());

    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::deleteBlobCb, qosReq);
    amDispatcher->dispatchDeleteBlob(qosReq);
}

void
AmProcessor::putBlob(AmQosReq *qosReq) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

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
        return;
    }
    shVol->readUnlock();

    // TODO(Andrew): Here we're turning the offset aligned
    // blobOffset back into an absolute blob offset (i.e.,
    // not aligned to the maximum object size). This allows
    // the rest of the putBlob routines to still expect an
    // absolute offset in case we need it
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    blobReq->setBlobOffset(blobReq->getBlobOffset() * maxObjSize);

    // Use a stock object ID if the length is 0.
    if (blobReq->getDataLen() == 0) {
        LOGWARN << "zero size object - "
                << " [objkey:" << blobReq->ObjKey <<"]";
        blobReq->setObjId(ObjectID());
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(blobReq->hashPerfCtx);
        blobReq->setObjId(ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                                blobReq->getDataLen()));
    }

    if (blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
        // Sending the update in a single request. Create transaction ID to
        // use for the single request
        blobReq->setTxId(BlobTxId(randNumGen->genNumSafe()));
    }

    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::putBlobCb, qosReq);

    if (blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
        amDispatcher->dispatchUpdateCatalogOnce(qosReq);
    } else {
        amDispatcher->dispatchUpdateCatalog(qosReq);
    }
    amDispatcher->dispatchPutObject(qosReq);
}

void
AmProcessor::putBlobCb(AmQosReq *qosReq, const Error& error) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    if (error.ok()) {
        // Add the Tx to the manager is this an updateOnce
        if (blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
            fds_verify(txMgr->addTx(blobReq->getVolId(),
                                    *(blobReq->txDesc),
                                    blobReq->dmtVersion,
                                    blobReq->getBlobName()) == ERR_OK);
            // Stage the transaction metadata changes
            fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->txDesc),
                                                   blobReq->metadata) == ERR_OK);
        }
        // Update the tx manager with this update
        fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->getTxId()),
                                               blobReq->getDataLen()) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(txMgr->updateStagedBlobOffset(*(blobReq->getTxId()),
                                                 blobReq->getBlobName(),
                                                 blobReq->getBlobOffset(),
                                                 blobReq->getObjId()) == ERR_OK);
        // Update the transaction manager with the stage object data
        if (blobReq->getDataLen() > 0) {
            fds_verify(txMgr->updateStagedBlobObject(*(blobReq->getTxId()),
                                                     blobReq->getObjId(),
                                                     blobReq->getDataBuf(),
                                                     blobReq->getDataLen())
                   == ERR_OK);
        }

        if (blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
            // Push the commited update to the cache and remove from manager
            // We push here because we ONCE messages don't have an explicit
            // commit and here is where we know we've actually committed
            // to SM and DM.
            // TODO(Andrew): Inserting the entire tx transaction currently
            // assumes that the tx descriptor has all of the contents needed
            // for a blob descriptor (e.g., size, version, etc..). Today this
            // is true for S3/Swift and doesn't get used anyways for block (so
            // the actual cached descriptor for block will not be correct).
            AmTxDescriptor::ptr txDescriptor;
            fds_verify(txMgr->getTxDescriptor(*(blobReq->getTxId()),
                                              txDescriptor) == ERR_OK);
            fds_verify(amCache->putTxDescriptor(txDescriptor) == ERR_OK);
            fds_verify(txMgr->removeTx(*(blobReq->getTxId())) == ERR_OK);
        }
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);
    delete blobReq;
}

void
AmProcessor::getBlob(AmQosReq *qosReq) {
    // Pull out the Get request
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());

    fds_volid_t volId = blobReq->getVolId();
    StorHvVolume *shVol = volTable->getVolume(volId);
    // TODO(bszmyd): Friday, October 10th 2014
    // This logic was copied directly from StorHvCtrl, but it's not
    // quite clear how using the non-locking call above would still
    // allow the following to function. Investigation needed.
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
    if (ERR_OK == err) {
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
            amDispatcher->dispatchGetObject(qosReq);
        }
    } else {
        blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::queryCatalogCb, qosReq);
        amDispatcher->dispatchQueryCatalog(qosReq);
    }
}

void
AmProcessor::setBlobMetadata(AmQosReq *qosReq) {
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(qosReq->getBlobReqPtr());

    // Stage the transaction metadata changes
    fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->getTxId()), blobReq->getMetaDataListPtr()))

    fds_verify(txMgr->getTxDmtVersion(*(blobReq->getTxId()), &(blobReq->dmt_version)));
    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::setBlobMetadataCb, qosReq);

    amDispatcher->dispatchSetBlobMetadata(qosReq);
}

void
AmProcessor::setBlobMetadataCb(AmQosReq *qosReq,
                               const Error &error) {
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(qosReq->getBlobReqPtr());

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);

    delete blobReq;
}

void
AmProcessor::statBlob(AmQosReq *qosReq) {
    Error err(ERR_OK);
    StatBlobReq* blobReq = static_cast<StatBlobReq *>(qosReq->getBlobReqPtr());
    fds_volid_t volId = blobReq->getVolId();

    LOGDEBUG << "volume:" << volId <<" blob:" << blobReq->getBlobName();

    // Check cache for blob descriptor
    BlobDescriptor::ptr cachedBlobDesc = amCache->getBlobDescriptor(volId,
                                                                    blobReq->getBlobName(),
                                                                    err);
    if (ERR_OK == err) {
        LOGTRACE << "Found cached blob descriptor for " << std::hex
            << volId << std::dec << " blob " << blobReq->getBlobName();

        StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, blobReq->cb);
        // Fill in the data here
        cb->blobDesc.setBlobName(cachedBlobDesc->getBlobName());
        cb->blobDesc.setBlobSize(cachedBlobDesc->getBlobSize());
        for (const_kv_iterator meta = cachedBlobDesc->kvMetaBegin();
             meta != cachedBlobDesc->kvMetaEnd();
             ++meta) {
            cb->blobDesc.addKvMeta(meta->first,  meta->second);
        }
        statBlobCb(qosReq, ERR_OK);
        return;
    }
    LOGTRACE << "Did not find cached blob descriptor for " << std::hex
        << volId << std::dec << " blob " << blobReq->getBlobName();

    blobReq->base_vol_id = volTable->getBaseVolumeId(volId);
    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::statBlobCb, qosReq);
    amDispatcher->dispatchStatBlob(qosReq);
}

void
AmProcessor::abortBlobTxCb(AmQosReq *qosReq,
                           const Error &error) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);

    fds_verify(ERR_OK == txMgr->removeTx(*(blobReq->getTxId())));

    delete blobReq;
}

void
AmProcessor::volumeContents(AmQosReq *qosReq) {
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq*>(qosReq->getBlobReqPtr());
    LOGDEBUG << "volume:" << blobReq->getVolId()
        <<" blob:" << blobReq->getBlobName();

    blobReq->base_vol_id = volTable->getBaseVolumeId(blobReq->getVolId());
    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::volumeContentsCb, qosReq);

    amDispatcher->dispatchVolumeContents(qosReq);
}

void
AmProcessor::deleteBlobCb(AmQosReq *qosReq, const Error& error) {
    DeleteBlobReq *blobReq = static_cast<DeleteBlobReq *>(qosReq->getBlobReqPtr());

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);
    delete blobReq;
}


void
AmProcessor::getBlobCb(AmQosReq *qosReq, const Error& error) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());

    if (ERR_OK == error) {
        // TODO(bszmyd): Thu 09 Oct 2014 04:30:52 PM MDT
        // We have successfully retrieved the BLOB, let's stick it in the cache
        // if it didn't already exist there (call is idempotent).
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);
    delete blobReq;
}

void
AmProcessor::queryCatalogCb(AmQosReq *qosReq, const Error& error) {
    if (error == ERR_OK) {
        GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());
        blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, qosReq);
        amDispatcher->dispatchGetObject(qosReq);
    } else {
        getBlobCb(qosReq, error);
    }
}

void
AmProcessor::statBlobCb(AmQosReq *qosReq, const Error& error) {
    DeleteBlobReq *blobReq = static_cast<DeleteBlobReq *>(qosReq->getBlobReqPtr());

    if (ERR_OK == error) {
        // TODO(bszmyd): Tuesday 14 Oct 2014 12:21:41 PM MDT
        // Update the descriptor cache here
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);
    delete blobReq;
}

void
AmProcessor::commitBlobTx(AmQosReq *qosReq) {
    fds_verify(qosReq != NULL);

    // Get the blob request
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Setup callback.
    blobReq->processorCb = AMPROCESSOR_CB_HANDLER(AmProcessor::commitBlobTxCb, qosReq);

    amDispatcher->dispatchCommitBlobTx(qosReq);
}

void
AmProcessor::commitBlobTxCb(AmQosReq *qosReq, const Error &error) {
    fds_verify(qosReq != NULL);
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Push the committed update to the cache and remove from manager
    // TODO(Andrew): Inserting the entire tx transaction currently
    // assumes that the tx descriptor has all of the contents needed
    // for a blob descriptor (e.g., size, version, etc..). Today this
    // is true for S3/Swift and doesn't get used anyways for block (so
    // the actual cached descriptor for block will not be correct).
    AmTxDescriptor::ptr txDesc;
    fds_verify(txMgr->getTxDescriptor(*(blobReq->getTxId()),
                                        txDesc) == ERR_OK);
    fds_verify(amCache->putTxDescriptor(txDesc) == ERR_OK);
    fds_verify(txMgr->removeTx(*(blobReq->getTxId())) == ERR_OK);

    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);

    delete blobReq;
}

void
AmProcessor::volumeContentsCb(AmQosReq *qosReq, const Error& error) {
    VolumeContentsReq *blobReq = static_cast<VolumeContentsReq *>(qosReq->getBlobReqPtr());

    // Tell QoS the request is done
    qosCtrl->markIODone(qosReq);
    blobReq->cb->call(error);
    delete blobReq;
}
}  // namespace fds
