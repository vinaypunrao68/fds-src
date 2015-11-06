/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <DmIoReq.h>

namespace fds {
namespace dm {

UpdateCatalogOnceHandler::UpdateCatalogOnceHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::UpdateCatalogOnceMsg, handleRequest);
    }
}

void UpdateCatalogOnceHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& message) {
    if (dataManager.testUturnAll || dataManager.testUturnUpdateCat) {
        GLOGDEBUG << "Uturn testing update catalog once " << logString(*asyncHdr)
                  << logString(*message);
        handleResponse(asyncHdr, message, ERR_OK, nullptr);
        return;
    }

    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    // Allocate a commit request structure because it is needed by the
    // commit call that will be executed during update processing.
    auto dmCommitBlobOnceReq = new DmIoCommitBlobOnce<DmIoUpdateCatOnce>(volId,
                                                                         message->blob_name,
                                                                         message->blob_version,
                                                                         message->dmt_version,
                                                                         message->sequence_id);
    dmCommitBlobOnceReq->cb =
            BIND_MSG_CALLBACK(UpdateCatalogOnceHandler::handleCommitBlobOnceResponse, asyncHdr);

    // allocate a new query cat log  class  and  queue  to per volume queue.
    auto dmUpdCatReq = new DmIoUpdateCatOnce(message, dmCommitBlobOnceReq);
    dmUpdCatReq->cb =
            BIND_MSG_CALLBACK(UpdateCatalogOnceHandler::handleResponse, asyncHdr, message);
    dmCommitBlobOnceReq->parent = dmUpdCatReq;
    dmCommitBlobOnceReq->ioBlobTxDesc = dmUpdCatReq->ioBlobTxDesc;

    (static_cast<DmIoCommitBlobTx*>(dmCommitBlobOnceReq))->localCb =
								std::bind(&UpdateCatalogOnceHandler::handleResponseCleanUp,
								this,
								asyncHdr,
								message,
								std::placeholders::_1,
								dmCommitBlobOnceReq);


    addToQueue(dmUpdCatReq);
}

void UpdateCatalogOnceHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, static_cast<DmIoUpdateCatOnce*>(dmRequest)->commitBlobReq);
    DmIoUpdateCatOnce* typedRequest = static_cast<DmIoUpdateCatOnce*>(dmRequest);

    // Start the transaction
    helper.err = dataManager.timeVolCat_->startBlobTx(typedRequest->volId,
                                                      typedRequest->blob_name,
                                                      typedRequest->updcatMsg->blob_mode,
                                                      typedRequest->ioBlobTxDesc,
                                                      typedRequest->updcatMsg->dmt_version);
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to start transaction" << typedRequest->ioBlobTxDesc << ": "
                 << helper.err;
        return;
    }

    // Apply the offset updates
    helper.err = dataManager.timeVolCat_->updateBlobTx(typedRequest->volId,
                                                       typedRequest->ioBlobTxDesc,
                                                       typedRequest->updcatMsg->obj_list);
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to update object offsets for transaction "
                 << *typedRequest->ioBlobTxDesc << ": " << helper.err;
        helper.err = dataManager.timeVolCat_->abortBlobTx(typedRequest->volId,
                                                          typedRequest->ioBlobTxDesc);
        if (!helper.err.ok()) {
            LOGERROR << "Failed to abort transaction " << *typedRequest->ioBlobTxDesc;
        }
        return;
    }

    // Apply the metadata updates
    helper.err = dataManager.timeVolCat_->updateBlobTx(typedRequest->volId,
                                                       typedRequest->ioBlobTxDesc,
                                                       typedRequest->updcatMsg->meta_list);
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to update metadata for transaction "
                 << *typedRequest->ioBlobTxDesc << ": " << helper.err;
        helper.err = dataManager.timeVolCat_->abortBlobTx(typedRequest->volId,
                                                          typedRequest->ioBlobTxDesc);
        if (!helper.err.ok()) {
            LOGERROR << "Failed to abort transaction " << *typedRequest->ioBlobTxDesc;
        }
        return;
    }

    // Commit the metadata updates
    // The commit callback we pass in will actually call the
    // final service callback
    helper.err = dataManager.timeVolCat_->commitBlobTx(
            typedRequest->volId, typedRequest->blob_name, typedRequest->ioBlobTxDesc,
            typedRequest->updcatMsg->sequence_id,
            std::bind(&CommitBlobTxHandler::volumeCatalogCb,
                      static_cast<CommitBlobTxHandler*>(dataManager.handlers[FDS_COMMIT_BLOB_TX]),
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
                      typedRequest->commitBlobReq));
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to commit transaction " << *typedRequest->ioBlobTxDesc << ": "
                 << helper.err;
        helper.err = dataManager.timeVolCat_->abortBlobTx(typedRequest->volId,
                                                          typedRequest->ioBlobTxDesc);
        if (!helper.err.ok()) {
            LOGERROR << "Failed to abort transaction " << typedRequest->ioBlobTxDesc;
        }
        return;
    } else {
        // We don't want to mark the I/O done or call handleResponse yet, work is now passed off to
        // CommitBlobTxHandler::volumeCatalogCb.
        helper.cancel();
    }
}

void UpdateCatalogOnceHandler::handleCommitBlobOnceResponse(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr, Error const& e, DmRequest* dmRequest) {
    auto commitOnceReq = static_cast<DmIoCommitBlobOnce<DmIoUpdateCatOnce>*>(dmRequest);
    DmIoUpdateCatOnce* parent = commitOnceReq->parent;
    parent->cb(e, dmRequest);
    delete parent;
}

void UpdateCatalogOnceHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& message,
                                              Error const& e, DmRequest* dmRequest) {
    asyncHdr->msg_code = e.GetErrno();
    DBG(GLOGDEBUG << logString(*asyncHdr));

    // Build response
    fpi::UpdateCatalogOnceRspMsg updcatRspMsg;
    if (dmRequest) {
        auto commitOnceReq = static_cast<DmIoCommitBlobOnce<DmIoUpdateCatOnce>*>(dmRequest);
        // Potential meta list corruption here... reopen FS-3355
        // commitOnceReq->dump_meta();
        updcatRspMsg.byteCount = commitOnceReq->rspMsg.byteCount;
        updcatRspMsg.meta_list.swap(commitOnceReq->rspMsg.meta_list);
    }

    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::UpdateCatalogOnceRspMsgTypeId, updcatRspMsg);

    if (dataManager.testUturnAll || dataManager.testUturnUpdateCat) {
        fds_verify(dmRequest == nullptr);
    } else {
         if (dmRequest && !static_cast<DmIoCommitBlobTx*>(dmRequest)->usedForMigration) {
             delete dmRequest;
         }
    }
}

void UpdateCatalogOnceHandler::handleResponseCleanUp(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                                     boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& message,
                                                     Error const& e, DmRequest* dmRequest) {
    DmIoCommitBlobTx* commitBlobReq = static_cast<DmIoCommitBlobTx*>(dmRequest);
    bool delete_req;

    {
        std::lock_guard<std::mutex> lock(commitBlobReq->migrClientCntMtx);
        fds_assert(commitBlobReq->migrClientCnt);
        commitBlobReq->migrClientCnt--;
        delete_req = commitBlobReq ? false : true; // delete if commitBlobReq == 0
    }

    if (delete_req) {
        delete dmRequest;
    }
}

}  // namespace dm
}  // namespace fds
