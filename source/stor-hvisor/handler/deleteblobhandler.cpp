/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include "./handler.h"
#include "../StorHvisorNet.h"
#include <net/net-service-tmpl.hpp>
namespace fds {

Error DeleteBlobHandler::handleRequest(const std::string& volumeName,
                                       const std::string& blobName,
                                       BlobTxId::ptr blobTxId,
                                       CallbackPtr cb) {
    StorHvCtrl::BlobRequestHelper helper(storHvisor, volumeName);
    LOGDEBUG <<" volume:" << volumeName
             <<" blob:" << blobName;
    DeleteBlobReq* blobReq = new DeleteBlobReq(helper.volId,
                                               blobName,
                                               volumeName,
                                               cb);
    blobReq->setBlobTxId(blobTxId);
    helper.blobReq = blobReq;
    return helper.processRequest();
}

Error DeleteBlobHandler::handleResponse(AmQosReq *qosReq,
                                        QuorumSvcRequest* svcReq,
                                        const Error& error,
                                        boost::shared_ptr<std::string> payload) {
    StorHvCtrl::ResponseHelper helper(storHvisor, qosReq);
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq*>(helper.blobReq);
    LOGDEBUG << " volume:" << blobReq->getVolId()
             << " blob:" << blobReq->getBlobName()
             << " txn:" << blobReq->txDesc;

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return error;
    }
    return error;
}

Error DeleteBlobHandler::handleQueueItem(AmQosReq *qosReq) {
    Error err(ERR_OK);
    StorHvCtrl::RequestHelper helper(storHvisor, qosReq);
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq*>(helper.blobReq);
    LOGDEBUG << " volume:" << helper.blobReq->getVolId()
             << " blob:" << helper.blobReq->getBlobName()
             << " txn:" << blobReq->txDesc;

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }

    // Update the tx manager with the delete op
    storHvisor->amTxMgr->updateTxOpType(*(blobReq->txDesc), blobReq->getIoType());

    DeleteBlobMsgPtr message(new DeleteBlobMsg());
    message->volume_id = blobReq->getVolId();
    message->blob_name = blobReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = blobReq->txDesc->getValue();

    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(
            storHvisor->om_client->getDMTNodesForVolume(
                storHvisor->vol_table->getBaseVolumeId(helper.volId))));

    asyncReq->setPayload(fpi::DeleteBlobMsgTypeId, message);
    auto cb = RESPONSE_MSG_HANDLER(DeleteBlobHandler::handleResponse, qosReq);

    asyncReq->onResponseCb(cb);
    LOGDEBUG << "invoke";
    asyncReq->invoke();
    return err;
}

}  // namespace fds
