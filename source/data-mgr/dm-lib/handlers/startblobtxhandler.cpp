/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fds_error.h>
#include <fdsp_utils.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <PerfTrace.h>
#include <VolumeMeta.h>

namespace fds {
namespace dm {

StartBlobTxHandler::StartBlobTxHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::StartBlobTxMsg, handleRequest);
    }
}

void StartBlobTxHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::StartBlobTxMsg>& message) {
    if (dataMgr->testUturnAll || dataMgr->testUturnStartTx) {
        LOGNOTIFY << "Uturn testing start blob tx " << logString(*asyncHdr) << logString(*message);
        handleResponse(asyncHdr, message, ERR_OK, nullptr);
        return;
    }

    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    auto dmReq = new DmIoStartBlobTx(message->volume_id,
                                     message->blob_name,
                                     message->blob_version,
                                     message->blob_mode,
                                     message->dmt_version);
    dmReq->cb = BIND_MSG_CALLBACK(StartBlobTxHandler::handleResponse, asyncHdr, message);
    dmReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(message->txId));

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void StartBlobTxHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoStartBlobTx* typedRequest = static_cast<DmIoStartBlobTx*>(dmRequest);

    helper.err = ERR_OK;

    LOGTRACE << "Will start transaction " << *typedRequest;

    // TODO(Anna) If this DM is not forwarding for this io's volume anymore
    // we reject start TX on DMT mismatch
    dataMgr->vol_map_mtx->lock();
    if (dataMgr->vol_meta_map.count(typedRequest->volId) > 0) {
        VolumeMeta* vol_meta = dataMgr->vol_meta_map[typedRequest->volId];
        if ((!vol_meta->isForwarding() || vol_meta->isForwardFinishing()) &&
            (typedRequest->dmt_version != dataMgr->omClient->getDMTVersion())) {
            PerfTracer::incr(typedRequest->opReqFailedPerfEventType, typedRequest->getVolId(),
                    typedRequest->perfNameStr);
            helper.err = ERR_IO_DMT_MISMATCH;
        }
    } else {
        PerfTracer::incr(typedRequest->opReqFailedPerfEventType, typedRequest->getVolId(),
                typedRequest->perfNameStr);
        helper.err = ERR_VOL_NOT_FOUND;
    }
    dataMgr->vol_map_mtx->unlock();

    if (helper.err.ok()) {
        helper.err = dataMgr->timeVolCat_->startBlobTx(typedRequest->volId,
                                                       typedRequest->blob_name,
                                                       typedRequest->blob_mode,
                                                       typedRequest->ioBlobTxDesc);
        if (!helper.err.ok()) {
            PerfTracer::incr(typedRequest->opReqFailedPerfEventType, typedRequest->getVolId(),
                    typedRequest->perfNameStr);
        }
    }
}

void StartBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::StartBlobTxMsg>& message,
                                        Error const& e, dmCatReq* dmRequest) {
    /*
     * we are not sending any response  message  in this call, instead we
     * will send the status  on async header
     * TODO(sanjay)- we will have to add  call to  send the response without payload
     * static response
     */
    asyncHdr->msg_code = e.GetErrno();
    LOGDEBUG << "startBlobTx completed " << e << " " << logString(*asyncHdr);

    // TODO(sanjay) - we will have to revisit  this call
    fpi::StartBlobTxRspMsg stBlobTxRsp;
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(StartBlobTxRspMsg), stBlobTxRsp);

    if (dataMgr->testUturnAll || dataMgr->testUturnStartTx) {
        fds_verify(dmRequest == nullptr);
    } else {
        delete dmRequest;
    }
}

}  // namespace dm
}  // namespace fds
