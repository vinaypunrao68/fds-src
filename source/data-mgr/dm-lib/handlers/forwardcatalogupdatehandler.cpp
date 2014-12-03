/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>

#include <functional>

namespace fds {
namespace dm {

ForwardCatalogUpdateHandler::ForwardCatalogUpdateHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::ForwardCatalogMsg, handleRequest);
    }
}

void ForwardCatalogUpdateHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::ForwardCatalogMsg>& message) {
    auto dmReq = new DmIoFwdCat(message);
    dmReq->cb = BIND_MSG_CALLBACK(ForwardCatalogUpdateHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void ForwardCatalogUpdateHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoFwdCat* typedRequest = static_cast<DmIoFwdCat*>(dmRequest);

    LOGTRACE << "Will commit fwd blob " << *typedRequest << " to tvc";
    helper.err = dataMgr->timeVolCat_->updateFwdCommittedBlob(
            typedRequest->volId,
            typedRequest->blob_name,
            typedRequest->blob_version,
            typedRequest->fwdCatMsg->obj_list,
            typedRequest->fwdCatMsg->meta_list,
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
    QueueHelper helper(fwdCatReq);

    LOGTRACE << "Commited fwd blob " << *fwdCatReq;
}

void ForwardCatalogUpdateHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                 boost::shared_ptr<fpi::ForwardCatalogMsg>& message,
                                                 Error const& e, dmCatReq* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << " " << *reinterpret_cast<DmIoFwdCat*>(dmRequest));
    asyncHdr->msg_code = e.GetErrno();

    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::ForwardCatalogRspMsgTypeId, fpi::ForwardCatalogRspMsg());

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
