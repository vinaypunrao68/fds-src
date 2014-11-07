/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <util/Log.h>
#include <fdsp_utils.h>
#include <platform/fds_flags.h>
#include <DmIoReq.h>
#include <PerfTrace.h>
#include <fds_error.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

QueryCatalogHandler::QueryCatalogHandler() {
    REGISTER_DM_MSG_HANDLER(fpi::QueryCatalogMsg, handleRequest);
}

void QueryCatalogHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::QueryCatalogMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_queries > 0));

    auto dmReq = new DmIoQueryCat(message);
    dmReq->cb = BIND_MSG_CALLBACK(QueryCatalogHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void QueryCatalogHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoQueryCat* typedRequest = static_cast<DmIoQueryCat*>(dmRequest);

    helper.err = dataMgr->timeVolCat_->queryIface()->getBlob(typedRequest->volId,
                                                      typedRequest->blob_name,
                                                      typedRequest->queryMsg->start_offset,
                                                      typedRequest->queryMsg->end_offset,
                                                      &typedRequest->blob_version,
                                                      &typedRequest->queryMsg->meta_list,
                                                      &typedRequest->queryMsg->obj_list);
    if (!helper.err.ok()) {
        PerfTracer::incr(typedRequest->opReqFailedPerfEventType, typedRequest->getVolId(),
                typedRequest->perfNameStr);
    }
}

void QueryCatalogHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::QueryCatalogMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = e.GetErrno();

    // TODO(Rao): We should have a separate response message for QueryCatalogMsg for
    // consistency
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::QueryCatalogMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
