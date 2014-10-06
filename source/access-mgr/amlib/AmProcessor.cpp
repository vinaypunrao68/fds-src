/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

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
                         AmTxManager::shared_ptr _amTxMgr)
        : Module(modName.c_str()),
          amDispatcher(_amDispatcher),
          qosCtrl(_qosCtrl),
          volTable(_volTable),
          txMgr(_amTxMgr) {
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

}  // namespace fds
