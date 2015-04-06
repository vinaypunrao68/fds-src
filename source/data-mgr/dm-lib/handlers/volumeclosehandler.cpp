/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <DmIoReq.h>
#include <PerfTrace.h>

namespace fds {
namespace dm {

VolumeCloseHandler::VolumeCloseHandler() {
    if (!dataMgr->features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CloseVolumeMsg, handleRequest);
    }
}

void VolumeCloseHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::CloseVolumeMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    // Handle U-turn
    HANDLE_U_TURN();

    auto dmReq = new DmIoVolumeClose(message->volume_id, message->token);
    dmReq->cb = BIND_MSG_CALLBACK(VolumeCloseHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void VolumeCloseHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoVolumeClose * request = static_cast<DmIoVolumeClose *>(dmRequest);

    LOGDEBUG << "Attempting to close volume: '"
             << std::hex << request->volId << std::dec << "'";

    helper.err = dataMgr->timeVolCat_->closeVolume(request->volId, request->token);
}

void VolumeCloseHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::CloseVolumeMsg>& message,
                                              Error const& e, dmCatReq* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::CloseVolumeRspMsg),
            fpi::CloseVolumeRspMsg());
    if (dmRequest)
        delete dmRequest;
}

}  // namespace dm
}  // namespace fds
