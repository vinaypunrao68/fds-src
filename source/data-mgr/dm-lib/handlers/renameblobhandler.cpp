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
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID_VAL(message->source_tx_id);

    HANDLE_U_TURN();

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
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    auto dmReq = new DmIoRenameBlob(volId,
                                    message->source_blob,
                                    message->sequence_id,
                                    message);
    dmReq->cb = BIND_MSG_CALLBACK(RenameBlobHandler::handleResponse, asyncHdr, message);

    addToQueue(dmReq);
}

void RenameBlobHandler::handleQueueItem(DmRequest* dmRequest) {
    auto typedRequest = static_cast<DmIoRenameBlob*>(dmRequest);
    auto& dmt_version = typedRequest->message->dmt_version;
    auto& dest_blob = typedRequest->message->destination_blob;
    auto source_tx = boost::make_shared<const BlobTxId>(typedRequest->message->source_tx_id);
    auto dest_tx = boost::make_shared<const BlobTxId>(typedRequest->message->destination_tx_id);

    // Mock a commit request
    typedRequest->commitReq = new DmIoCommitBlobOnce<DmIoRenameBlob>(typedRequest->volId,
                                                                     dest_blob,
                                                                     blob_version_invalid,
                                                                     dmt_version,
                                                                     typedRequest->seq_id);
    typedRequest->commitReq->cb = [this] (const Error& e, DmRequest* d) mutable -> void { handleCommitNewBlob(e, d); };

    (static_cast<DmIoCommitBlobTx*>(typedRequest->commitReq))->localCb =
        std::bind(&RenameBlobHandler::handleResponseCleanUp,
                  this,
                  std::placeholders::_1,
                  typedRequest->commitReq);

    typedRequest->commitReq->parent = typedRequest;
    typedRequest->commitReq->orig_request = false;
    QueueHelper helper(dataManager, typedRequest->commitReq);
    // This isn't the _real_ request, do not confuse QoS
    helper.ioIsMarkedAsDone = true;

    // Build our response and get a copy of the old blob's metadata
    typedRequest->response = boost::make_shared<fpi::RenameBlobRespMsg>();
    helper.err = dataManager.timeVolCat_->queryIface()->getBlobMeta(typedRequest->volId,
                                                                    typedRequest->blob_name,
                                                                    nullptr,
                                                                    reinterpret_cast<fds_uint64_t*>(&typedRequest->response->byteCount),
                                                                    &typedRequest->response->metaDataList);
    if (!helper.err.ok()) {
        LOGWARN << "Failed to get source blob '" << typedRequest->blob_name << "': "
                << helper.err;
        return;
    }

    // Retrieve a copy of the offset data
    fpi::FDSP_BlobObjectList obj_list;
    helper.err = dataManager.timeVolCat_->queryIface()->getBlob(typedRequest->volId,
                                                                typedRequest->blob_name,
                                                                0,
                                                                typedRequest->response->byteCount,
                                                                nullptr,
                                                                nullptr,
                                                                &obj_list,
                                                                reinterpret_cast<fds_uint64_t*>(&typedRequest->response->byteCount));

    // Start the transaction for the source blob
    helper.err = dataManager.timeVolCat_->startBlobTx(typedRequest->volId,
                                                      typedRequest->blob_name,
                                                      0,
                                                      source_tx,
                                                      dmt_version);
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to start transaction" << source_tx << ": "
                 << helper.err;
        return;
    }

    // Start the transaction for the destination blob
    helper.err = dataManager.timeVolCat_->startBlobTx(typedRequest->volId,
                                                      typedRequest->message->destination_blob,
                                                      blob::TRUNCATE,
                                                      dest_tx,
                                                      dmt_version);
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to start transaction" << dest_tx << ": "
                 << helper.err;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        return;
    }

    // Create the destination blob
    helper.err = dataManager.timeVolCat_->updateBlobTx(typedRequest->volId,
                                                       dest_tx,
                                                       typedRequest->response->metaDataList);

    if (!helper.err.ok()) {
        LOGWARN << "Failed to write new blob '" << typedRequest->message->destination_blob << "': "
                << helper.err;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, dest_tx);
        return;
    }

    // Apply the offset updates
    helper.err = dataManager.timeVolCat_->updateBlobTx(typedRequest->volId,
                                                       dest_tx,
                                                       obj_list);
    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to update object offsets for transaction "
                 << *dest_tx << ": " << helper.err;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, dest_tx);
        return;
    }

    // Commit the metadata updates
    // The commit callback we pass in will actually call the delete on the
    // source blob and commit the tx
    helper.err = dataManager.timeVolCat_->commitBlobTx(
        typedRequest->volId,
        typedRequest->message->destination_blob,
        dest_tx,
        typedRequest->seq_id,
        std::bind(&CommitBlobTxHandler::volumeCatalogCb,
                  static_cast<CommitBlobTxHandler*>(dataManager.handlers[FDS_COMMIT_BLOB_TX]),
                  std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
                  typedRequest->commitReq));

    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to commit transaction " << *dest_tx << ": "
                 << helper.err;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, dest_tx);
        return;
    } else {
        // We don't want to mark the I/O done or call handleResponse yet, work is now passed off to
        // CommitBlobTxHandler::volumeCatalogCb.
        helper.cancel();
    }
}

void RenameBlobHandler::handleCommitNewBlob(Error const& e, DmRequest* dmRequest) {
    auto commitOnceReq = static_cast<DmIoCommitBlobOnce<DmIoRenameBlob>*>(dmRequest);
    auto typedRequest = commitOnceReq->parent;
    auto source_tx = boost::make_shared<const BlobTxId>(typedRequest->message->source_tx_id);
    auto& dmt_version = typedRequest->message->dmt_version;

    if (!static_cast<DmIoCommitBlobTx*>(commitOnceReq)->usedForMigration) {
        delete commitOnceReq;
    }

    // Mock a commit request
    typedRequest->commitReq = new DmIoCommitBlobOnce<DmIoRenameBlob>(typedRequest->volId,
                                                                     typedRequest->blob_name,
                                                                     blob_version_invalid,
                                                                     dmt_version,
                                                                     typedRequest->seq_id);
    typedRequest->commitReq->cb = [this] (const Error& e, DmRequest* d) mutable -> void { handleDeleteOldBlob(e, d); };

    (static_cast<DmIoCommitBlobTx*>(typedRequest->commitReq))->localCb =
        std::bind(&RenameBlobHandler::handleResponseCleanUp,
                  this,
                  std::placeholders::_1,
                  typedRequest->commitReq);


    typedRequest->commitReq->parent = typedRequest;
    typedRequest->commitReq->orig_request = false;
    QueueHelper helper(dataManager, typedRequest->commitReq);
    helper.err = e;
    // This isn't the _real_ request, do not confuse QoS
    helper.ioIsMarkedAsDone = true;

    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to commit tx during rename...aborting: "
                 << e;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        return;
    }

    // Delete the source blob (but keep object references)
    helper.err = dataManager.timeVolCat_->deleteBlob(typedRequest->volId,
                                                     source_tx,
                                                     blob_version_invalid,
                                                     false);

    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to delete the source blob" << typedRequest->blob_name << ": "
                 << helper.err;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        return;
    }

    // Commit the delete
    helper.err = dataManager.timeVolCat_->commitBlobTx(
        typedRequest->volId,
        typedRequest->blob_name,
        source_tx,
        typedRequest->seq_id,
        std::bind(&CommitBlobTxHandler::volumeCatalogCb,
                  static_cast<CommitBlobTxHandler*>(dataManager.handlers[FDS_COMMIT_BLOB_TX]),
                  std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3, std::placeholders::_4, std::placeholders::_5,
                  typedRequest->commitReq));

    if (helper.err != ERR_OK) {
        LOGERROR << "Failed to commit transaction " << *source_tx << ": "
                 << helper.err;
        dataManager.timeVolCat_->abortBlobTx(typedRequest->volId, source_tx);
        return;
    } else {
        // We don't want to mark the I/O done or call handleResponse yet, work is now passed off to
        // CommitBlobTxHandler::volumeCatalogCb.
        helper.cancel();
    }
}

void RenameBlobHandler::handleDeleteOldBlob(Error const& e, DmRequest* dmRequest) {
    auto commitOnceReq = static_cast<DmIoCommitBlobOnce<DmIoRenameBlob>*>(dmRequest);
    DmIoRenameBlob* parent = commitOnceReq->parent;

    if (!static_cast<DmIoCommitBlobTx*>(commitOnceReq)->usedForMigration) {
        delete commitOnceReq;
    }

    QueueHelper helper(dataManager, parent);
    helper.err = e;
}

void RenameBlobHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::RenameBlobMsg>& message,
                                       Error const& e, DmRequest* dmRequest) {
    if (dmRequest) {
        asyncHdr->msg_code = e.GetErrno();
        auto renameReq = static_cast<DmIoRenameBlob*>(dmRequest);
        auto response = renameReq->response ?
            renameReq->response : boost::make_shared<fpi::RenameBlobRespMsg>();
        DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*response));
        DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::RenameBlobRespMsg), *response);
    } else {
        DM_SEND_ASYNC_RESP(*asyncHdr, FDSP_MSG_TYPEID(fpi::RenameBlobRespMsg),
                           fpi::RenameBlobRespMsg());
    }

    delete dmRequest;
}

void RenameBlobHandler::handleResponseCleanUp(Error const& e, DmRequest* dmRequest) {
    DmIoCommitBlobTx* commitBlobReq = static_cast<DmIoCommitBlobTx*>(dmRequest);
    bool delete_req;

    {
        std::lock_guard<std::mutex> lock(commitBlobReq->migrClientCntMtx);
        fds_assert(commitBlobReq->migrClientCnt);
        commitBlobReq->migrClientCnt--;
        delete_req = commitBlobReq->migrClientCnt ? false : true; // delete if commitBlobReq == 0
    }

    if (delete_req) {
        delete dmRequest;
    }
}

}  // namespace dm
}  // namespace fds
