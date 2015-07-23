/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>

namespace fds {
namespace dm {

GetBlobMetaDataHandler::GetBlobMetaDataHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetBlobMetaDataMsg, handleRequest);
    }
}

void GetBlobMetaDataHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                           boost::shared_ptr<fpi::GetBlobMetaDataMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    fds_volid_t volId(message->volume_id);
    Error err(ERR_OK);
    if (!dataManager.amIPrimaryGroup(volId)) {
    	err = ERR_DM_NOT_PRIMARY;
    }
    if (err.OK()) {
    	err = dataManager.validateVolumeIsActive(volId);
    }
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoGetBlobMetaData(volId,
                                         message->blob_name,
                                         message->blob_version,
                                         message);
    dmReq->cb = BIND_MSG_CALLBACK(GetBlobMetaDataHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void GetBlobMetaDataHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoGetBlobMetaData* typedRequest = static_cast<DmIoGetBlobMetaData*>(dmRequest);

    fds_uint64_t blobSize {0};
    helper.err = dataManager
                .timeVolCat_
               ->queryIface()
               ->getBlobMeta(typedRequest->volId,
                             typedRequest->blob_name,
                             &typedRequest->blob_version,
                             &blobSize,
                             &typedRequest->message->metaDataList);
    if (!helper.err.ok()) {
        PerfTracer::incr(typedRequest->opReqFailedPerfEventType,
                         typedRequest->getVolId());
    }
    typedRequest->message->byteCount = blobSize;
}

// TODO(Rao): Refactor DmRequest to contain BlobNode information? Check with Andrew.
void GetBlobMetaDataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                            boost::shared_ptr<fpi::GetBlobMetaDataMsg>& message,
                                            Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    // TODO(Rao): We should have a seperate response message for QueryCatalogMsg for
    // consistency
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::GetBlobMetaDataMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
