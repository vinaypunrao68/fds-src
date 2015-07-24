/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fds_error.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>
#include <VolumeMeta.h>

namespace fds {
namespace dm {

ReloadVolumeHandler::ReloadVolumeHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::ReloadVolumeMsg, handleRequest);
    }
}

void ReloadVolumeHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::ReloadVolumeMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    // Handle U-turn
    HANDLE_U_TURN();

    fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        auto dummyResponse = boost::make_shared<fpi::ReloadVolumeMsg>();
        handleResponse(asyncHdr, dummyResponse, err, nullptr);
        return;
    }

    auto dmReq = new DmIoReloadVolume(message);
    dmReq->cb = BIND_MSG_CALLBACK(ReloadVolumeHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);
    addToQueue(dmReq);
}

void ReloadVolumeHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoReloadVolume* typedRequest = static_cast<DmIoReloadVolume*>(dmRequest);

    LOGTRACE << "Will reload volume " << *typedRequest;

    fds_volid_t volId(typedRequest->message->volume_id);
    const VolumeDesc * voldesc = dataManager.getVolumeDesc(volId);
    if (!voldesc) {
        LOGERROR << "Volume entry not found in descriptor map for volume '" << volId << "'";
        helper.err = ERR_VOL_NOT_FOUND;
    } else {
        helper.err = dataManager.timeVolCat_->queryIface()->reloadCatalog(*voldesc);
    }

    if (!helper.err.ok()) {
        PerfTracer::incr(typedRequest->opReqFailedPerfEventType,
                         typedRequest->getVolId());
    }
}

void ReloadVolumeHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::ReloadVolumeMsg>& message,
                                        Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    LOGDEBUG << "Reload volume completed " << e << " " << logString(*asyncHdr);

    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::ReloadVolumeRspMsg),
                       fpi::ReloadVolumeRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
