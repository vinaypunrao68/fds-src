/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>

namespace fds {
namespace dm {

StatStreamHandler::StatStreamHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::StatStreamMsg, handleRequest);
    }
}

void StatStreamHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::StatStreamMsg>& message) {
    // GLOGDEBUG << "received stat stream " << logString(*asyncHdr);

    auto dmReq = new DmIoStatStream(FdsDmSysTaskId, message);
    dmReq->cb = BIND_MSG_CALLBACK(StatStreamHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void StatStreamHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoStatStream* typedRequest = static_cast<DmIoStatStream*>(dmRequest);

    helper.err = dataManager.statStreamAggr_->handleModuleStatStream(typedRequest->statStreamMsg);
}

void StatStreamHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::StatStreamMsg>& message,
                                       Error const& e, DmRequest* dmRequest) {
    DmIoStatStream *req = static_cast<DmIoStatStream*>(dmRequest);
    if (req) {
        DBG(GLOGDEBUG << logString(*asyncHdr) << *static_cast<DmIoStatStream*>(dmRequest));
    } else {
        DBG(GLOGDEBUG << logString(*asyncHdr));
    }

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::StatStreamMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
