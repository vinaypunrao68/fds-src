/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <list>
#include <DataMgr.h>
#include <dmhandler.h>
#include <DMSvcHandler.h>

#define MAX_EXTENT0_OBJS 1024
#define MAX_EXTENT_OBJS 2048

namespace fds {
namespace dm {

DmSysStatsHandler::DmSysStatsHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetDmStatsMsg, handleRequest);
    }
}

void DmSysStatsHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::GetDmStatsMsg>& message) {
    fds_volid_t volId(message->volume_id);
    LOGDEBUG << "volume: " << volId;

    Error err(ERR_OK);
    // if (!dataManager.amIPrimaryGroup(volId)) {err = ERR_DM_NOT_PRIMARY;}
    if (err.OK()) {
    	err = dataManager.validateVolumeIsActive(volId);
    }
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    // setup the request
    auto dmRequest = new DmIoGetSysStats(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(DmSysStatsHandler::handleResponse, asyncHdr, message);

    addToQueue(dmRequest);
}

void DmSysStatsHandler::handleQueueItem(DmRequest *dmRequest) {
    QueueHelper helper(dataManager, dmRequest);  // this will call the callback
    DmIoGetSysStats *request = static_cast<DmIoGetSysStats*>(dmRequest);

    LOGDEBUG << "volume: " << dmRequest->volId;
    // do processing and set the error
    DmCommitLog::ptr commitLog;
    VolumeCatalogQueryIface::ptr volCatIf = dataManager.timeVolCat_->queryIface();
    dataManager.timeVolCat_->getCommitlog(dmRequest->volId, commitLog);

    request->message->commitlog_size = commitLog->getActiveTx();
    request->message->extent0_size = MAX_EXTENT0_OBJS;
    request->message->extent_size = MAX_EXTENT_OBJS;
    request->message->metadata_size = volCatIf->getTotalMetadataSize(dmRequest->volId);
}

void DmSysStatsHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::GetDmStatsMsg>& message,
                                       const Error &e, DmRequest *dmRequest) {
    LOGDEBUG << " volid: " << (dmRequest ? dmRequest->volId : invalid_vol_id)
             << " err: " << e;
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(asyncHdr, fpi::GetDmStatsMsgTypeId, message);
    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
