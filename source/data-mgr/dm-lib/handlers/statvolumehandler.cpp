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

StatVolumeHandler::StatVolumeHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::StatVolumeMsg, handleRequest);
    }
}

void StatVolumeHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::StatVolumeMsg>& message) {
    fds_volid_t volId(message->volume_id);
    LOGTRACE << "Received a statVolume request for volume "
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
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoStatVolume(message);
    dmReq->cb = BIND_MSG_CALLBACK(StatVolumeHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void StatVolumeHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoStatVolume* typedRequest = static_cast<DmIoStatVolume*>(dmRequest);

    helper.err = dataManager.timeVolCat_->queryIface()->statVolumeLogical(
            typedRequest->getVolId(),
            // FIXME(DAC): These casts are poster-children for inappropriate usage of
            //             reinterpret_cast.
            reinterpret_cast<fds_uint64_t*>(&typedRequest->msg->volumeStatus.size),
            reinterpret_cast<fds_uint64_t*>(&typedRequest->msg->volumeStatus.blobCount),
            reinterpret_cast<fds_uint64_t*>(&typedRequest->msg->volumeStatus.objectCount));
}

void StatVolumeHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::StatVolumeMsg>& message,
                                       Error const& e, DmRequest* dmRequest) {
    LOGTRACE << "Finished stat for volume " << message->volume_id;
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::StatVolumeMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
