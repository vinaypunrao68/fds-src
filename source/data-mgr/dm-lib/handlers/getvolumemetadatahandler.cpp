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

GetVolumeMetadataHandler::GetVolumeMetadataHandler() {
    if (!dataMgr->features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetVolumeMetadataMsg, handleRequest);
    }
}

void GetVolumeMetadataHandler::handleRequest(
    boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
    boost::shared_ptr<fpi::GetVolumeMetadataMsg>& message) {
    LOGTRACE << "Received a get volume metadata request for volume "
             << message->volumeId;

    boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp> response =
            boost::make_shared<fpi::GetVolumeMetadataMsgRsp>();
    auto dmReq = new DmIoGetVolumeMetadata(message->volumeId, response);
    dmReq->cb = BIND_MSG_CALLBACK(GetVolumeMetadataHandler::handleResponse, asyncHdr, response);

    addToQueue(dmReq);
}

void GetVolumeMetadataHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoGetVolumeMetadata* typedRequest = static_cast<DmIoGetVolumeMetadata*>(dmRequest);

    helper.err = dataMgr->timeVolCat_->queryIface()->getVolumeMetadata(typedRequest->getVolId(),
                                                                       typedRequest->msg->metadataList);
}

void GetVolumeMetadataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp>& message,
                                              Error const& e, dmCatReq* dmRequest) {
    LOGTRACE << "Finished get metadata for volume " << dmRequest->volId;
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::GetVolumeMetadataRspMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds