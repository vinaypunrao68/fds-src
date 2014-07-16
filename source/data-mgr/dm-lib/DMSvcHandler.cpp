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

extern DataMgr *dataMgr;

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
}


void DMSvcHandler::commitBlobTx(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               boost::shared_ptr<fpi::CommitBlobTxMsg>& commitBlbTx)
{
    LOGDEBUG << logString(*asyncHdr) << logString(*commitBlbTx);

    auto dmBlobTxReq = new DmIoCommitBlobTx(commitBlbTx->volume_id,
                                           commitBlbTx->blob_name,
                                           commitBlbTx->blob_version,
                                           commitBlbTx->blobEnd);
    dmBlobTxReq->dmio_commit_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::commitBlobTxCb, asyncHdr);

    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(commitBlbTx->txId));

    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue  commit blob tx  request "
            << logString(*asyncHdr) << logString(*commitBlbTx);
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

    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(abortBlbTx->txId));

    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue  abort blob tx  request "
            << logString(*asyncHdr) << logString(*abortBlbTx);
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

    auto dmBlobTxReq = new DmIoStartBlobTx(startBlbTx->volume_id,
                                           startBlbTx->blob_name,
                                           startBlbTx->blob_version);
    dmBlobTxReq->dmio_start_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::startBlobTxCb, asyncHdr);

    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(startBlbTx->txId));


    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue  start blob tx  request "
            << logString(*asyncHdr) << logString(*startBlbTx);
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
    LOGDEBUG << logString(*asyncHdr);
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
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

    Error err = dataMgr->qosCtrl->enqueueIO(dmQryReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmQryReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Query Catalog request "
                  << logString(*asyncHdr) << logString(*queryMsg);
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
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto dmUpdCatReq = new DmIoUpdateCat(updcatMsg->volume_id,
                                         updcatMsg->blob_name,
                                         updcatMsg->blob_version);
    dmUpdCatReq->ioBlobTxDesc = boost::make_shared<const BlobTxId>(updcatMsg->txId);
    dmUpdCatReq->obj_list = std::move(updcatMsg->obj_list);
    dmUpdCatReq->dmio_updatecat_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::updateCatalogCb, asyncHdr);

    Error err = dataMgr->qosCtrl->enqueueIO(dmUpdCatReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmUpdCatReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Update Catalog request "
            << logString(*asyncHdr) << logString(*updcatMsg);
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

    Error err = dataMgr->qosCtrl->enqueueIO(dmReq->getVolId(),
                                      static_cast<FDS_IOType*>(dmReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request "
                << logString(*asyncHdr) << ":" << logString(*getBlbMeta);
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

    auto dmSetMDReq = new DmIoSetBlobMetaData(setMDMsg->volume_id,
                                              setMDMsg->blob_name,
                                              setMDMsg->blob_version);

    dmSetMDReq->md_list = std::move(setMDMsg->metaDataList);
    dmSetMDReq->ioBlobTxDesc = boost::make_shared<const BlobTxId>(setMDMsg->txId);

    dmSetMDReq->dmio_setmd_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::setBlobMetaDataCb, asyncHdr);

    Error err = dataMgr->qosCtrl->enqueueIO(dmSetMDReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmSetMDReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue set metadata request "
                << logString(*asyncHdr);  // << logString(*setMDMsg);
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
}  // namespace fds
