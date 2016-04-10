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

    LOGDEBUG << "Attempting to open volume: " << request->volId;

    auto volMeta = dataManager.getVolumeMeta(request->volId);
    if (dataManager.features.isVolumegroupingEnabled()) {

        /* Request to open from volume group handle that wishes to do just reads.
         * When trying to do reads it's important the volume can eventually be activated.
         * With VG, the only volume can be activated is via open from coordinator.
         * If coordinator isn't set we return ERR_DM_VOL_NOT_ACTIVATED so that group
         * handle can retry and open as a coordinator.
         * If coordinator is set then the volume can be read eventually.
         */
        if (!request->msg->mode.can_write) {
            if (!volMeta->isCoordinatorSet()) {
                helper.err = ERR_DM_VOL_NOT_ACTIVATED;
            }
            return;
        }

        /* When syncing is in progress we won't allow open.
         * NOTE: When sync is in progress, force flag has no effect. This may need to be
         * revisted.  At the moment we don't have a way to cancel ongoing sync process
         */
        if (volMeta->isInitializerInProgress()) {
            LOGWARN << volMeta->logString() << " Failed to open.  Sync is in progress";
            helper.err = ERR_SYNC_INPROGRESS;
            return;
        }

        /* When force isn't set (force open from coordinator), we reject setting coordinator
         * if one is already set
         */
        if (!request->msg->force &&
            volMeta->isCoordinatorSet() &&
            volMeta->getCoordinatorId() != request->msg->coordinator.id) {
            LOGWARN << "Rejecting openvolume request due to coordinator mismatch"
                << " volume: " << request->volId
                << " from: " << request->msg->coordinator.id
                << " stored: " << volMeta->getCoordinatorId();
            helper.err = ERR_INVALID_COORDINATOR;
            request->coordinator = volMeta->getCoordinator();
            return;
        }

        /* Set coordinator when either force is set or coordinator wasn't set */
        volMeta->setCoordinator(request->msg->coordinator);
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
        LOGDEBUG << "on opening volume: " << request->volId
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
        response.coordinator = request->coordinator;
    }
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::OpenVolumeRspMsg), response);
    if (dmRequest) {
        delete dmRequest;
        dmRequest = nullptr;
    }
}

}  // namespace dm
}  // namespace fds
