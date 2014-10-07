/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <fds_process.h>
#include <AmDispatcher.h>
#include <net/SvcRequestPool.h>

namespace fds {

AmDispatcher::AmDispatcher(const std::string &modName,
                           DMTManagerPtr _dmtMgr)
        : Module(modName.c_str()),
          dmtMgr(_dmtMgr) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    uturnAll = conf.get<fds_bool_t>("testing.uturn_dispatcher_all");
}

AmDispatcher::~AmDispatcher() {
}

int
AmDispatcher::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
AmDispatcher::mod_startup() {
}

void
AmDispatcher::mod_shutdown() {
}

void
AmDispatcher::dispatchStartBlobTx(AmQosReq *qosReq) {
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(
        qosReq->getBlobReqPtr());

    // Update DMT version in request
    // TODO(Andrew): There's a potential race here between the
    // version we cache in the blobReq now and the version we
    // actually dispatch on below. Make the update/dispatch consistent.
    blobReq->dmtVersion = dmtMgr->getCommittedVersion();

    // Create callback
    QuorumSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::startBlobTxCb,
                             qosReq));

    // Create network message
    StartBlobTxMsgPtr startBlobTxMsg(new StartBlobTxMsg());
    startBlobTxMsg->blob_name    = blobReq->getBlobName();
    startBlobTxMsg->blob_version = blob_version_invalid;
    startBlobTxMsg->volume_id    = blobReq->getVolId();
    startBlobTxMsg->blob_mode    = blobReq->getBlobMode();
    startBlobTxMsg->txId         = blobReq->txId.getValue();
    startBlobTxMsg->dmt_version  = blobReq->dmtVersion;

    auto asyncStartBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(
        dmtMgr->getCommittedNodeGroup(blobReq->getVolId())));
    asyncStartBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg),
                                    startBlobTxMsg);
    asyncStartBlobTxReq->onResponseCb(respCb);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString()
             << fds::logString(*startBlobTxMsg);
}

void
AmDispatcher::startBlobTxCb(AmQosReq *qosReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(qosReq != NULL);
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    blobReq->processorCb(error);
    delete blobReq;
}

}  // namespace fds
