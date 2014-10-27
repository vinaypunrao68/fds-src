/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include "./handler.h"
#include "../StorHvisorNet.h"
#include <net/net-service-tmpl.hpp>
#include "responsehandler.h"

#include "requests/VolumeContentsReq.h"

namespace fds {

Error GetBucketHandler::handleRequest(BucketContext* bucket_context,
                                      fds_uint32_t start,
                                      fds_uint32_t maxkeys,
                                      CallbackPtr cb) {
    StorHvCtrl::BlobRequestHelper helper(storHvisor, bucket_context->bucketName);
    LOGDEBUG <<" volume:" << bucket_context->bucketName;
    helper.blobReq = new VolumeContentsReq(helper.volId, bucket_context, maxkeys, cb);
    return helper.processRequest();
}

Error GetBucketHandler::handleResponse(AmQosReq *qosReq,
                                       FailoverSvcRequest* svcReq,
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
    auto response = MSG_DESERIALIZE(GetBucketMsg, error, payload);

    ListBucketResponseHandler::ptr cb = SHARED_DYN_CAST(ListBucketResponseHandler,
                                                        helper.blobReq->cb);
    size_t count = response->blob_info_list.size();
    LOGDEBUG << " volid: " << response->volume_id
             << " numBlobs: " << response->blob_info_list.size();
    for (size_t i = 0; i < count; ++i) {
        apis::BlobDescriptor bd;
        bd.name = response->blob_info_list[i].blob_name;
        bd.byteCount = response->blob_info_list[i].blob_size;
        cb->vecBlobs.push_back(bd);
    }

    return err;
}

Error GetBucketHandler::handleQueueItem(AmQosReq *qosReq) {
    Error err(ERR_OK);
    StorHvCtrl::RequestHelper helper(storHvisor, qosReq);
    LOGDEBUG << "volume:" << helper.blobReq->vol_id
             <<" blob:" << helper.blobReq->getBlobName();

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }

    GetBucketMsgPtr message(new GetBucketMsg());
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq*>(helper.blobReq);
    message->volume_id = blobReq->vol_id;
    message->startPos  = 0;
    message->maxKeys   = blobReq->maxkeys;

    auto asyncReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            storHvisor->om_client->getDMTNodesForVolume(
                storHvisor->vol_table->getBaseVolumeId(helper.volId))));

    asyncReq->setPayload(fpi::GetBucketMsgTypeId, message);
    auto cb = RESPONSE_MSG_HANDLER(GetBucketHandler::handleResponse, qosReq);

    asyncReq->onResponseCb(cb);
    LOGDEBUG << "invoke";
    asyncReq->invoke();
    return err;
}

}  // namespace fds
