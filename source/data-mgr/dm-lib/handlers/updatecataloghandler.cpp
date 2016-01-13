/*
 * Copyright 2014 Formation Data Systems, Inc
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

UpdateCatalogHandler::UpdateCatalogHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::UpdateCatalogMsg, handleRequest);
    }
}

void UpdateCatalogHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::UpdateCatalogMsg> & message) {
    if (dataManager.testUturnAll || dataManager.testUturnUpdateCat) {
        GLOGDEBUG << "Uturn testing update catalog " << logString(*asyncHdr) <<
            logString(*message);
        handleResponse(asyncHdr, message, ERR_OK, NULL);
        return;
    }

    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

	fds_volid_t volId(message->volume_id);
    auto err = preEnqueueWriteOpHandling(volId, asyncHdr, PlatNetSvcHandler::threadLocalPayloadBuf);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    HANDLE_FILTER_OLD_DMT_DURING_RESYNC();

    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto request = new DmIoUpdateCat(message);

    request->cb = BIND_MSG_CALLBACK(UpdateCatalogHandler::handleResponse, asyncHdr, message);

    addToQueue(request);
}

void UpdateCatalogHandler::handleQueueItem(DmRequest * dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoUpdateCat * request = static_cast<DmIoUpdateCat *>(dmRequest);

    ENSURE_IO_ORDER(request, helper);

    LOGDEBUG << "Will update blob " << *request;

    helper.err = dataManager.timeVolCat_->updateBlobTx(request->volId,
                                                       request->ioBlobTxDesc,
                                                       request->obj_list);
}

void UpdateCatalogHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::UpdateCatalogMsg> & message, const Error &e,
        DmRequest *dmRequest) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::UpdateCatalogRspMsg),
            fpi::UpdateCatalogRspMsg());

    delete dmRequest;
}
}  // namespace dm
}  // namespace fds
