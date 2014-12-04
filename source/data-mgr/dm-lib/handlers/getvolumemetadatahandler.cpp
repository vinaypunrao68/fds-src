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

GetVolumeMetaDataHandler::GetVolumeMetaDataHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetVolumeMetaDataMsg, handleRequest);
    }
}

void GetVolumeMetaDataHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::GetVolumeMetaDataMsg>& message) {
    LOGNORMAL << "get vol meta data msg ";

    auto dmReq = new DmIoGetVolumeMetaData(message);
    dmReq->cb = BIND_MSG_CALLBACK(GetVolumeMetaDataHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void GetVolumeMetaDataHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoGetVolumeMetaData* typedRequest = static_cast<DmIoGetVolumeMetaData*>(dmRequest);

    helper.err = dataMgr->timeVolCat_->queryIface()->getVolumeMeta(
            typedRequest->getVolId(),
            // FIXME(DAC): These casts are poster-children for inappropriate usage of
            //             reinterpret_cast.
            reinterpret_cast<fds_uint64_t*>(&typedRequest->msg->volume_meta_data.size),
            reinterpret_cast<fds_uint64_t*>(&typedRequest->msg->volume_meta_data.blobCount),
            reinterpret_cast<fds_uint64_t*>(&typedRequest->msg->volume_meta_data.objectCount));
}

void GetVolumeMetaDataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::GetVolumeMetaDataMsg>& message,
                                              Error const& e, dmCatReq* dmRequest) {
    LOGNORMAL << "finished get volume meta";
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::GetVolumeMetaDataMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
