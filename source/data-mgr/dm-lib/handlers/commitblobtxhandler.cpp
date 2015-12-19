/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fds_error.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>
#include <blob/BlobTypes.h>
#include <fds_types.h>
#include <fds_assert.h>

#include <functional>

namespace fds {
namespace dm {

CommitBlobTxHandler::CommitBlobTxHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::CommitBlobTxMsg, handleRequest);
    }
}

void CommitBlobTxHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::CommitBlobTxMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    HANDLE_INVALID_TX_ID();

    // Handle U-turn
    HANDLE_U_TURN();

    fds_volid_t volId(message->volume_id);
    auto err = dataManager.validateVolumeIsActive(volId);
    if (!err.OK())
    {
        handleResponse(asyncHdr, message, err, nullptr);
        return;
    }

    HANDLE_FILTER_OLD_DMT_DURING_RESYNC();

    auto dmReq = new DmIoCommitBlobTx(volId,
                                      message->blob_name,
                                      message->blob_version,
                                      message->dmt_version,
                                      message->sequence_id);
    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmReq->cb = BIND_MSG_CALLBACK(CommitBlobTxHandler::handleResponse, asyncHdr, message);
    dmReq->ioBlobTxDesc = boost::make_shared<const BlobTxId>(message->txId);

	(static_cast<DmIoCommitBlobTx*>(dmReq))->localCb =
								std::bind(&CommitBlobTxHandler::handleResponseCleanUp,
								this,
								asyncHdr,
								message,
								std::placeholders::_1,
								dmReq);


    addToQueue(dmReq);
}

void CommitBlobTxHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoCommitBlobTx* typedRequest = static_cast<DmIoCommitBlobTx*>(dmRequest);

    ENSURE_IO_ORDER(typedRequest);

    LOGDEBUG << "Will commit blob " << *typedRequest;
    helper.err = dataManager
                .timeVolCat_->commitBlobTx(typedRequest->volId,
                                           typedRequest->blob_name,
                                           typedRequest->ioBlobTxDesc,
                                           // TODO(Rao): We should use a static commit
                                           //            callback
                                           typedRequest->sequence_id,
                                           std::bind(&CommitBlobTxHandler::volumeCatalogCb,
                                                     this,
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     std::placeholders::_3,
                                                     std::placeholders::_4,
                                                     std::placeholders::_5,
                                                     dmRequest));
    // Our callback, volumeCatalogCb(), will be called and will handle calling handleResponse().
    if (helper.err.ok()) {
        helper.cancel();
    }
}

/**
 * Callback from volume catalog when transaction is commited
 * @param[in] version of blob that was committed or in case of deletion,
 * a version that was deleted
 * @param[in] blob_obj_list list of offset to object mapping that
 * was committed or NULL
 * @param[in] meta_list list of metadata k-v pairs that was committed or NULL
 */
void CommitBlobTxHandler::volumeCatalogCb(Error const& e, blob_version_t blob_version,
                                          BlobObjList::const_ptr const& blob_obj_list,
                                          MetaDataList::const_ptr const& meta_list,
                                          fds_uint64_t const blobSize,
										  DmRequest* dmRequest) {
    DmIoCommitBlobTx* commitBlobReq = static_cast<DmIoCommitBlobTx*>(dmRequest);
    QueueHelper helper(dataManager, commitBlobReq);
    // If this is a piggy-back request, do not notify QoS
    if (!commitBlobReq->orig_request) {
        helper.ioIsMarkedAsDone = true;
    }
    helper.err = e;
    if (!helper.err.ok()) {
        LOGWARN << "Failed to commit Tx for blob '" << commitBlobReq->blob_name << "'";
        if (commitBlobReq->ioBlobTxDesc)
             LOGWARN   << " TxId:" << *(commitBlobReq->ioBlobTxDesc);
        return;
    }

    meta_list->toFdspPayload(commitBlobReq->rspMsg.meta_list);
    commitBlobReq->rspMsg.byteCount = blobSize;

    LOGDEBUG << "DMT version: " << commitBlobReq->dmt_version << " blob "
             << commitBlobReq->blob_name << " vol " << std::hex << commitBlobReq->volId << std::dec
             << " current DMT version " << MODULEPROVIDER()->getSvcMgr()->getDMTVersion();

    // 'finish this io' for qos accounting purposes, if we are
    // forwarding, the main time goes to waiting for response
    // from another DM, which is not really consuming local
    // DM resources
    helper.markIoDone();

    /**
     * There are 2 cases here:
     * 1.
     * Per AM design, it will ensure that when a new DMT version is published, it will first
     * finish all open transaction on the current, DMT before switching over to the new DMT.
     * Because of this, the new DM (executor) will not receive any active I/O.
     * 2.
     * In the case of a DM communication error, the AM reports the DM down to OM, the OM
     * then marks the DM inactive and publishes a new service map marking the DM as inactive.
     * It then publishes a new DMT version, where if the DM (executor) was a primary, demotes
     * it to a secondary. The resync then happens per case 1.
     * Once the Resync is completed, a new service map is then published (this may need to be
     * fixed due to the need for atomicity for new DMT + new service map in one shot).
     */
    // do forwarding if needed and commit was successful
    fds_volid_t volId(commitBlobReq->volId);
	if (!(dataManager.features.isTestModeEnabled()) &&
			(dataManager.dmMigrationMgr->shouldForwardIO(volId,
														 commitBlobReq->dmt_version))) {
		LOGMIGRATE << "Forwarding request that used DMT " << commitBlobReq->dmt_version
				   << " because our DMT is " << MODULEPROVIDER()->getSvcMgr()->getDMTVersion();

		commitBlobReq->usedForMigration = true;

        // Respond to AM before we forward
        helper.skipImplicitCb = true;
        commitBlobReq->cb(helper.err, commitBlobReq);

		helper.err = dataManager.dmMigrationMgr->forwardCatalogUpdate(volId,
																	  commitBlobReq,
																	  blob_version,
																	  blob_obj_list,
																	  meta_list);
	}
}

void CommitBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::CommitBlobTxMsg>& message,
                                         Error const& e, DmRequest* dmRequest) {
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = e.GetErrno();

    if (dmRequest) {
        // Sends reply to AM
        DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CommitBlobTxRspMsgTypeId,
                static_cast<DmIoCommitBlobTx*>(dmRequest)->rspMsg);

        if (!static_cast<DmIoCommitBlobTx*>(dmRequest)->usedForMigration) {
            delete dmRequest;
        }
    } else {
        DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CommitBlobTxRspMsgTypeId,
                           fpi::CommitBlobTxRspMsg());
    }
}

void CommitBlobTxHandler::handleResponseCleanUp(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         	 	boost::shared_ptr<fpi::CommitBlobTxMsg>& message,
												Error const& e, DmRequest* dmRequest) {
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
