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
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::OpenVolumeMsg, handleRequest);
    }
}

void VolumeOpenHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::OpenVolumeMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    // Handle U-turn
    HANDLE_U_TURN();

    Error err;
    fds_volid_t volId(message->volume_id);

    if (dataManager.features.isVolumegroupingEnabled()) {
        err = dataManager.validateVolumeExists(volId);
    } else {
        err = dataManager.validateVolumeIsActive(volId);
    }
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoVolumeOpen(message, asyncHdr->msg_src_uuid);
    dmReq->cb = BIND_MSG_CALLBACK(VolumeOpenHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void VolumeOpenHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoVolumeOpen * request = static_cast<DmIoVolumeOpen *>(dmRequest);

    LOGDEBUG << "Attempting to open volume: '"
             << std::hex << request->volId << std::dec << "'";

    auto volMeta = dataManager.getVolumeMeta(request->volId);
    if (dataManager.features.isVolumegroupingEnabled()) {
        if (volMeta->isInitializerInProgress()) {
            LOGWARN << volMeta->logString() << " Failed to open.  Sync is in progress";
            helper.err = ERR_SYNC_INPROGRESS;
            return;
        }
        if (volMeta->isCoordinatorSet() &&
            request->msg->coordinatorVersion < volMeta->getCoordinatorVersion()) {
            LOGWARN << "Rejecting openvolume request due to version check failure.  from: "
                << request->client_uuid_
                << " request version: " << request->msg->coordinatorVersion
                << " stored version: " << volMeta->getCoordinatorVersion();
            helper.err = ERR_INVALID_VERSION;
            return;
        }
        volMeta->setCoordinatorId(request->client_uuid_);
        volMeta->setCoordinatorVersion(request->msg->coordinatorVersion);
        volMeta->setState(fpi::Loading,
                          util::strformat(" - openvolume from coordinator: %ld",
                                          volMeta->getCoordinatorId().svc_uuid));
    }

    helper.err = dataManager.timeVolCat_->openVolume(request->volId,
                                                     request->client_uuid_,
                                                     request->token,
                                                     request->access_mode,
                                                     request->sequence_id,
                                                     request->version);

    if (helper.err.ok()) {
        LOGDEBUG << "on opening vol: " << request->volId
                 << ", latest sequence was determined to be "
                 << request->sequence_id;
    }
}

void VolumeOpenHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::OpenVolumeMsg>& message,
                                              Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());

    DBG(GLOGDEBUG << logString(*asyncHdr));

    auto response = fpi::OpenVolumeRspMsg();
    if (dmRequest) {
        DmIoVolumeOpen * request = static_cast<DmIoVolumeOpen *>(dmRequest);
        response.token = request->token;
        response.sequence_id = request->sequence_id;
        response.replicaVersion = request->version;
    }
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::OpenVolumeRspMsg), response);
    if (dmRequest) {
        delete dmRequest;
        dmRequest = nullptr;
    }
}

}  // namespace dm
}  // namespace fds
