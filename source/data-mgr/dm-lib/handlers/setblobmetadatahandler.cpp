/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <PerfTrace.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>

namespace fds {
namespace dm {

SetBlobMetaDataHandler::SetBlobMetaDataHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::SetBlobMetaDataMsg, handleRequest);
    }
}

void SetBlobMetaDataHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                           boost::shared_ptr<fpi::SetBlobMetaDataMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID();

    // Handle U-turn
    HANDLE_U_TURN();

    // U-turn specific to set metadata
    if (dataManager.testUturnSetMeta) {
        LOGNOTIFY << "Uturn testing" << logString(*message);
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

    HANDLE_FILTER_OLD_DMT_DURING_RESYNC();

    auto dmReq = new DmIoSetBlobMetaData(message);
    dmReq->cb = BIND_MSG_CALLBACK(SetBlobMetaDataHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void SetBlobMetaDataHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoSetBlobMetaData* typedRequest = static_cast<DmIoSetBlobMetaData*>(dmRequest);

    ENSURE_IO_ORDER(typedRequest);

    LOGDEBUG << "Will setBlobMetaData  blob " << *typedRequest;
    helper.err = dataManager.timeVolCat_->updateBlobTx(typedRequest->volId,
                                                       typedRequest->ioBlobTxDesc,
                                                       typedRequest->md_list);
}

void SetBlobMetaDataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                            boost::shared_ptr<fpi::SetBlobMetaDataMsg>& message,
                                            Error const& e, DmRequest* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::SetBlobMetaDataRspMsgTypeId, fpi::SetBlobMetaDataRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
