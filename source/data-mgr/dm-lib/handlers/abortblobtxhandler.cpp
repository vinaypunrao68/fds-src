/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <PerfTrace.h>
#include <blob/BlobTypes.h>
#include <fds_assert.h>

namespace fds {
namespace dm {

AbortBlobTxHandler::AbortBlobTxHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::AbortBlobTxMsg, handleRequest);
    }
}

void AbortBlobTxHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::AbortBlobTxMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID();

    // Handle U-turn
    HANDLE_U_TURN();

    // Allocate a new Blob transaction class and queue to per volume queue.
    auto dmReq = new DmIoAbortBlobTx(message->volume_id,
                                     message->blob_name,
                                     message->blob_version);
    dmReq->cb = BIND_MSG_CALLBACK(AbortBlobTxHandler::handleResponse, asyncHdr, message);
    dmReq->ioBlobTxDesc = boost::make_shared<const BlobTxId>(message->txId);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void AbortBlobTxHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoAbortBlobTx* typedRequest = static_cast<DmIoAbortBlobTx*>(dmRequest);

    // Call TVC abortTx
    helper.err = dataMgr->timeVolCat_->abortBlobTx(typedRequest->volId, typedRequest->ioBlobTxDesc);
}

void AbortBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::AbortBlobTxMsg>& message,
                                        Error const& e, dmCatReq* dmRequest) {
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::AbortBlobTxRspMsgTypeId, fpi::AbortBlobTxRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
