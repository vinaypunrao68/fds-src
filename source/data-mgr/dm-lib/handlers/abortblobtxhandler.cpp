/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>
#include <blob/BlobTypes.h>
#include <fds_assert.h>

namespace fds {
namespace dm {

AbortBlobTxHandler::AbortBlobTxHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::AbortBlobTxMsg, handleRequest);
    }
}

void AbortBlobTxHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::AbortBlobTxMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID();

    // Handle U-turn
    HANDLE_U_TURN();

    fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    // Allocate a new Blob transaction class and queue to per volume queue.
    auto dmReq = new DmIoAbortBlobTx(volId,
                                     message->blob_name,
                                     message->blob_version);
    dmReq->cb = BIND_MSG_CALLBACK(AbortBlobTxHandler::handleResponse, asyncHdr, message);
    dmReq->ioBlobTxDesc = boost::make_shared<const BlobTxId>(message->txId);

    addToQueue(dmReq);
}

void AbortBlobTxHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoAbortBlobTx* typedRequest = static_cast<DmIoAbortBlobTx*>(dmRequest);

    // Call TVC abortTx
    helper.err = dataManager.timeVolCat_->abortBlobTx(typedRequest->volId,
                                                      typedRequest->ioBlobTxDesc);
}

void AbortBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::AbortBlobTxMsg>& message,
                                        Error const& e, DmRequest* dmRequest) {
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::AbortBlobTxRspMsgTypeId, fpi::AbortBlobTxRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
