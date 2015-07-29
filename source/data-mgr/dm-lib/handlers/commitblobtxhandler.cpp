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

    addToQueue(dmReq);
}

void CommitBlobTxHandler::handleQueueItem(DmRequest* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoCommitBlobTx* typedRequest = static_cast<DmIoCommitBlobTx*>(dmRequest);

    LOGTRACE << "Will commit blob " << typedRequest->blob_name << " to tvc";
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
                                                     typedRequest));
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
                                          DmIoCommitBlobTx* commitBlobReq) {
    QueueHelper helper(dataManager, commitBlobReq);
    helper.err = e;
    if (!helper.err.ok()) {
        LOGWARN << "Failed to commit Tx for blob '" << commitBlobReq->blob_name << "'";
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
    // FIXME(DAC): Is it okay that the caller isn't notified of errors after this point? Wouldn't
    //             it make more sense to wait to respond until after the request is successfully
    //             forwarded to another DM in the case that that's necessary?
    helper.markIoDone();

    // do forwarding if needed and commit was successful
    if (commitBlobReq->dmt_version != MODULEPROVIDER()->getSvcMgr()->getDMTVersion()) {
        fds_volid_t volId(commitBlobReq->volId);
        fds_bool_t justOff(false);

        if (!(dataManager.features.isTestModeEnabled()) &&
        		(dataManager.dmMigrationMgr->shouldForwardIO(volId,
        													 commitBlobReq->dmt_version,
															 justOff))) {
            // DMT version must not match in order to forward the update!!!
            if (commitBlobReq->dmt_version != MODULEPROVIDER()->getSvcMgr()->getDMTVersion()) {
                LOGMIGRATE << "Forwarding request that used DMT " << commitBlobReq->dmt_version
                           << " because our DMT is " << MODULEPROVIDER()->getSvcMgr()->getDMTVersion();
                helper.err = dataManager.dmMigrationMgr->forwardCatalogUpdate(volId,
                															  commitBlobReq,
																			  blob_version,
																			  blob_obj_list,
																			  meta_list);
                if (helper.err.ok()) {
                    // we forwarded the request!!!
                    // if forwarding -- do not reply to AM yet, will reply when we receive response
                    // for fwd cat update from destination DM
                    // TODO(DAC): Actually sent the above mentioned response.
                    helper.skipImplicitCb = true;
                }
            }
        } else {
        	if (justOff) {
        		// Forwarding was just turned off. Send end forward message.
        		dataManager.dmMigrationMgr->sendNotifyFinishVolResync(volId);
        	}

            // DMT mismatch must not happen if volume is in 'not forwarding' state
            fds_verify(commitBlobReq->dmt_version != MODULEPROVIDER()->getSvcMgr()->getDMTVersion());
        }
    }
}

void CommitBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::CommitBlobTxMsg>& message,
                                         Error const& e, DmRequest* dmRequest) {
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = e.GetErrno();

    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CommitBlobTxRspMsgTypeId,
            static_cast<DmIoCommitBlobTx*>(dmRequest)->rspMsg);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
