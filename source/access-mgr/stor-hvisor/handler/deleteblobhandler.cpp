/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include "./handler.h"
#include "../StorHvisorNet.h"
#include <net/net-service-tmpl.hpp>

#include "requests/DeleteBlobReq.h"

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
    blobReq->tx_desc = blobTxId;
    helper.blobReq = blobReq;
    return helper.processRequest();
}

Error DeleteBlobHandler::handleResponse(AmRequest *amReq,
                                        QuorumSvcRequest* svcReq,
                                        const Error& error,
                                        boost::shared_ptr<std::string> payload) {
    StorHvCtrl::ResponseHelper helper(storHvisor, amReq);
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq*>(helper.blobReq);
    LOGDEBUG << " volume:" << blobReq->io_vol_id
             << " blob:" << blobReq->getBlobName()
             << " txn:" << blobReq->tx_desc;

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return error;
    }
    return error;
}

Error DeleteBlobHandler::handleQueueItem(AmRequest *amReq) {
    Error err(ERR_OK);
    StorHvCtrl::RequestHelper helper(storHvisor, amReq);
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq*>(helper.blobReq);
    LOGDEBUG << " volume:" << helper.blobReq->io_vol_id
             << " blob:" << helper.blobReq->getBlobName()
             << " txn:" << blobReq->tx_desc;

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }
    // check if this is a snapshot
    if (helper.shVol->voldesc->isSnapshot()) {
        LOGWARN << "delete blob on a snapshot is not allowed.";
        helper.setStatus(FDSN_StatusErrorAccessDenied);
        return FDSN_StatusErrorAccessDenied;
    }

    // Update the tx manager with the delete op
    storHvisor->amTxMgr->updateTxOpType(*(blobReq->tx_desc), amReq->io_type);

    DeleteBlobMsgPtr message(new DeleteBlobMsg());
    message->volume_id = blobReq->io_vol_id;
    message->blob_name = blobReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = blobReq->tx_desc->getValue();

    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(
            storHvisor->om_client->getDMTNodesForVolume(
                storHvisor->vol_table->getBaseVolumeId(helper.volId))));

    asyncReq->setPayload(fpi::DeleteBlobMsgTypeId, message);
    auto cb = RESPONSE_MSG_HANDLER(DeleteBlobHandler::handleResponse, amReq);

    asyncReq->onResponseCb(cb);
    LOGDEBUG << "invoke";
    asyncReq->invoke();
    return err;
}

}  // namespace fds
