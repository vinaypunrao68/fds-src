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

VolumegroupUpdateHandler::VolumegroupUpdateHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::VolumeGroupInfoUpdateCtrlMsg, handleRequest);
    }
}

void VolumegroupUpdateHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::VolumeGroupInfoUpdateCtrlMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    fds_volid_t volId(message->group.groupId);
    auto volMeta = dataManager.getVolumeMeta(volId);
    if (volMeta == nullptr) {
        Error err = ERR_VOL_NOT_FOUND;
        LOGWARN << "Failed setting volumegroup info vol: " << volId
            << " " << err;
        handleResponse(asyncHdr, err, nullptr);
        return;
    }

    auto dmReq = new DmIoVolumegroupUpdate(volId, message);
    dmReq->cb = BIND_MSG_CALLBACK(VolumegroupUpdateHandler::handleResponse, asyncHdr);
    addToQueue(dmReq);
}

void VolumegroupUpdateHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoVolumegroupUpdate* request = static_cast<DmIoVolumegroupUpdate*>(dmRequest);

    LOGDEBUG << "Attempting to set volumegroup info for vol: '"
             << std::hex << request->volId << std::dec << "'";
    auto volMeta = dataManager.getVolumeMeta(request->volId);
    if (volMeta == nullptr) {
        LOGWARN << "Failed setting volumegroup info vol: " << request->volId
            << ". Volume not found";
        helper.err = ERR_VOL_NOT_FOUND;
        return;
    } else if (volMeta->getState() != fpi::Loading) {
        LOGWARN << "Failed setting volumegroup info vol: " << request->volId
            << ". Volume isn't in loading state";
        helper.err = ERR_INVALID;
        return;
    } else if (volMeta->getSequenceId() != static_cast<uint64_t>(request->message->group.lastCommitId)) {
        LOGWARN << "vol: " << request->volId << " doesn't have active state."
            << " current sequence id: " << volMeta->getSequenceId()
            << " expected sequence id: " << request->message->group.lastCommitId;
        volMeta->setState(fpi::Offline, " - VolumegroupUpdateHandler:sequence id mismatch");
        // TODO(Rao): At this point we should trigger a sync
        return;
    }
    volMeta->setOpId(request->message->group.lastOpId);
    volMeta->setState(fpi::Active, " - VolumegroupUpdateHandler:state matched with coordinator");
}

void VolumegroupUpdateHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              Error const& e, DmRequest* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    auto response = fpi::EmptyMsg();
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), response);
    if (dmRequest) delete dmRequest;
}


}  // namespace dm
}  // namespace fds
