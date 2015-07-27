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
    }

    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

	fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

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

    LOGDEBUG << "Will update blob: '" << request->blob_name << "' of volume: '" <<
            std::hex << request->volId << std::dec << "'";

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
