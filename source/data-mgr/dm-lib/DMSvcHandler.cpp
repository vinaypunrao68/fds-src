/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <DMSvcHandler.h>
#include <platform/fds_flags.h>
#include <dm-platform.h>

namespace fds {

DMSvcHandler::DMSvcHandler()
{
    REGISTER_FDSP_MSG_HANDLER(fpi::QueryCatalogMsg, queryCatalogObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::UpdateCatalogMsg, updateCatalog);
    REGISTER_FDSP_MSG_HANDLER(fpi::StartBlobTxMsg, startBlobTx);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteCatalogObjectMsg, deleteCatalogObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::CommitBlobTxMsg, commitBlobTx);
    REGISTER_FDSP_MSG_HANDLER(fpi::AbortBlobTxMsg, abortBlobTx);
    REGISTER_FDSP_MSG_HANDLER(fpi::SetBlobMetaDataMsg, setBlobMetaData);
    REGISTER_FDSP_MSG_HANDLER(fpi::GetBlobMetaDataMsg, getBlobMetaData);
    REGISTER_FDSP_MSG_HANDLER(fpi::GetVolumeMetaDataMsg, getVolumeMetaData);
    /* DM to DM service messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::VolSyncStateMsg, volSyncState);
    REGISTER_FDSP_MSG_HANDLER(fpi::ForwardCatalogMsg, fwdCatalogUpdateMsg);
}


void DMSvcHandler::commitBlobTx(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               boost::shared_ptr<fpi::CommitBlobTxMsg>& commitBlbTx)
{
    LOGDEBUG << logString(*asyncHdr) << logString(*commitBlbTx);

    auto dmBlobTxReq = new DmIoCommitBlobTx(commitBlbTx->volume_id,
                                            commitBlbTx->blob_name,
                                            commitBlbTx->blob_version,
                                            commitBlbTx->dmt_version);
    dmBlobTxReq->dmio_commit_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::commitBlobTxCb, asyncHdr);

    PerfTracer::tracePointBegin(dmBlobTxReq->opReqLatencyCtx);

    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(commitBlbTx->txId));

    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue  commit blob tx  request "
            << logString(*asyncHdr) << logString(*commitBlbTx);
        PerfTracer::tracePointEnd(dmBlobTxReq->opReqLatencyCtx);
        PerfTracer::incr(dmBlobTxReq->opReqFailedPerfEventType, dmBlobTxReq->getVolId(),
                dmBlobTxReq->perfNameStr);
        dmBlobTxReq->dmio_commit_blob_tx_resp_cb(err, dmBlobTxReq);
    }
}

void DMSvcHandler::commitBlobTxCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   const Error &e, DmIoCommitBlobTx *req)
{
    /*
     * we are not sending any response  message  in this call, instead we 
     * will send the status  on async header
     * TODO(sanjay)- we will have to add  call to  send the response without payload 
     * static response
     */
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    // TODO(sanjay) - we will have to revisit  this call
    fpi::CommitBlobTxRspMsg stBlobTxRsp;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(CommitBlobTxRspMsg), stBlobTxRsp);

    delete req;
}


void DMSvcHandler::abortBlobTx(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               boost::shared_ptr<fpi::AbortBlobTxMsg>& abortBlbTx)
{
    LOGDEBUG << logString(*asyncHdr) << logString(*abortBlbTx);

    auto dmBlobTxReq = new DmIoAbortBlobTx(abortBlbTx->volume_id,
                                           abortBlbTx->blob_name,
                                           abortBlbTx->blob_version);
    dmBlobTxReq->dmio_abort_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::abortBlobTxCb, asyncHdr);

    PerfTracer::tracePointBegin(dmBlobTxReq->opReqLatencyCtx);

    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(abortBlbTx->txId));

    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue  abort blob tx  request "
            << logString(*asyncHdr) << logString(*abortBlbTx);
        PerfTracer::tracePointEnd(dmBlobTxReq->opReqLatencyCtx);
        PerfTracer::incr(dmBlobTxReq->opReqFailedPerfEventType, dmBlobTxReq->getVolId(),
                dmBlobTxReq->perfNameStr);
        dmBlobTxReq->dmio_abort_blob_tx_resp_cb(err, dmBlobTxReq);
    }
}

void DMSvcHandler::abortBlobTxCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   const Error &e, DmIoAbortBlobTx *req)
{
    /*
     * we are not sending any response  message  in this call, instead we 
     * will send the status  on async header
     * TODO(sanjay)- we will have to add  call to  send the response without payload 
     * static response
     */
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    // TODO(sanjay) - we will have to revisit  this call
    fpi::AbortBlobTxRspMsg stBlobTxRsp;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(AbortBlobTxRspMsg), stBlobTxRsp);

    delete req;
}

void DMSvcHandler::startBlobTx(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               boost::shared_ptr<fpi::StartBlobTxMsg>& startBlbTx)
{
    LOGDEBUG << logString(*asyncHdr) << logString(*startBlbTx);

    // TODO(xxx) implement uturn
    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnStartTx == true)) {
        LOGNOTIFY << "Uturn testing start blob tx";
        fds_panic("not implemented");
    }

    auto dmBlobTxReq = new DmIoStartBlobTx(startBlbTx->volume_id,
                                           startBlbTx->blob_name,
                                           startBlbTx->blob_version,
                                           startBlbTx->blob_mode,
                                           startBlbTx->dmt_version);
    dmBlobTxReq->dmio_start_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::startBlobTxCb, asyncHdr);

    PerfTracer::tracePointBegin(dmBlobTxReq->opReqLatencyCtx);
    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(startBlbTx->txId));


    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue start blob tx  request "
            << logString(*asyncHdr) << logString(*startBlbTx);
        PerfTracer::incr(dmBlobTxReq->opReqFailedPerfEventType, dmBlobTxReq->getVolId(),
                dmBlobTxReq->perfNameStr);
        PerfTracer::tracePointEnd(dmBlobTxReq->opReqLatencyCtx);
        dmBlobTxReq->dmio_start_blob_tx_resp_cb(err, dmBlobTxReq);
    }
}

void DMSvcHandler::startBlobTxCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   const Error &e, DmIoStartBlobTx *req)
{
    /*
     * we are not sending any response  message  in this call, instead we 
     * will send the status  on async header
     * TODO(sanjay)- we will have to add  call to  send the response without payload 
     * static response
     */
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    LOGDEBUG << "startBlobTx completed " << e << " " << logString(*asyncHdr);

    // TODO(sanjay) - we will have to revisit  this call
    fpi::StartBlobTxRspMsg stBlobTxRsp;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(StartBlobTxRspMsg), stBlobTxRsp);

    delete req;
}

void DMSvcHandler::queryCatalogObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*queryMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_queries > 0));
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto dmQryReq = new DmIoQueryCat(queryMsg);
    dmQryReq->dmio_querycat_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::queryCatalogObjectCb, asyncHdr, queryMsg);

    PerfTracer::tracePointBegin(dmQryReq->opReqLatencyCtx);

    Error err = dataMgr->qosCtrl->enqueueIO(dmQryReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmQryReq));

    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Query Catalog request "
                  << logString(*asyncHdr) << logString(*queryMsg);
        PerfTracer::tracePointEnd(dmQryReq->opReqLatencyCtx);
        PerfTracer::incr(dmQryReq->opReqFailedPerfEventType, dmQryReq->getVolId(),
                dmQryReq->perfNameStr);
        dmQryReq->dmio_querycat_resp_cb(err, dmQryReq);
    }
}

void DMSvcHandler::queryCatalogObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg,
                                        const Error &e,
                                        dmCatReq *req) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*queryMsg));

    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());

    // TODO(Rao): We should have a seperate response message for QueryCatalogMsg for
    // consistency
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), *queryMsg);

    delete req;
}

void DMSvcHandler::updateCatalog(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                 boost::shared_ptr<fpi::UpdateCatalogMsg>& updcatMsg)
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*updcatMsg));
    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_updates > 0));

    // TODO(xxx) implement uturn
    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnUpdateCat == true)) {
        GLOGNOTIFY << "Uturn testing update catalog";
        fds_panic("not implemented");
    }

    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto dmUpdCatReq = new DmIoUpdateCat(updcatMsg);

    dmUpdCatReq->dmio_updatecat_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::updateCatalogCb, asyncHdr);

    PerfTracer::tracePointBegin(dmUpdCatReq->opReqLatencyCtx);

    Error err = dataMgr->qosCtrl->enqueueIO(dmUpdCatReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmUpdCatReq));

    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Update Catalog request "
            << logString(*asyncHdr) << logString(*updcatMsg);
        PerfTracer::tracePointEnd(dmUpdCatReq->opReqLatencyCtx);
        PerfTracer::incr(dmUpdCatReq->opReqFailedPerfEventType, dmUpdCatReq->getVolId(),
                dmUpdCatReq->perfNameStr);
        dmUpdCatReq->dmio_updatecat_resp_cb(err, dmUpdCatReq);
    }
}

void DMSvcHandler::updateCatalogCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   const Error &e, DmIoUpdateCat *req)
{
    DBG(GLOGDEBUG << logString(*asyncHdr));
    fpi::UpdateCatalogRspMsg updcatRspMsg;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::UpdateCatalogRspMsg), updcatRspMsg);

    delete req;
}

void DMSvcHandler::deleteCatalogObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::DeleteCatalogObjectMsg>& delcatMsg)
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*delcatMsg));
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto dmDelCatReq = new DmIoDeleteCat(delcatMsg->volume_id,
                                         delcatMsg->blob_name,
                                         delcatMsg->blob_version);
    dmDelCatReq->dmio_deletecat_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::deleteCatalogObjectCb, asyncHdr);

    Error err = dataMgr->qosCtrl->enqueueIO(dmDelCatReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmDelCatReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Delete Catalog request "
            << logString(*asyncHdr) << logString(*delcatMsg);
        dmDelCatReq->dmio_deletecat_resp_cb(err, dmDelCatReq);
    }
}

void DMSvcHandler::deleteCatalogObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   const Error &e, DmIoDeleteCat *req)
{
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    // TODO(sanjay) - we will have to revisit  this call
    fpi::DeleteCatalogObjectRspMsg delcatRspMsg;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(DeleteCatalogObjectRspMsg), delcatRspMsg);

    delete req;
}

void DMSvcHandler::getBlobMetaData(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   boost::shared_ptr<fpi::GetBlobMetaDataMsg>& getBlbMeta) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*getBlbMeta));

    auto dmReq = new DmIoGetBlobMetaData(getBlbMeta->volume_id,
                                         getBlbMeta->blob_name,
                                         getBlbMeta->blob_version,
                                         getBlbMeta);
    dmReq->dmio_getmd_resp_cb =
                    BIND_MSG_CALLBACK2(DMSvcHandler::getBlobMetaDataCb, asyncHdr, getBlbMeta);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    Error err = dataMgr->qosCtrl->enqueueIO(dmReq->getVolId(),
                                      static_cast<FDS_IOType*>(dmReq));

    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request "
                << logString(*asyncHdr) << ":" << logString(*getBlbMeta);
        PerfTracer::tracePointEnd(dmReq->opReqLatencyCtx);
        PerfTracer::incr(dmReq->opReqFailedPerfEventType, dmReq->getVolId(),
                dmReq->perfNameStr);
        dmReq->dmio_getmd_resp_cb(err, dmReq);
    }
}

// TODO(Rao): Refactor dmCatReq to contain BlobNode information? Check with Andrew.
void DMSvcHandler::getBlobMetaDataCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::GetBlobMetaDataMsg>& message,
                                     const Error &e, DmIoGetBlobMetaData *req) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    // TODO(Rao): We should have a seperate response message for QueryCatalogMsg for
    // consistency
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    sendAsyncResp(asyncHdr, fpi::GetBlobMetaDataMsgTypeId, message);

    delete req;
}

void
DMSvcHandler::setBlobMetaData(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                              boost::shared_ptr<fpi::SetBlobMetaDataMsg>& setMDMsg)
{
    DBG(GLOGDEBUG << logString(*asyncHdr));  // << logString(*setMDMsg));

    // TODO(xxx) implement uturn
    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnSetMeta == true)) {
        GLOGNOTIFY << "Uturn testing set metadata";
        fds_panic("not implemented");
    }

    auto dmSetMDReq = new DmIoSetBlobMetaData(setMDMsg);

    dmSetMDReq->dmio_setmd_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::setBlobMetaDataCb, asyncHdr);

    PerfTracer::tracePointBegin(dmSetMDReq->opReqLatencyCtx);

    Error err = dataMgr->qosCtrl->enqueueIO(dmSetMDReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmSetMDReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue set metadata request "
                << logString(*asyncHdr);  // << logString(*setMDMsg);
        PerfTracer::tracePointEnd(dmSetMDReq->opReqLatencyCtx);
        PerfTracer::incr(dmSetMDReq->opReqFailedPerfEventType, dmSetMDReq->getVolId(),
                dmSetMDReq->perfNameStr);
        dmSetMDReq->dmio_setmd_resp_cb(err, dmSetMDReq);
    }
}

void
DMSvcHandler::setBlobMetaDataCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                const Error &e, DmIoSetBlobMetaData *req)
{
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    // TODO(sanjay) - we will have to revisit  this call
    fpi::SetBlobMetaDataRspMsg setBlobMetaDataRspMsg;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(SetBlobMetaDataRspMsg),
                  setBlobMetaDataRspMsg);

    delete req;
}

/**
 * Destination handler for receiving a VolsyncStateMsg (rsync has finished).
 *
 * @param[in] asyncHdr the async header sent with svc layer request
 * @param[in] fwdMsg the VolSyncState message
 * 
 */
void DMSvcHandler::volSyncState(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::VolSyncStateMsg>& syncStateMsg)
{
    Error err(ERR_OK);

    // synchronous call to process the volume sync state
    err = dataMgr->processVolSyncState(syncStateMsg->volume_id,
                                       syncStateMsg->forward_complete);

    asyncHdr->msg_code = err.GetErrno();
    fpi::VolSyncStateRspMsg volSyncStateRspMsg;
    // TODO(Brian): send a response here, make sure we've set the cb properly in the caller first
    // sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(VolSyncStateRspMsg), VolSyncStateRspMsg);
}

/**
 * Destination handler for receiving a ForwardCatalogMsg.
 *
 * @param[in] asyncHdr shared pointer reference to the async header
 * sent with svc layer requests 
 * @param[in] syncStateMsg shared pointer reference to the
 * ForwardCatalogMsg
 *
 */
void DMSvcHandler::fwdCatalogUpdateMsg(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                       boost::shared_ptr<fpi::ForwardCatalogMsg>& fwdCatMsg)
{
    Error err(ERR_OK);

    // Update catalog qos requeset but w/ different type
    auto dmFwdReq = new DmIoFwdCat(fwdCatMsg);
    // Bind CB
    dmFwdReq->dmio_fwdcat_resp_cb = BIND_MSG_CALLBACK2(DMSvcHandler::fwdCatalogUpdateCb,
                                                       asyncHdr, fwdCatMsg);
    // Enqueue to shadow queue
    err = dataMgr->catSyncRecv->enqueueFwdUpdate(dmFwdReq);
    if (!err.ok()) {
        LOGWARN << "Unable to enqueue Forward Update Catalog request "
                << logString(*asyncHdr) << " " << *dmFwdReq;
        dmFwdReq->dmio_fwdcat_resp_cb(err, dmFwdReq);
    }
}

/**
 * Callback for DmIoFwdCat request in fwdCatalogUpdate.
 *
 * @param[in] asyncHdr shared pointer reference to the async header
 * sent with svc layer requests 
 * @param[in] syncStateMsg shared pointer reference to the
 * ForwardCatalogMsg
 * @param[in] err Error code
 * @param[in] req The DmIoFwdCat request created by the caller
 *
 */
void DMSvcHandler::fwdCatalogUpdateCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::ForwardCatalogMsg>& fwdCatMsg,
                                      const Error &err, DmIoFwdCat *req)
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << " " << *req);
    // Set error value
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());

    // Send forwardCatalogUpdateRspMsg
    fpi::ForwardCatalogRspMsg fwdCatRspMsg;
    sendAsyncResp(asyncHdr,
                  FDSP_MSG_TYPEID(fpi::ForwardCatalogRspMsg),
                  fwdCatMsg);

    delete req;
}

void
DMSvcHandler::getVolumeMetaData(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::GetVolumeMetaDataMsg>& message) {
    // DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    auto dmReq = new DmIoGetVolumeMetaData(message);
    dmReq->dmio_get_volmd_resp_cb =
                    BIND_MSG_CALLBACK2(DMSvcHandler::getVolumeMetaDataCb, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    Error err = dataMgr->qosCtrl->enqueueIO(dmReq->getVolId(),
                                      static_cast<FDS_IOType*>(dmReq));

    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request "
                << logString(*asyncHdr) << ":" << logString(*message);
        PerfTracer::tracePointEnd(dmReq->opReqLatencyCtx);
        PerfTracer::incr(dmReq->opReqFailedPerfEventType, dmReq->getVolId(),
                dmReq->perfNameStr);
        dmReq->dmio_get_volmd_resp_cb(err, dmReq);
    }
}

void
DMSvcHandler::getVolumeMetaDataCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                    boost::shared_ptr<fpi::GetVolumeMetaDataMsg>& message,
                    const Error &e, DmIoGetVolumeMetaData *req) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    sendAsyncResp(asyncHdr, fpi::GetVolumeMetaDataMsgTypeId, message);
    delete req;
}

}  // namespace fds
