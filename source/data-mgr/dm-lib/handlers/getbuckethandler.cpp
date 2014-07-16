/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <DataMgr.h>
#include <dmhandler.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

GetBucketHandler::GetBucketHandler() {
    REGISTER_DM_MSG_HANDLER(fpi::GetBucketMsg, handleRequest);
}

void GetBucketHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::GetBucketMsg>& message) {
    LOGDEBUG << "volume: " << message->volume_id;

    // setup the request
    auto dmRequest = new DmIoGetBucket(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(GetBucketHandler::handleResponse, asyncHdr, message);

    RequestHelper helper(dmRequest);  // this will queue the request
}

void GetBucketHandler::handleQueueItem(dmCatReq *dmRequest) {
    QueueHelper helper(dmRequest);  // this will call the callback
    DmIoGetBucket *request = static_cast<DmIoGetBucket*>(dmRequest);

    // do processing and set the error
    // helper.err = getBlobMetaDataSvc(request);
}

void GetBucketHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::GetBucketMsg>& message,
                                      const Error &e, dmCatReq *dmRequest) {
}
}  // namespace dm
}  // namespace fds
