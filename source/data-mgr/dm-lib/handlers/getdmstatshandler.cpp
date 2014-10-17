/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <list>
#include <DataMgr.h>
#include <dmhandler.h>
#include <DMSvcHandler.h>
#include <dm-vol-cat/DmPersistVc.h>

namespace fds {
namespace dm {

DmSysStatsHandler::DmSysStatsHandler() {
    REGISTER_DM_MSG_HANDLER(fpi::GetDmStatsMsg, handleRequest);
}

void DmSysStatsHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::GetDmStatsMsg>& message) {
    LOGDEBUG << "volume: " << message->volume_id;
    // setup the request
    auto dmRequest = new DmIoGetSysStats(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(DmSysStatsHandler::handleResponse, asyncHdr, message);

    addToQueue(dmRequest);
}

void DmSysStatsHandler::handleQueueItem(dmCatReq *dmRequest) {
    QueueHelper helper(dmRequest);  // this will call the callback
    DmIoGetSysStats *request = static_cast<DmIoGetSysStats*>(dmRequest);

    LOGDEBUG << "volume: " << dmRequest->volId;
    // do processing and set the error
     DmCommitLog::ptr commitLog;
     VolumeCatalogQueryIface::ptr volCatIf = dataMgr->timeVolCat_->queryIface();
     dataMgr->timeVolCat_->getCommitlog(dmRequest->volId, commitLog);

     request->message->commitlog_size = commitLog->getActiveTx();
     request->message->extent0_size = MAX_EXTENT0_OBJS;
     request->message->extent_size = MAX_EXTENT_OBJS;
     request->message->metadata_size = volCatIf->getTotalMetadataSize(dmRequest->volId);
}

void DmSysStatsHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::GetDmStatsMsg>& message,
                                      const Error &e, dmCatReq *dmRequest) {
    LOGDEBUG << " volid: " << dmRequest->volId
             << " err: " << e;
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(asyncHdr, fpi::GetDmStatsMsgTypeId, message);
    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
