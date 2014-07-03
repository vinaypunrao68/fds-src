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
}


void DMSvcHandler::startBlobTx(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               boost::shared_ptr<fpi::StartBlobTxMsg>& startBlbTx)
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*startBlbTx));

    auto dmBlobTxReq = new DmIoStartBlobTx(startBlbTx->volume_id,
                                           startBlbTx->blob_name,
                                           startBlbTx->blob_version);
    dmBlobTxReq->dmio_start_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::startBlobTxCb, asyncHdr);
            /*
        std::bind(&DMSvcHandler::startBlobTxCb, this,
                  asyncHdr,
                  std::placeholders::_1, std::placeholders::_2);
            */

    Error err = dataMgr->qosCtrl->enqueueIO(dmBlobTxReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmBlobTxReq));
    /*
     * allocate a new  Blob transaction  class and  queue  to per volume queue.
     */
    dmBlobTxReq->ioBlobTxDesc = BlobTxId::ptr(new BlobTxId(startBlbTx->txId));
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
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    // TODO(sanjay) - we will have to revisit  this call
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(StartBlobTxRspMsg), *asyncHdr);

    delete req;
}

void DMSvcHandler::queryCatalogObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg)
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*queryMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_queries > 0));
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    auto dmQryReq = new DmIoQueryCat(queryMsg->volume_id,
                                     queryMsg->blob_name,
                                     queryMsg->blob_version);
    // Set the version
    // TODO(Andrew): Have a better constructor so that I can
    // set it that way.
    dmQryReq->setBlobVersion(queryMsg->blob_version);
    dmQryReq->dmio_querycat_resp_cb =
            BIND_MSG_CALLBACK3(DMSvcHandler::queryCatalogObjectCb, asyncHdr, queryMsg);
    /*
        std::bind(&DMSvcHandler::queryCatalogObjectCb, this,
                  asyncHdr,
                  queryMsg,
                  std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3);
    */

    Error err = dataMgr->qosCtrl->enqueueIO(dmQryReq->getVolId(),
                                      static_cast<FDS_IOType*>(dmQryReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Query Catalog request "
                  << logString(*asyncHdr) << logString(*queryMsg);
        dmQryReq->dmio_querycat_resp_cb(err, dmQryReq, nullptr);
    }
}

// TODO(Rao): Refactor dmCatReq to contain BlobNode information? Check with Andrew.
void DMSvcHandler::queryCatalogObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg,
                                        const Error &e, dmCatReq *req,
                                        BlobNode *bnode)
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*queryMsg));

    // TODO(Rao): We should have a seperate response message for QueryCatalogMsg for
    // consistency
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    if (bnode) {
        bnode->ToFdspPayload(queryMsg);
        delete bnode;
    }

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
    dmUpdCatReq->obj_list = std::move(updcatMsg->obj_list);
    dmUpdCatReq->dmio_updatecat_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::updateCatalogCb, asyncHdr);
    /*
        std::bind(&DMSvcHandler::updateCatalogCb, this,
                  asyncHdr,
                  std::placeholders::_1, std::placeholders::_2);
    */

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
#if 0
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
    /*
        std::bind(&DMSvcHandler::deleteCatalogObjectCb, this,
                  asyncHdr,
                  std::placeholders::_1, std::placeholders::_2);
    */
    Error err = dataMgr->qosCtrl->enqueueIO(dmDelCatReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmDelCatReq));
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue Delete Catalog request "
            << logString(*asyncHdr) << logString(*delcatMsg);
        dmUpdCatReq->dmio_deletecat_resp_cb(err, dmDelCatReq);
    }
}

void DMSvcHandler::deleteCatalogCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                   const Error &e, DmIoDeleteCat *req)
{
    DBG(GLOGDEBUG << logString(*asyncHdr));
    // fpi::DeleteCatalogRspMsg updcatRspMsg;
    // net::ep_send_async_resp(*asyncHdr, updcatRspMsg);

    delete req;
}
#endif
}  // namespace fds
