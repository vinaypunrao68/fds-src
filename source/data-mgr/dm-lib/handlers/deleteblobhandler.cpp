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

DeleteBlobHandler::DeleteBlobHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::DeleteBlobMsg, handleRequest);
    }
}

void DeleteBlobHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::DeleteBlobMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID();

    // Handle U-turn
    HANDLE_U_TURN();

    fds_volid_t volId(message->volume_id);
    auto err = preEnqueueWriteOpHandling(volId, message->opId,
                                         asyncHdr, PlatNetSvcHandler::threadLocalPayloadBuf);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    HANDLE_FILTER_OLD_DMT_DURING_RESYNC();

    // setup the request
    auto dmRequest = new DmIoDeleteBlob(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(DeleteBlobHandler::handleResponse, asyncHdr, message);

    addToQueue(dmRequest);
}

void DeleteBlobHandler::handleQueueItem(DmRequest *dmRequest) {
    QueueHelper helper(dataManager, dmRequest);  // this will call the callback
    DmIoDeleteBlob *request = static_cast<DmIoDeleteBlob*>(dmRequest);

    ENSURE_IO_ORDER(request, helper);

    LOGDEBUG << " volid:" << request->volId
             << " blob:" << request->message->blob_name
             << " blob_version " << request->blob_version;

    // do processing and set the error
    BlobTxId::const_ptr ioBlobTxDesc = boost::make_shared<const BlobTxId>(request->message->txId);
    helper.err = (blobTxIdInvalid == *ioBlobTxDesc) ? ERR_DM_INVALID_TX_ID :
            dataManager.timeVolCat_->deleteBlob(request->volId,
                                                ioBlobTxDesc,
                                                request->blob_version);
}

void DeleteBlobHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::DeleteBlobMsg>& message,
                                      const Error &e, DmRequest *dmRequest) {
    if (dmRequest) {
        LOGDEBUG << " volid:" << dmRequest->volId
                 << " blob:" << dmRequest->blob_name
                 << " version: " << dmRequest->blob_version
                 << " err:" << e;
    }

    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::EmptyMsgTypeId, fpi::EmptyMsg());
    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
