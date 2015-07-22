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

    fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoVolumeOpen(message, asyncHdr->msg_src_uuid);
    dmReq->cb = BIND_MSG_CALLBACK(VolumeOpenHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void VolumeOpenHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoVolumeOpen * request = static_cast<DmIoVolumeOpen *>(dmRequest);

    LOGDEBUG << "Attempting to open volume: '"
             << std::hex << request->volId << std::dec << "'";

    helper.err = dataManager.timeVolCat_->openVolume(request->volId,
                                                     request->client_uuid_,
                                                     request->token,
                                                     request->access_mode,
                                                     request->sequence_id);

    if (helper.err.ok()) {
        LOGDEBUG << "on opening vol: " << request->volId
                 << ", latest sequence was determined to be "
                 << request->sequence_id;
    }
}

void VolumeOpenHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::OpenVolumeMsg>& message,
                                              Error const& e, DmRequest* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    auto response = fpi::OpenVolumeRspMsg();
    if (dmRequest) {
        DmIoVolumeOpen * request = static_cast<DmIoVolumeOpen *>(dmRequest);
        response.token = request->token;
        response.sequence_id = request->sequence_id;
    }
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::OpenVolumeRspMsg), response);
    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
