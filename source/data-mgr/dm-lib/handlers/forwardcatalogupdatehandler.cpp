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

ForwardCatalogUpdateHandler::ForwardCatalogUpdateHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::ForwardCatalogMsg, handleRequest);
    }
}

void ForwardCatalogUpdateHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::ForwardCatalogMsg>& message) {
    auto dmReq = new DmIoFwdCat(message);
    dmReq->cb = BIND_MSG_CALLBACK(ForwardCatalogUpdateHandler::handleCompletion, asyncHdr, message);

    addToQueue(dmReq);

    DBG(LOGMIGRATE << "Enqueued new forward request " << logString(*asyncHdr)
        << " " << *reinterpret_cast<DmIoFwdCat*>(dmReq));

    // Reply back now to acknowledge that we received the forward message and properly
    // enqueued it for later processing
    handleResponse(asyncHdr, message, ERR_OK, dmReq);
}

void ForwardCatalogUpdateHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoFwdCat* typedRequest = static_cast<DmIoFwdCat*>(dmRequest);

    LOGMIGRATE << "Will commit fwd blob " << *typedRequest << " to tvc";
    helper.err = dataManager.timeVolCat_->updateFwdCommittedBlob(
            typedRequest->volId,
            typedRequest->blob_name,
            typedRequest->blob_version,
            typedRequest->fwdCatMsg->obj_list,
            typedRequest->fwdCatMsg->meta_list,
            typedRequest->fwdCatMsg->sequence_id,
            std::bind(&ForwardCatalogUpdateHandler::handleUpdateFwdCommittedBlob, this,
                      std::placeholders::_1, typedRequest));

    // The callback, handleUpdateFwdCommittedBlob(), will be called and will handle calling
    // handleResponse().
    if (helper.err.ok()) {
        helper.cancel();
    }
}

void ForwardCatalogUpdateHandler::handleUpdateFwdCommittedBlob(Error const& e,
                                                               DmIoFwdCat* fwdCatReq) {
    QueueHelper helper(dataManager, fwdCatReq);

    LOGTRACE << "Commited fwd blob " << *fwdCatReq;
}

/**
 * Replies back to the caller. Does NOT delete the request because we're only replying
 * on request receipt, not completion.
 */
void ForwardCatalogUpdateHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                 boost::shared_ptr<fpi::ForwardCatalogMsg>& message,
                                                 Error const& e,
                                                 DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    // TODO(Andrew): There is a race here if the request actually gets completed before
    // we reply then the dmRequest may be NULL
    DBG(LOGMIGRATE << logString(*asyncHdr) << " " << *reinterpret_cast<DmIoFwdCat*>(dmRequest));
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::ForwardCatalogRspMsgTypeId, fpi::ForwardCatalogRspMsg());
}

/**
 * Invoked when the forward request completes. We've already replied so we
 * only need to delete the request.
 */
void ForwardCatalogUpdateHandler::handleCompletion(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                   boost::shared_ptr<fpi::ForwardCatalogMsg>& message,
                                                   Error const& e,
                                                   DmRequest* dmRequest) {
    DBG(LOGMIGRATE << "Completed request " << logString(*asyncHdr) << " "
        << *reinterpret_cast<DmIoFwdCat*>(dmRequest));
    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
