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

VolumeOpenHandler::VolumeOpenHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::OpenVolumeMsg, handleRequest);
    }
}

void VolumeOpenHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::OpenVolumeMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    // Handle U-turn
    HANDLE_U_TURN();

    auto err = dataManager.validateVolumeIsActive(message->volume_id);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoVolumeOpen(message);
    dmReq->cb = BIND_MSG_CALLBACK(VolumeOpenHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void VolumeOpenHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoVolumeOpen * request = static_cast<DmIoVolumeOpen *>(dmRequest);

    LOGDEBUG << "Attempting to open volume: '"
             << std::hex << request->volId << std::dec << "'";

    helper.err = dataManager.timeVolCat_->openVolume(request->volId, request->token,
                                                     request->access_policy);
}

void VolumeOpenHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::OpenVolumeMsg>& message,
                                              Error const& e, dmCatReq* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    auto response = fpi::OpenVolumeRspMsg();
    DmIoVolumeOpen * request = static_cast<DmIoVolumeOpen *>(dmRequest);
    response.token = request->token;
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::OpenVolumeRspMsg), response);
    if (dmRequest)
        delete dmRequest;
}

}  // namespace dm
}  // namespace fds
