/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <PerfTrace.h>
#include <fds_assert.h>

namespace fds {
namespace dm {

SetVolumeMetadataHandler::SetVolumeMetadataHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::SetVolumeMetadataMsg, handleRequest);
    }
}

void SetVolumeMetadataHandler::handleRequest(
    boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
    boost::shared_ptr<fpi::SetVolumeMetadataMsg>& message) {

    fds_volid_t volId(message->volumeId);
    LOGTRACE << "Received a set volume metadata request for volume "
             << volId;

    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    HANDLE_FILTER_OLD_DMT_DURING_RESYNC();

    auto dmReq = new DmIoSetVolumeMetaData(message);
    dmReq->cb = BIND_MSG_CALLBACK(SetVolumeMetadataHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void SetVolumeMetadataHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoSetVolumeMetaData* typedRequest = static_cast<DmIoSetVolumeMetaData*>(dmRequest);
    
    ENSURE_IO_ORDER(typedRequest, helper);

    helper.err = dataManager.timeVolCat_->setVolumeMetadata(typedRequest->getVolId(),
                                                            typedRequest->msg->metadataList, typedRequest->msg->sequence_id);
}

void SetVolumeMetadataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::SetVolumeMetadataMsg>& message,
                                       Error const& e, DmRequest* dmRequest) {
    LOGTRACE << "Finished set metadata for volume " << message->volumeId;
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::SetVolumeMetadataRspMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
