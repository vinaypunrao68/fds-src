/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <list>
#include <DataMgr.h>
#include <dmhandler.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

GetBucketHandler::GetBucketHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetBucketMsg, handleRequest);
    }
}

void GetBucketHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::GetBucketMsg>& message) {
    LOGDEBUG << "volume: " << message->volume_id;

    // setup the request
    auto dmRequest = new DmIoGetBucket(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(GetBucketHandler::handleResponse, asyncHdr, dmRequest->response);

    addToQueue(dmRequest);
}

void GetBucketHandler::handleQueueItem(dmCatReq *dmRequest) {
    QueueHelper helper(dmRequest);  // this will call the callback
    DmIoGetBucket *request = static_cast<DmIoGetBucket*>(dmRequest);

    // do processing and set the error
    helper.err = dataMgr->timeVolCat_->queryIface()->listBlobs(dmRequest->volId,
                                                               &request->response->blob_info_list);
    LOGDEBUG << " volid: " << dmRequest->volId
             << " numblobs: " << request->response->blob_info_list.size();
}

void GetBucketHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::GetBucketRspMsg>& message,
                                      const Error &e, dmCatReq *dmRequest) {
    LOGDEBUG << " volid: " << dmRequest->volId
             << " err: " << e;
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(asyncHdr, fpi::GetBucketRspMsgTypeId, message);
    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
