/*
 * Copyright 2014 Formation Data Systems, Inc
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

UpdateCatalogHandler::UpdateCatalogHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::UpdateCatalogMsg, handleRequest);
    }
}

void UpdateCatalogHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::UpdateCatalogMsg> & message) {
    if ((dataMgr->testUturnAll == true) || (dataMgr->testUturnUpdateCat == true)) {
        GLOGDEBUG << "Uturn testing update catalog " << logString(*asyncHdr) <<
            logString(*message);
        handleResponse(asyncHdr, message, ERR_OK, NULL);
    }

    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));
    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_updates > 0));

    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto request = new DmIoUpdateCat(message);

    request->cb = BIND_MSG_CALLBACK(UpdateCatalogHandler::handleResponse, asyncHdr, message);

    /*
     * TODO(umesh): ignore for now, uncomment it later
    PerfTracer::tracePointBegin(request->opReqLatencyCtx);
     */
    addToQueue(request);
}

void UpdateCatalogHandler::handleQueueItem(dmCatReq * dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoUpdateCat * request = static_cast<DmIoUpdateCat *>(dmRequest);

    LOGDEBUG << "Will update blob: '" << request->blob_name << "' of volume: '" <<
            std::hex << request->volId << std::dec << "'";

    helper.err = dataMgr->timeVolCat_->updateBlobTx(request->volId, request->ioBlobTxDesc,
            request->obj_list);
}

void UpdateCatalogHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::UpdateCatalogMsg> & message, const Error &e,
        dmCatReq *dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::UpdateCatalogRspMsg),
            fpi::UpdateCatalogRspMsg());
    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
