/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <string>
#include <ObjectId.h>
#include <fds_process.h>
#include <AmProcessor.h>
#include <fiu-control.h>
#include <util/fiu_util.h>

#include "requests/requests.h"

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
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    if (conf.get<fds_bool_t>("testing.uturn_processor_all")) {
        fiu_enable("am.uturn.processor.*", 1, NULL, 0);
    }
    randNumGen = RandNumGenerator::unique_ptr(
        new RandNumGenerator(RandNumGenerator::getRandSeed()));
}

void
AmProcessor::getVolumeMetadata(AmRequest *amReq) {
    GetVolumeMetaDataReq* volReq = static_cast<GetVolumeMetaDataReq *>(amReq);
    fds_verify(true == volReq->magicInUse());

    // Set the processor callback
    volReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::getVolumeMetadataCb, amReq);
    amDispatcher->dispatchGetVolumeMetadata(amReq);
}

void
AmProcessor::getVolumeMetadataCb(AmRequest *amReq, const Error &error) {
    GetVolumeMetaDataReq* volReq = static_cast<GetVolumeMetaDataReq *>(amReq);
    fds_verify(true == volReq->magicInUse());

    GetVolumeMetaDataCallback::ptr cb =
            SHARED_DYN_CAST(GetVolumeMetaDataCallback, volReq->cb);
    cb->volumeMetaData = volReq->volumeMetadata;
    qosCtrl->markIODone(amReq);
    cb->call(error);
}

void
AmProcessor::abortBlobTx(AmRequest *amReq) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(amReq);

    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::abortBlobTxCb, amReq);

    amDispatcher->dispatchAbortBlobTx(amReq);
}

void
AmProcessor::startBlobTx(AmRequest *amReq) {
    // Get the blob request
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    fiu_do_on("am.uturn.processor.startBlobTx",
              qosCtrl->markIODone(amReq); \
              blobReq->cb->call(ERR_OK); \
              delete blobReq; \
              return;);

    // check if this is a snapshot
    // TODO(Andrew): Why not just let DM reject the IO?
    StorHvVolume *shVol = volTable->getLockedVolume(blobReq->io_vol_id);
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                      blobReq->cb);
        qosCtrl->markIODone(amReq);
        cb->call(FDSN_StatusErrorAccessDenied);
        delete blobReq;
    }
    shVol->readUnlock();

    // Generate a random transaction ID to use
    blobReq->tx_desc = boost::make_shared<BlobTxId>(randNumGen->genNumSafe());
    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::startBlobTxCb, amReq);

    amDispatcher->dispatchStartBlobTx(amReq);
}

void
AmProcessor::startBlobTxCb(AmRequest *amReq, const Error &error) {
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  blobReq->cb);

    if (error.ok()) {
        // Update callback and record new open transaction
        cb->blobTxId  = *blobReq->tx_desc;
        fds_verify(txMgr->addTx(blobReq->io_vol_id,
                                *blobReq->tx_desc,
                                blobReq->dmtVersion,
                                blobReq->getBlobName()) == ERR_OK);
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    cb->call(error.GetErrno());
}

void
AmProcessor::deleteBlob(AmRequest *amReq) {
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(amReq);
    fds_volid_t volId = blobReq->io_vol_id;
    StorHvVolume* shVol = volTable->getLockedVolume(volId);

    LOGDEBUG    << " volume:" << volId
                << " blob:" << blobReq->getBlobName()
                << " txn:" << blobReq->tx_desc;

    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    if ((shVol == NULL) || (!shVol->isValidLocked())) {
        LOGCRITICAL << "unable to get volume info for vol: " << volId;
        deleteBlobCb(amReq, FDSN_StatusErrorUnknown);
        shVol->readUnlock();
        return;
    }

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "delete blob on a snapshot is not allowed.";
        deleteBlobCb(amReq, FDSN_StatusErrorAccessDenied);
        shVol->readUnlock();
        return;
    }
    shVol->readUnlock();

    // Update the tx manager with the delete op
    txMgr->updateTxOpType(*(blobReq->tx_desc), blobReq->getIoType());

    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::deleteBlobCb, amReq);
    amDispatcher->dispatchDeleteBlob(amReq);
}

void
AmProcessor::putBlob(AmRequest *amReq) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);

    // check if this is a snapshot
    // TODO(Andrew): Why not just let DM reject the IO?
    StorHvVolume *shVol = volTable->getLockedVolume(blobReq->io_vol_id);
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                      blobReq->cb);
        qosCtrl->markIODone(amReq);
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
    blobReq->blob_offset = (blobReq->blob_offset * maxObjSize);

    // Use a stock object ID if the length is 0.
    if (blobReq->data_len == 0) {
        LOGWARN << "zero size object - "
                << " [objkey:" << blobReq->ObjKey <<"]";
        blobReq->obj_id = ObjectID();
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(blobReq->hash_perf_ctx);
        blobReq->obj_id = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                                blobReq->data_len);
    }

    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::putBlobCb, amReq);

    if (blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
        // Sending the update in a single request. Create transaction ID to
        // use for the single request
        blobReq->setTxId(BlobTxId(randNumGen->genNumSafe()));
        amDispatcher->dispatchUpdateCatalogOnce(amReq);
    } else {
        amDispatcher->dispatchUpdateCatalog(amReq);
    }

    // Either dispatch the put blob request or, if there's no data, just call
    // our callback handler now (NO-OP).
    blobReq->data_len > 0 ? amDispatcher->dispatchPutObject(amReq) :
                                blobReq->notifyResponse(amReq, ERR_OK);
}

void
AmProcessor::putBlobCb(AmRequest *amReq, const Error& error) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);

    if (error.ok()) {
        // Add the Tx to the manager is this an updateOnce
        if (blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
            fds_verify(txMgr->addTx(blobReq->io_vol_id,
                                    *(blobReq->tx_desc),
                                    blobReq->dmtVersion,
                                    blobReq->getBlobName()) == ERR_OK);
            // Stage the transaction metadata changes
            fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->tx_desc),
                                                   blobReq->metadata) == ERR_OK);
        }
        // Update the tx manager with this update
        fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->tx_desc),
                                               blobReq->data_len) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(txMgr->updateStagedBlobOffset(*(blobReq->tx_desc),
                                                 blobReq->getBlobName(),
                                                 blobReq->blob_offset,
                                                 blobReq->obj_id) == ERR_OK);
        // Update the transaction manager with the stage object data
        if (blobReq->data_len > 0) {
            fds_verify(txMgr->updateStagedBlobObject(*(blobReq->tx_desc),
                                                     blobReq->obj_id,
                                                     blobReq->getDataBuf(),
                                                     blobReq->data_len)
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
            fds_verify(txMgr->getTxDescriptor(*(blobReq->tx_desc),
                                              txDescriptor) == ERR_OK);
            fds_verify(amCache->putTxDescriptor(txDescriptor) == ERR_OK);
            fds_verify(txMgr->removeTx(*(blobReq->tx_desc)) == ERR_OK);
        }
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);
    delete blobReq;
}

void
AmProcessor::getBlob(AmRequest *amReq) {
    // Pull out the Get request
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);

    fiu_do_on("am.uturn.processor.getBlob",
              qosCtrl->markIODone(amReq); \
              blobReq->cb->call(ERR_OK); \
              delete blobReq; \
              return;);

    fds_volid_t volId = blobReq->io_vol_id;
    StorHvVolume *shVol = volTable->getVolume(volId);
    // TODO(bszmyd): Friday, October 10th 2014
    // This logic was copied directly from StorHvCtrl, but it's not
    // quite clear how using the non-locking call above would still
    // allow the following to function. Investigation needed.
    if ((shVol == NULL) || (!shVol->isValidLocked())) {
        LOGCRITICAL << "getBlob failed to get volume for vol " << volId;
        getBlobCb(amReq, ERR_INVALID);
        return;
    }

    // TODO(Anna) We are doing update catalog using absolute
    // offsets, so we need to be consistent in query catalog
    // Review this in the next sprint!
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    blobReq->blob_offset = (blobReq->blob_offset * maxObjSize);

    // Check cache for object ID
    Error err = ERR_OK;
    ObjectID::ptr objectId = amCache->getBlobOffsetObject(volId,
                                                          blobReq->getBlobName(),
                                                          blobReq->blob_offset,
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
            cb->returnSize = std::min(blobReq->data_len, objectData->size());
            memcpy(cb->returnBuffer, objectData->c_str(), cb->returnSize);

            getBlobCb(amReq, err);
        } else {
            // We couldn't find the data in the cache even though the id was
            // obtained there. Fallback to retrieving the data from the SM.
            blobReq->obj_id = *objectId;
            blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, amReq);
            amDispatcher->dispatchGetObject(amReq);
        }
    } else {
        blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::queryCatalogCb, amReq);
        amDispatcher->dispatchQueryCatalog(amReq);
    }
}

void
AmProcessor::setBlobMetadata(AmRequest *amReq) {
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(amReq);

    // Stage the transaction metadata changes
    fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->tx_desc), blobReq->getMetaDataListPtr()))

    fds_verify(txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version)));
    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::setBlobMetadataCb, amReq);

    amDispatcher->dispatchSetBlobMetadata(amReq);
}

void
AmProcessor::setBlobMetadataCb(AmRequest *amReq, const Error &error) {
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(amReq);

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);

    delete blobReq;
}

void
AmProcessor::statBlob(AmRequest *amReq) {
    Error err(ERR_OK);
    StatBlobReq* blobReq = static_cast<StatBlobReq *>(amReq);
    fds_volid_t volId = blobReq->io_vol_id;

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
        statBlobCb(amReq, ERR_OK);
        return;
    }
    LOGTRACE << "Did not find cached blob descriptor for " << std::hex
        << volId << std::dec << " blob " << blobReq->getBlobName();

    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::statBlobCb, amReq);
    amDispatcher->dispatchStatBlob(amReq);
}

void
AmProcessor::abortBlobTxCb(AmRequest *amReq, const Error &error) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(amReq);

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);

    fds_verify(ERR_OK == txMgr->removeTx(*(blobReq->tx_desc)));

    delete blobReq;
}

void
AmProcessor::volumeContents(AmRequest *amReq) {
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq*>(amReq);
    LOGDEBUG << "volume:" << blobReq->io_vol_id
        <<" blob:" << blobReq->getBlobName();

    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::volumeContentsCb, amReq);

    amDispatcher->dispatchVolumeContents(amReq);
}

void
AmProcessor::deleteBlobCb(AmRequest *amReq, const Error& error) {
    DeleteBlobReq *blobReq = static_cast<DeleteBlobReq *>(amReq);

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);
    delete blobReq;
}


void
AmProcessor::getBlobCb(AmRequest *amReq, const Error& error) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);

    if (ERR_OK == error) {
        // TODO(bszmyd): Thu 09 Oct 2014 04:30:52 PM MDT
        // We have successfully retrieved the BLOB, let's stick it in the cache
        // if it didn't already exist there (call is idempotent).
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);
    delete blobReq;
}

void
AmProcessor::queryCatalogCb(AmRequest *amReq, const Error& error) {
    if (error == ERR_OK) {
        GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
        blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, amReq);
        amDispatcher->dispatchGetObject(amReq);
    } else {
        getBlobCb(amReq, error);
    }
}

void
AmProcessor::statBlobCb(AmRequest *amReq, const Error& error) {
    DeleteBlobReq *blobReq = static_cast<DeleteBlobReq *>(amReq);

    if (ERR_OK == error) {
        // TODO(bszmyd): Tuesday 14 Oct 2014 12:21:41 PM MDT
        // Update the descriptor cache here
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);
    delete blobReq;
}

void
AmProcessor::commitBlobTx(AmRequest *amReq) {
    fds_verify(amReq != NULL);

    // Get the blob request
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(amReq);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Setup callback.
    blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::commitBlobTxCb, amReq);

    amDispatcher->dispatchCommitBlobTx(amReq);
}

void
AmProcessor::commitBlobTxCb(AmRequest *amReq, const Error &error) {
    fds_verify(amReq != NULL);
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(amReq);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Push the committed update to the cache and remove from manager
    // TODO(Andrew): Inserting the entire tx transaction currently
    // assumes that the tx descriptor has all of the contents needed
    // for a blob descriptor (e.g., size, version, etc..). Today this
    // is true for S3/Swift and doesn't get used anyways for block (so
    // the actual cached descriptor for block will not be correct).
    AmTxDescriptor::ptr txDesc;
    fds_verify(txMgr->getTxDescriptor(*(blobReq->tx_desc),
                                        txDesc) == ERR_OK);
    fds_verify(amCache->putTxDescriptor(txDesc) == ERR_OK);
    fds_verify(txMgr->removeTx(*(blobReq->tx_desc)) == ERR_OK);

    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);

    delete blobReq;
}

void
AmProcessor::volumeContentsCb(AmRequest *amReq, const Error& error) {
    VolumeContentsReq *blobReq = static_cast<VolumeContentsReq *>(amReq);

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    blobReq->cb->call(error);
    delete blobReq;
}
}  // namespace fds
