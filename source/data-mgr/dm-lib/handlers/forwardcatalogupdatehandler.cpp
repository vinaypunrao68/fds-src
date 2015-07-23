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
    dmReq->cb = BIND_MSG_CALLBACK(ForwardCatalogUpdateHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);

    DBG(LOGMIGRATE << "Enqueued new forward request " << logString(*asyncHdr)
        << " " << *reinterpret_cast<DmIoFwdCat*>(dmReq));
}

void ForwardCatalogUpdateHandler::handleQueueItem(DmRequest* dmRequest) {
    DmIoFwdCat* typedRequest = static_cast<DmIoFwdCat*>(dmRequest);
    QueueHelper helper(dataManager, typedRequest);

    LOGMIGRATE << "Will commit fwd blob " << *typedRequest << " to tvc";
    helper.err = dataManager.timeVolCat_->updateFwdCommittedBlob(
            typedRequest->volId,
            typedRequest->blob_name,
            typedRequest->blob_version,
            typedRequest->fwdCatMsg->obj_list,
            typedRequest->fwdCatMsg->meta_list,
            typedRequest->fwdCatMsg->sequence_id);
}

/**
 * Invoked when the forward request completes.
 */
void ForwardCatalogUpdateHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                 boost::shared_ptr<fpi::ForwardCatalogMsg>& message,
                                                 Error const& e,
                                                 DmRequest* dmRequest) {
    DBG(LOGMIGRATE << "Completed request " << logString(*asyncHdr) << " "
        << *reinterpret_cast<DmIoFwdCat*>(dmRequest));
    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::ForwardCatalogRspMsgTypeId, fpi::ForwardCatalogRspMsg());
    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
