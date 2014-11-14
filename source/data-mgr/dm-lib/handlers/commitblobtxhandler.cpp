/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fds_error.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <PerfTrace.h>
#include <blob/BlobTypes.h>
#include <fds_types.h>
#include <fds_assert.h>

#include <functional>

namespace fds {
namespace dm {

CommitBlobTxHandler::CommitBlobTxHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CommitBlobTxMsg, handleRequest);
    }
}

void CommitBlobTxHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::CommitBlobTxMsg>& message) {
    if (dataMgr->testUturnAll) {
        LOGNOTIFY << "Uturn testing commit blob tx " << logString(*asyncHdr) << logString(*message);
        handleResponse(asyncHdr, message, ERR_OK, nullptr);
        return;
    }

    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    auto dmReq = new DmIoCommitBlobTx(message->volume_id,
                                      message->blob_name,
                                      message->blob_version,
                                      message->dmt_version);
    dmReq->cb = BIND_MSG_CALLBACK(CommitBlobTxHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(message->txId));

    addToQueue(dmReq);
}

void CommitBlobTxHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    DmIoCommitBlobTx* typedRequest = static_cast<DmIoCommitBlobTx*>(dmRequest);

    LOGTRACE << "Will commit blob " << typedRequest->blob_name << " to tvc";
    helper.err = dataMgr->timeVolCat_->commitBlobTx(typedRequest->volId,
                                                    typedRequest->blob_name,
                                                    typedRequest->ioBlobTxDesc,
                                                    // TODO(Rao): We should use a static commit
                                                    //            callback
                                                    std::bind(&CommitBlobTxHandler::volumeCatalogCb,
                                                              this,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2,
                                                              std::placeholders::_3,
                                                              std::placeholders::_4,
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
                                          DmIoCommitBlobTx* commitBlobReq) {
    QueueHelper helper(commitBlobReq);
    helper.err = e;

    LOGDEBUG << "Dmt version: " << commitBlobReq->dmt_version << " blob "
             << commitBlobReq->blob_name << " vol " << std::hex << commitBlobReq->volId << std::dec
             << " ; current DMT version " << dataMgr->omClient->getDMTVersion();

    // 'finish this io' for qos accounting purposes, if we are
    // forwarding, the main time goes to waiting for response
    // from another DM, which is not really consuming local
    // DM resources
    // FIXME(DAC): Is it okay that the caller isn't notified of errors after this point? Wouldn't
    //             it make more sense to wait to respond until after the request is successfully
    //             forwarded to another DM in the case that that's necessary?
    helper.markIoDone();

    // do forwarding if needed and commit was successful
    if (helper.err.ok() && commitBlobReq->dmt_version != dataMgr->omClient->getDMTVersion()) {
        VolumeMeta* vol_meta = nullptr;
        fds_bool_t is_forwarding = false;

        // check if we need to forward this commit to receiving
        // DM if we are in progress of migrating volume catalog
        dataMgr->vol_map_mtx->lock();
        fds_verify(dataMgr->vol_meta_map.count(commitBlobReq->volId) > 0);
        vol_meta = dataMgr->vol_meta_map[commitBlobReq->volId];
        is_forwarding = vol_meta->isForwarding();
        dataMgr->vol_map_mtx->unlock();

        // move the state, once we drain  planned queue contents
        LOGTRACE << "is_forwarding ? " << is_forwarding << " req DMT ver "
                 << commitBlobReq->dmt_version << " DMT ver " << dataMgr->omClient->getDMTVersion();

        if (is_forwarding) {
            // DMT version must not match in order to forward the update!!!
            if (commitBlobReq->dmt_version != dataMgr->omClient->getDMTVersion()) {
                if (dataMgr->feature.isCatSyncEnabled()) {
                    helper.err = dataMgr->catSyncMgr->forwardCatalogUpdate(commitBlobReq,
                                                                            blob_version,
                                                                            blob_obj_list,
                                                                            meta_list);
                } else {
                    LOGWARN << "catalog sync feature - NOT enabled";
                }
                if (helper.err.ok()) {
                    // we forwarded the request!!!
                    // if forwarding -- do not reply to AM yet, will reply when we receive response
                    // for fwd cat update from destination DM
                    // TODO(DAC): Actually sent the above mentioned response.
                    commitBlobReq->cb = NULL;
                }
            }
        } else {
            // DMT mismatch must not happen if volume is in 'not forwarding' state
            fds_verify(commitBlobReq->dmt_version != dataMgr->omClient->getDMTVersion());
        }
    }

    // check if we can finish forwarding if volume still forwards cat commits
    if (dataMgr->feature.isCatSyncEnabled() && dataMgr->catSyncMgr->isSyncInProgress()) {
        fds_bool_t is_finish_forward = false;
        VolumeMeta* vol_meta = nullptr;
        dataMgr->vol_map_mtx->lock();
        fds_verify(dataMgr->vol_meta_map.count(commitBlobReq->volId) > 0);
        vol_meta = dataMgr->vol_meta_map[commitBlobReq->volId];
        is_finish_forward = vol_meta->isForwardFinishing();
        dataMgr->vol_map_mtx->unlock();
        if (is_finish_forward) {
            if (!dataMgr->timeVolCat_->isPendingTx(commitBlobReq->volId,
                    dataMgr->catSyncMgr->dmtCloseTs())) {
                dataMgr->finishForwarding(commitBlobReq->volId);
            }
        }
    }
}

void CommitBlobTxHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::CommitBlobTxMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {
    /*
     * we are not sending any response  message  in this call, instead we
     * will send the status  on async header
     * TODO(sanjay)- we will have to add  call to  send the response without payload
     * static response
     */
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = e.GetErrno();
    // TODO(sanjay) - we will have to revisit  this call
    fpi::CommitBlobTxRspMsg stBlobTxRsp;
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CommitBlobTxRspMsgTypeId, stBlobTxRsp);

    if (dataMgr->testUturnAll) {
        fds_verify(dmRequest == nullptr);
    } else {
        delete dmRequest;
    }
}

}  // namespace dm
}  // namespace fds
