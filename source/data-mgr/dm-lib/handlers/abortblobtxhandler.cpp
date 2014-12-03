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

    auto dmReq = new DmIoAbortBlobTx(message->volume_id,
                                     message->blob_name,
                                     message->blob_version);
    dmReq->cb = BIND_MSG_CALLBACK(AbortBlobTxHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    // Allocate a new Blob transaction class and queue to per volume queue.
    dmReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(message->txId));

    addToQueue(dmReq);
}

void AbortBlobTxHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoAbortBlobTx* typedRequest = static_cast<DmIoAbortBlobTx*>(dmRequest);

    auto blobTxId = typedRequest->ioBlobTxDesc;
    fds_verify(*blobTxId != blobTxIdInvalid);

    // Call TVC abortTx
    dataMgr->timeVolCat_->abortBlobTx(typedRequest->volId, blobTxId);
}

void AbortBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::AbortBlobTxMsg>& message,
                                        Error const& e, dmCatReq* dmRequest) {
    // We are not sending any response message in this call, instead we
    // will send th estatus on async header
    // TODO(sanjay) - we will have to add  call to  send the response without payload
    // static response
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = e.GetErrno();
    // TODO(sanjay) - we will have to revisit  this call
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::AbortBlobTxRspMsgTypeId, fpi::AbortBlobTxRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
