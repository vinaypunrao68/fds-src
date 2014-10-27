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

DeleteBlobHandler::DeleteBlobHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::DeleteBlobMsg, handleRequest);
    }
}

void DeleteBlobHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::DeleteBlobMsg>& message) {
    LOGDEBUG << "volume: " << message->volume_id;

    // setup the request
    auto dmRequest = new DmIoDeleteBlob(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(DeleteBlobHandler::handleResponse, asyncHdr, message);

    addToQueue(dmRequest);
}

void DeleteBlobHandler::handleQueueItem(dmCatReq *dmRequest) {
    QueueHelper helper(dmRequest);  // this will call the callback
    DmIoDeleteBlob *request = static_cast<DmIoDeleteBlob*>(dmRequest);

    // do processing and set the error

    BlobTxId::const_ptr ioBlobTxDesc = boost::make_shared<const BlobTxId>(request->message->txId);
    helper.err = dataMgr->timeVolCat_->deleteBlob(request->volId, ioBlobTxDesc,
                                                  request->blob_version);

    LOGDEBUG << " volid:" << request->volId
             << " blob:" << request->message->blob_name
             << " blob_version " << request->blob_version;
}

void DeleteBlobHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::DeleteBlobMsg>& message,
                                      const Error &e, dmCatReq *dmRequest) {
    LOGDEBUG << " volid:" << dmRequest->volId
             << " blob:" << dmRequest->blob_name
             << " version: " << dmRequest->blob_version
             << " err:" << e;
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::EmptyMsgTypeId, fpi::EmptyMsg());
    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
