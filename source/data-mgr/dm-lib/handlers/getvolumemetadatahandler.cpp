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

GetVolumeMetadataHandler::GetVolumeMetadataHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetVolumeMetadataMsg, handleRequest);
    }
}

void GetVolumeMetadataHandler::handleRequest(
    boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
    boost::shared_ptr<fpi::GetVolumeMetadataMsg>& message) {

    fds_volid_t volId(message->volumeId);
    LOGTRACE << "Received a get volume metadata request for volume "
             << volId;

    Error err(ERR_OK);
    if (!dataManager.amIPrimaryGroup(volId)) {
    	err = ERR_DM_NOT_PRIMARY;
    }
    if (err.OK()) {
    	err = dataManager.validateVolumeIsActive(volId);
    }
    if (!err.OK())
    {
        auto dummyResponse = boost::make_shared<fpi::GetVolumeMetadataMsgRsp>();
        handleResponse(asyncHdr, dummyResponse, err, nullptr);
        return;
    }

    boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp> response =
            boost::make_shared<fpi::GetVolumeMetadataMsgRsp>();
    auto dmReq = new DmIoGetVolumeMetadata(volId, response);
    dmReq->cb = BIND_MSG_CALLBACK(GetVolumeMetadataHandler::handleResponse, asyncHdr, response);

    addToQueue(dmReq);
}

void GetVolumeMetadataHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoGetVolumeMetadata* typedRequest = static_cast<DmIoGetVolumeMetadata*>(dmRequest);

    helper.err = dataManager
                .timeVolCat_
               ->queryIface()
               ->getVolumeMetadata(typedRequest->getVolId(), typedRequest->msg->metadataList);
}

void GetVolumeMetadataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::GetVolumeMetadataMsgRsp>& message,
                                              Error const& e, DmRequest* dmRequest) {
    LOGTRACE << "Finished get metadata for volume " << dmRequest->volId;
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::GetVolumeMetadataRspMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
