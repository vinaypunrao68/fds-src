/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>

#include <functional>

namespace fds {
namespace dm {

DmMigrationDeltablobHandler::DmMigrationDeltablobHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CtrlNotifyDeltaBlobsMsg, handleRequest);
    }
}

void DmMigrationDeltablobHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message) {
    auto dmReq = new DmIoMigDltBlob(message);
    dmReq->cb = BIND_MSG_CALLBACK(DmMigrationDeltablobHandler::handleCompletion, asyncHdr, message);

    addToQueue(dmReq);

    DBG(LOGMIGRATE << "Enqueued delta blob migration  request " << logString(*asyncHdr)
        << " " << *reinterpret_cast<DmIoMigDltBlob*>(dmReq));

    // Reply back now to acknowledge that we received the forward message and properly
    // enqueued it for later processing
    handleResponse(asyncHdr, message, ERR_OK, dmReq);
}

void DmMigrationDeltablobHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoMigDltBlob* typedRequest = static_cast<DmIoMigDltBlob*>(dmRequest);

    LOGMIGRATE << "Sending the delta blob migration dequest to migration Mgr " << *typedRequest;
}

/**
 * Replies back to the caller. Does NOT delete the request because we're only replying
 * on request receipt, not completion.
 */
void DmMigrationDeltablobHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                 boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message,
                                                 Error const& e,
                                                 dmCatReq* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    DBG(LOGMIGRATE << logString(*asyncHdr) << " " << *reinterpret_cast<DmIoFwdCat*>(dmRequest));
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CtrlNotifyDeltaBlobDescRspMsgTypeId, fpi::CtrlNotifyDeltaBlobDescRspMsg());
}

/**
 * Invoked when the forward request completes. We've already replied so we
 * only need to delete the request.
 */
void DmMigrationDeltablobHandler::handleCompletion(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                   boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message,
                                                   Error const& e,
                                                   dmCatReq* dmRequest) {
    DBG(LOGMIGRATE << "Completed request " << logString(*asyncHdr) << " "
        << *reinterpret_cast<DmIoMigDltBlob*>(dmRequest));
    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
