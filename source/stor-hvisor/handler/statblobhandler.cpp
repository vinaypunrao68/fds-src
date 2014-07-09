/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include "./handler.h"
#include "../StorHvisorNet.h"
#include <net/net-service-tmpl.hpp>
namespace fds {

Error StatBlobHandler::handleRequest(const std::string& volumeName,
                                     const std::string& blobName,
                                     CallbackPtr cb) {
    StorHvCtrl::BlobRequestHelper helper(storHvisor, volumeName);
    LOGDEBUG << "volume: " << volumeName;

    helper.setupVolumeInfo();
    helper.blobReq = new StatBlobReq(helper.volId,
                                     volumeName,
                                     blobName,
                                     cb);
    return helper.processRequest();
}

Error StatBlobHandler::handleResponse(AmQosReq *qosReq,
                                      FailoverRpcRequest* rpcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload) {
    Error err(ERR_OK);
    StorHvCtrl::ResponseHelper helper(storHvisor, qosReq);

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return error;
    }

    // using the same structure for input and output
    fpi::GetBlobMetaDataMsgPtr resp =
            net::ep_deserialize<fpi::GetBlobMetaDataMsg>(const_cast<Error&>(error), payload);

    StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, helper.blobReq->cb);
    // Fill in the data here
    cb->blobDesc.setBlobName(helper.blobReq->getBlobName());
    cb->blobDesc.setBlobSize(0);  // TODO(prem) : fill this
    for (const auto& meta : resp->metaDataList) {
        cb->blobDesc.addKvMeta(meta.key,  meta.value);
    }
    return err;
}

Error StatBlobHandler::handleQueueItem(AmQosReq *qosReq) {
    Error err(ERR_OK);

    StorHvCtrl::RequestHelper helper(storHvisor, qosReq);

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }

    GetBlobMetaDataMsgPtr message(new GetBlobMetaDataMsg());

    auto asyncReq = gRpcRequestPool->newFailoverNetRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            storHvisor->om_client->getDMTNodesForVolume(helper.volId)));
    asyncReq->setPayload(fpi::QueryCatalogMsgTypeId, message);

    auto cb = RESPONSE_MSG_HANDLER(StatBlobHandler::handleResponse, qosReq);

    asyncReq->setTimeoutMs(500);
    asyncReq->onResponseCb(cb);
    asyncReq->invoke();
    return err;
}

}  // namespace fds
