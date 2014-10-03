/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <fds_process.h>
#include <AmProcessor.h>

namespace fds {

AmProcessor::AmProcessor(const std::string &modName,
                         AmTxManager::shared_ptr _amTxMgr)
        : Module(modName.c_str()),
          txMgr(_amTxMgr) {
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
    Error err(ERR_OK);
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);
}

void
AmProcessor::startBlobTxCb(AmQosReq *qosReq,
                           const Error &error) {
}

}  // namespace fds
