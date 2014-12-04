/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <DmIoReq.h>
#include <PerfTrace.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>

namespace fds {
namespace dm {

SetBlobMetaDataHandler::SetBlobMetaDataHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::SetBlobMetaDataMsg, handleRequest);
    }
}

void SetBlobMetaDataHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                           boost::shared_ptr<fpi::SetBlobMetaDataMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr));

    // TODO(xxx) implement uturn
    if (dataMgr->testUturnAll || dataMgr->testUturnSetMeta) {
        GLOGNOTIFY << "Uturn testing set metadata";
        fds_panic("not implemented");
    }

    auto dmReq = new DmIoSetBlobMetaData(message);
    dmReq->cb = BIND_MSG_CALLBACK(SetBlobMetaDataHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void SetBlobMetaDataHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoSetBlobMetaData* typedRequest = static_cast<DmIoSetBlobMetaData*>(dmRequest);

    helper.err = dataMgr->timeVolCat_->updateBlobTx(typedRequest->volId,
                                                    typedRequest->ioBlobTxDesc,
                                                    typedRequest->md_list);
    if (!helper.err.ok()) {
        PerfTracer::incr(typedRequest->opReqFailedPerfEventType, typedRequest->getVolId(),
                typedRequest->perfNameStr);
    }
}

void SetBlobMetaDataHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                            boost::shared_ptr<fpi::SetBlobMetaDataMsg>& message,
                                            Error const& e, dmCatReq* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::SetBlobMetaDataRspMsgTypeId, fpi::SetBlobMetaDataRspMsg());

    if (dataMgr->testUturnAll || dataMgr->testUturnSetMeta) {
        fds_panic("not implemented");
    } else {
        delete dmRequest;
    }
}

}  // namespace dm
}  // namespace fds
