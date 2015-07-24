/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fds_error.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>
#include <VolumeMeta.h>

namespace fds {
namespace dm {

StartBlobTxHandler::StartBlobTxHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::StartBlobTxMsg, handleRequest);
    }
}

void StartBlobTxHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::StartBlobTxMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID();

    // Handle U-turn
    HANDLE_U_TURN();

    // start blob tx specific U-turn
    if (dataManager.testUturnStartTx) {
        LOGNOTIFY << "Uturn testing start blob tx " << logString(*asyncHdr) << logString(*message);
        handleResponse(asyncHdr, message, ERR_OK, nullptr);
        return;
    }

    fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoStartBlobTx(volId,
                                     message->blob_name,
                                     message->blob_version,
                                     message->blob_mode,
                                     message->dmt_version);
    dmReq->cb = BIND_MSG_CALLBACK(StartBlobTxHandler::handleResponse, asyncHdr, message);
    dmReq->ioBlobTxDesc = boost::make_shared<const BlobTxId>(message->txId);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void StartBlobTxHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoStartBlobTx* typedRequest = static_cast<DmIoStartBlobTx*>(dmRequest);

    LOGTRACE << "Will start transaction " << *typedRequest;

    // TODO(Anna) If this DM is not forwarding for this io's volume anymore
    // we reject start TX on DMT mismatch
    dataManager.vol_map_mtx->lock();
    auto volMetaIter = dataManager.vol_meta_map.find(typedRequest->volId);
    if (dataManager.vol_meta_map.end() != volMetaIter) {
        VolumeMeta* vol_meta = volMetaIter->second;
        if ((!vol_meta->isForwarding() || vol_meta->isForwardFinishing()) &&
            (typedRequest->dmt_version != MODULEPROVIDER()->getSvcMgr()->getDMTVersion())) {
            helper.err = ERR_IO_DMT_MISMATCH;
        }
    } else {
        helper.err = ERR_VOL_NOT_FOUND;
    }
    dataManager.vol_map_mtx->unlock();

    if (helper.err.ok()) {
        helper.err = dataManager.timeVolCat_->startBlobTx(typedRequest->volId,
                                                          typedRequest->blob_name,
                                                          typedRequest->blob_mode,
                                                          typedRequest->ioBlobTxDesc);
    }
}

void StartBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::StartBlobTxMsg>& message,
                                        Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    LOGDEBUG << "startBlobTx completed " << e << " " << logString(*asyncHdr);

    DM_SEND_ASYNC_RESP(*asyncHdr,
                       FDSP_MSG_TYPEID(fpi::StartBlobTxRspMsg),
                       fpi::StartBlobTxRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
