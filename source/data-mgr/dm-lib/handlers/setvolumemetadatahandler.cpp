/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <PerfTrace.h>
#include <fds_assert.h>

namespace fds {
namespace dm {

SetVolumeMetadataHandler::SetVolumeMetadataHandler() {
    if (!dataMgr->features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::SetVolumeMetadataMsg, handleRequest);
    }
}

void SetVolumeMetadataHandler::handleRequest(
    boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
    boost::shared_ptr<fpi::SetVolumeMetadataMsg>& message) {
    LOGTRACE << "Received a set volume metadata request for volume "
             << message->volumeId;

    auto dmReq = new DmIoSetVolumeMetaData(message);
    dmReq->cb = BIND_MSG_CALLBACK(SetVolumeMetadataHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void SetVolumeMetadataHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoSetVolumeMetaData* typedRequest = static_cast<DmIoSetVolumeMetaData*>(dmRequest);

    helper.err = dataMgr->timeVolCat_->setVolumeMetadata(typedRequest->getVolId(),
                                                         typedRequest->msg->metadataList);
}

void SetVolumeMetadataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::SetVolumeMetadataMsg>& message,
                                       Error const& e, dmCatReq* dmRequest) {
    LOGTRACE << "Finished set metadata for volume " << message->volumeId;
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::SetVolumeMetadataRspMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
