/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include "./handler.h"
#include "../StorHvisorNet.h"
#include <net/net-service-tmpl.hpp>

#include "requests/StatBlobReq.h"

namespace fds {

Error StatBlobHandler::handleRequest(const std::string& volumeName,
                                     const std::string& blobName,
                                     CallbackPtr cb) {
    StorHvCtrl::BlobRequestHelper helper(storHvisor, volumeName);
    LOGDEBUG << " volume:" << volumeName
             <<" blob:" << blobName << " helper.volId: " << helper.volId;

    helper.blobReq = new StatBlobReq(helper.volId,
                                     volumeName,
                                     blobName,
                                     cb);
    return helper.processRequest();
}

Error StatBlobHandler::handleResponse(AmQosReq *qosReq,
                                      FailoverSvcRequest* svcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload) {
    Error err(ERR_OK);
    StorHvCtrl::ResponseHelper helper(storHvisor, qosReq);

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
        helper.setStatus(error.GetErrno());
        return error;
    }

    // TODO(Andrew): Update the AM's blob descriptor cache here

    // using the same structure for input and output
    auto response = MSG_DESERIALIZE(GetBlobMetaDataMsg, error, payload);

    StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, helper.blobReq->cb);
    // Fill in the data here
    cb->blobDesc.setBlobName(helper.blobReq->getBlobName());
    cb->blobDesc.setBlobSize(response->byteCount);
    for (const auto& meta : response->metaDataList) {
        cb->blobDesc.addKvMeta(meta.key,  meta.value);
    }
    return err;
}

Error StatBlobHandler::handleQueueItem(AmQosReq *qosReq) {
    Error err(ERR_OK);
    StorHvCtrl::RequestHelper helper(storHvisor, qosReq);
    LOGDEBUG << "volume:" << helper.blobReq->vol_id
             <<" blob:" << helper.blobReq->getBlobName();

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }

    // Check cache for blob descriptor
    BlobDescriptor::ptr cachedBlobDesc =
            helper.storHvisor->amCache->getBlobDescriptor(
                helper.blobReq->vol_id,
                helper.blobReq->getBlobName(),
                err);
    if (err == ERR_OK) {
        LOGTRACE << "Found cached blob descriptor for " << std::hex
                 << helper.blobReq->vol_id << std::dec << " blob "
                 << helper.blobReq->getBlobName();

        StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, helper.blobReq->cb);
        // Fill in the data here
        cb->blobDesc.setBlobName(cachedBlobDesc->getBlobName());
        cb->blobDesc.setBlobSize(cachedBlobDesc->getBlobSize());
        for (const_kv_iterator meta = cachedBlobDesc->kvMetaBegin();
             meta != cachedBlobDesc->kvMetaEnd();
             meta++) {
            cb->blobDesc.addKvMeta(meta->first,  meta->second);
        }
        cb->call(err);
        // Delete the blob request since a responsehelper doesn't
        // do it for you
        delete helper.blobReq;
        // TODO(Andrew): This is what's being returned to the request
        // dispatcher, which always expects OK. The callback was given
        // the correct error code, this doesn't really matter.
        return ERR_OK;
    }
    LOGTRACE << "Did not find cached blob descriptor for " << std::hex
             << helper.blobReq->vol_id << std::dec << " blob "
             << helper.blobReq->getBlobName();

    GetBlobMetaDataMsgPtr message(new GetBlobMetaDataMsg());
    message->volume_id = helper.blobReq->vol_id;
    message->blob_name = helper.blobReq->getBlobName();

    auto asyncReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            storHvisor->om_client->getDMTNodesForVolume(
                storHvisor->vol_table->getBaseVolumeId(helper.volId))));

    asyncReq->setPayload(fpi::GetBlobMetaDataMsgTypeId, message);

    auto cb = RESPONSE_MSG_HANDLER(StatBlobHandler::handleResponse, qosReq);

    asyncReq->onResponseCb(cb);
    LOGDEBUG << "invoke";
    asyncReq->invoke();
    return ERR_OK;
}

}  // namespace fds
