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

// TODO(Greg): May be removed when sync interface is removed.
#include <responsehandler.h>

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
    fds_verify(amReq->magicInUse());
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::getVolumeMetadataCb, amReq);
    amDispatcher->dispatchGetVolumeMetadata(amReq);
}

void
AmProcessor::getVolumeMetadataCb(AmRequest *amReq, const Error &error) {
    GetVolumeMetaDataCallback::ptr cb =
            SHARED_DYN_CAST(GetVolumeMetaDataCallback, amReq->cb);
    cb->volumeMetaData = static_cast<GetVolumeMetaDataReq *>(amReq)->volumeMetadata;
    qosCtrl->markIODone(amReq);
    cb->call(error);
}

void
AmProcessor::abortBlobTx(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::abortBlobTxCb, amReq);
    amDispatcher->dispatchAbortBlobTx(amReq);
}

void
AmProcessor::startBlobTx(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());

    fiu_do_on("am.uturn.processor.startBlobTx",
              qosCtrl->markIODone(amReq); \
              amReq->cb->call(ERR_OK); \
              delete amReq; \
              return;);

    // check if this is a snapshot
    // TODO(Andrew): Why not just let DM reject the IO?
    StorHvVolume *shVol = volTable->getLockedVolume(amReq->io_vol_id);
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        qosCtrl->markIODone(amReq);
        amReq->cb->call(FDSN_StatusErrorAccessDenied);
        delete amReq;
        return;
    }
    shVol->readUnlock();

    // Generate a random transaction ID to use
    static_cast<StartBlobTxReq*>(amReq)->tx_desc =
        boost::make_shared<BlobTxId>(randNumGen->genNumSafe());
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::startBlobTxCb, amReq);

    amDispatcher->dispatchStartBlobTx(amReq);
}

void
AmProcessor::startBlobTxCb(AmRequest *amReq, const Error &error) {
    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  amReq->cb);

    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(amReq);
    if (error.ok()) {
        // Update callback and record new open transaction
        cb->blobTxId  = *blobReq->tx_desc;
        fds_verify(txMgr->addTx(amReq->io_vol_id,
                                *blobReq->tx_desc,
                                blobReq->dmt_version,
                                amReq->getBlobName()) == ERR_OK);
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    cb->call(error.GetErrno());
}

void
AmProcessor::deleteBlob(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());
    fds_volid_t volId = amReq->io_vol_id;
    StorHvVolume* shVol = volTable->getLockedVolume(volId);

    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(amReq);
    LOGDEBUG    << " volume:" << volId
                << " blob:" << amReq->getBlobName()
                << " txn:" << blobReq->tx_desc;

    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    if ((shVol == NULL) || (!shVol->isValidLocked())) {
        LOGCRITICAL << "unable to get volume info for vol: " << volId;
        genericCb(amReq, FDSN_StatusErrorUnknown);
        shVol->readUnlock();
        return;
    }

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "delete blob on a snapshot is not allowed.";
        genericCb(amReq, FDSN_StatusErrorAccessDenied);
        shVol->readUnlock();
        return;
    }
    shVol->readUnlock();

    // Update the tx manager with the delete op
    txMgr->updateTxOpType(*(blobReq->tx_desc), amReq->io_type);

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::genericCb, amReq);
    amDispatcher->dispatchDeleteBlob(amReq);
}

void
AmProcessor::putBlob(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());

    // check if this is a snapshot
    // TODO(Andrew): Why not just let DM reject the IO?
    StorHvVolume *shVol = volTable->getLockedVolume(amReq->io_vol_id);
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                      amReq->cb);
        qosCtrl->markIODone(amReq);
        cb->call(FDSN_StatusErrorAccessDenied);
        delete amReq;
        return;
    }
    shVol->readUnlock();

    // TODO(Andrew): Here we're turning the offset aligned
    // blobOffset back into an absolute blob offset (i.e.,
    // not aligned to the maximum object size). This allows
    // the rest of the putBlob routines to still expect an
    // absolute offset in case we need it
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    amReq->blob_offset = (amReq->blob_offset * maxObjSize);

    // Use a stock object ID if the length is 0.
    if (amReq->data_len == 0) {
        LOGWARN << "zero size object - "
                << " [objkey:" << amReq->getBlobName() <<"]";
        amReq->obj_id = ObjectID();
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(amReq->hash_perf_ctx);
        amReq->obj_id = ObjIdGen::genObjectId(amReq->getDataBuf(), amReq->data_len);
    }

    fiu_do_on("am.uturn.processor.putBlob",
              qosCtrl->markIODone(amReq);       \
              amReq->cb->call(ERR_OK);        \
              delete amReq;                   \
              return;);

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::putBlobCb, amReq);

    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
        // Sending the update in a single request. Create transaction ID to
        // use for the single request
        blobReq->setTxId(BlobTxId(randNumGen->genNumSafe()));
        amDispatcher->dispatchUpdateCatalogOnce(amReq);
    } else {
        amDispatcher->dispatchUpdateCatalog(amReq);
    }

    // Either dispatch the put blob request or, if there's no data, just call
    // our callback handler now (NO-OP).
    amReq->data_len > 0 ? amDispatcher->dispatchPutObject(amReq) :
                          blobReq->notifyResponse(ERR_OK);
}

void
AmProcessor::putBlobCb(AmRequest *amReq, const Error& error) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);

    if (error.ok()) {
        // Add the Tx to the manager is this an updateOnce
        if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
            fds_verify(txMgr->addTx(amReq->io_vol_id,
                                    *(blobReq->tx_desc),
                                    blobReq->dmt_version,
                                    amReq->getBlobName()) == ERR_OK);
            // Stage the transaction metadata changes
            fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->tx_desc),
                                                   blobReq->metadata) == ERR_OK);
        }
        // Update the tx manager with this update
        fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->tx_desc),
                                               amReq->data_len) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(txMgr->updateStagedBlobOffset(*(blobReq->tx_desc),
                                                 amReq->getBlobName(),
                                                 amReq->blob_offset,
                                                 amReq->obj_id) == ERR_OK);
        // Update the transaction manager with the stage object data
        if (amReq->data_len > 0) {
            fds_verify(txMgr->updateStagedBlobObject(*(blobReq->tx_desc),
                                                     amReq->obj_id,
                                                     amReq->getDataBuf(),
                                                     amReq->data_len)
                   == ERR_OK);
        }

        if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
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
    amReq->cb->call(error);
    delete amReq;
}

void
AmProcessor::getBlob(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());
    fiu_do_on("am.uturn.processor.getBlob",
              qosCtrl->markIODone(amReq); \
              amReq->cb->call(ERR_OK); \
              delete amReq; \
              return;);

    fds_volid_t volId = amReq->io_vol_id;
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
    amReq->blob_offset = (amReq->blob_offset * maxObjSize);

    // Check cache for object ID
    Error err = ERR_OK;
    ObjectID::ptr objectId = amCache->getBlobOffsetObject(volId,
                                                          amReq->getBlobName(),
                                                          amReq->blob_offset,
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

            // Pull out the GET callback object so we can populate it
            // with cache contents and send it to the requester.
            GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);

            cb->returnSize = std::min(amReq->data_len, objectData->size());

            // Make sure we have a buffer.
            // TODO(Andrew): This should be a shared pointer
            // as we pass it around a lot.
            if (cb->returnBuffer == nullptr) {
                cb->returnBuffer = new char[cb->returnSize];
            }

            // Only return UP-TO the amount of data requested, never more
            memcpy(cb->returnBuffer, objectData->c_str(), cb->returnSize);

            // Report results of GET request to requestor.
            getBlobCb(amReq, err);
        } else {
            // We couldn't find the data in the cache even though the id was
            // obtained there. Fallback to retrieving the data from the SM.
            amReq->obj_id = *objectId;
            amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, amReq);
            amDispatcher->dispatchGetObject(amReq);
        }
    } else {
        amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::queryCatalogCb, amReq);
        amDispatcher->dispatchQueryCatalog(amReq);
    }
}

void
AmProcessor::setBlobMetadata(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(amReq);

    // Stage the transaction metadata changes
    fds_verify(txMgr->updateStagedBlobDesc(*(blobReq->tx_desc), blobReq->getMetaDataListPtr()))

    fds_verify(txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version)));
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::genericCb, amReq);

    amDispatcher->dispatchSetBlobMetadata(amReq);
}

void
AmProcessor::statBlob(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());

    fds_volid_t volId = amReq->io_vol_id;
    LOGDEBUG << "volume:" << volId <<" blob:" << amReq->getBlobName();

    // Check cache for blob descriptor
    Error err(ERR_OK);
    BlobDescriptor::ptr cachedBlobDesc = amCache->getBlobDescriptor(volId,
                                                                    amReq->getBlobName(),
                                                                    err);
    if (ERR_OK == err) {
        LOGTRACE << "Found cached blob descriptor for " << std::hex
            << volId << std::dec << " blob " << amReq->getBlobName();

        StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, amReq->cb);
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
        << volId << std::dec << " blob " << amReq->getBlobName();

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::statBlobCb, amReq);
    amDispatcher->dispatchStatBlob(amReq);
}

void
AmProcessor::abortBlobTxCb(AmRequest *amReq, const Error &error) {
    qosCtrl->markIODone(amReq);
    amReq->cb->call(error);

    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(amReq);
    fds_verify(ERR_OK == txMgr->removeTx(*(blobReq->tx_desc)));

    delete amReq;
}

void
AmProcessor::volumeContents(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::genericCb, amReq);
    amDispatcher->dispatchVolumeContents(amReq);
}

void
AmProcessor::getBlobCb(AmRequest *amReq, const Error& error) {
    if (ERR_OK == error) {
        // TODO(bszmyd): Thu 09 Oct 2014 04:30:52 PM MDT
        // We have successfully retrieved the BLOB, let's stick it in the cache
        // if it didn't already exist there (call is idempotent).
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    amReq->cb->call(error);

    // TODO(Greg): This check may be removed when sync interface is removed.
    boost::shared_ptr<ResponseHandler> ah =
            SHARED_DYN_CAST(ResponseHandler, amReq->cb);
    if (ah->isAsyncHandler()) {
        // We're finished with our buffer used to return the object.
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
        delete[] cb->returnBuffer;
    }

    delete amReq;
}

void
AmProcessor::queryCatalogCb(AmRequest *amReq, const Error& error) {
    if (error == ERR_OK) {
        amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::getBlobCb, amReq);
        amDispatcher->dispatchGetObject(amReq);
    } else {
        getBlobCb(amReq, error);
    }
}

void
AmProcessor::statBlobCb(AmRequest *amReq, const Error& error) {
    if (ERR_OK == error) {
        // TODO(bszmyd): Tuesday 14 Oct 2014 12:21:41 PM MDT
        // Update the descriptor cache here
    }

    // Tell QoS the request is done
    qosCtrl->markIODone(amReq);
    amReq->cb->call(error);
    delete amReq;
}

void
AmProcessor::commitBlobTx(AmRequest *amReq) {
    fds_verify(amReq->magicInUse());
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor::commitBlobTxCb, amReq);
    amDispatcher->dispatchCommitBlobTx(amReq);
}

void
AmProcessor::commitBlobTxCb(AmRequest *amReq, const Error &error) {
    // Push the committed update to the cache and remove from manager
    // TODO(Andrew): Inserting the entire tx transaction currently
    // assumes that the tx descriptor has all of the contents needed
    // for a blob descriptor (e.g., size, version, etc..). Today this
    // is true for S3/Swift and doesn't get used anyways for block (so
    // the actual cached descriptor for block will not be correct).
    AmTxDescriptor::ptr txDesc;
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(amReq);
    fds_verify(txMgr->getTxDescriptor(*(blobReq->tx_desc), txDesc) == ERR_OK);
    fds_verify(amCache->putTxDescriptor(txDesc) == ERR_OK);
    fds_verify(txMgr->removeTx(*(blobReq->tx_desc)) == ERR_OK);

    qosCtrl->markIODone(amReq);
    amReq->cb->call(error);

    delete amReq;
}

void
AmProcessor::genericCb(AmRequest *amReq, const Error& error) {
    qosCtrl->markIODone(amReq);
    amReq->cb->call(error);
    delete amReq;
}
}  // namespace fds
