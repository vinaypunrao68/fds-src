/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>

namespace fds {
namespace dm {

RenameBlobHandler::RenameBlobHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::RenameBlobMsg, handleRequest);
    }
}

void RenameBlobHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                           boost::shared_ptr<fpi::RenameBlobMsg>& message) {
    auto response = boost::make_shared<fpi::RenameBlobRespMsg>();
    if (dataManager.testUturnAll) {
        GLOGDEBUG << "Uturn testing rename blob " << logString(*asyncHdr) <<
            logString(*message);
        handleResponse(asyncHdr, response, ERR_OK, NULL);
    }
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));


    fds_volid_t volId(message->volume_id);
    Error err(ERR_OK);
    if (!dataManager.amIPrimaryGroup(volId)) {
    	err = ERR_DM_NOT_PRIMARY;
    }
    if (err.OK()) {
    	err = dataManager.validateVolumeIsActive(volId);
    }

    if (!err.OK())
    {
        handleResponse(asyncHdr, response, err, nullptr);
        return;
    }

    auto dmReq = new DmIoRenameBlob(volId,
                                    message->source_blob,
                                    message->destination_blob,
                                    message->sequence_id,
                                    response);
    dmReq->cb = BIND_MSG_CALLBACK(RenameBlobHandler::handleResponse, asyncHdr, response);

    addToQueue(dmReq);
}

void RenameBlobHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoRenameBlob* typedRequest = static_cast<DmIoRenameBlob*>(dmRequest);
    auto renameRequest = static_cast<DmIoRenameBlob*>(dmRequest);

    fds_uint64_t blobSize {0};
    helper.err = dataManager
                .timeVolCat_
               ->renameBlob(typedRequest->volId,
                            typedRequest->blob_name,
                            typedRequest->new_blob_name,
                            typedRequest->seq_id,
                            std::bind(&RenameBlobHandler::renameBlobCb,
                                      this,
                                      std::placeholders::_1,
                                      renameRequest,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      std::placeholders::_4,
                                      std::placeholders::_5));

    // Our callback, renameBlobCb(), will be called and will handle calling handleResponse().
    if (helper.err.ok()) {
        helper.cancel();
    }
}

void RenameBlobHandler::renameBlobCb(Error const& e,
                                     DmIoRenameBlob* typedRequest,
                                     blob_version_t blob_version,
                                     BlobObjList::const_ptr const& blob_obj_list,
                                     MetaDataList::const_ptr const& meta_list,
                                     fds_uint64_t const blobSize) {
    QueueHelper helper(dataManager, typedRequest);
    helper.err = e;
    if (!helper.err.ok()) {
        LOGWARN << "Failed to rename blob '" << typedRequest->blob_name << "'";
        return;
    }

    meta_list->toFdspPayload(typedRequest->message->metaDataList);
    typedRequest->message->byteCount = blobSize;

    helper.markIoDone();
}


void RenameBlobHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                            boost::shared_ptr<fpi::RenameBlobRespMsg>& message,
                                            Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::RenameBlobRespMsg), *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
