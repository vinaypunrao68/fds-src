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
}

void DMSvcHandler::queryCatalogObject(boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg)
{
    DBG(GLOGDEBUG << logString(*queryMsg));
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
        std::bind(&DMSvcHandler::queryCatalogObjectCb, this,
                  queryMsg,
                  std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3);

    Error err = dataMgr->qosCtrl->enqueueIO(dmQryReq->getVolId(),
                                      static_cast<FDS_IOType*>(dmQryReq));
    if (err != ERR_OK) {
        LOGNORMAL << "Unable to enqueue Query Catalog request "
                  << logString(queryMsg->hdr);
        dmQryReq->dmio_querycat_resp_cb(err, dmQryReq, nullptr);
    } else {
        LOGNORMAL << "Successfully enqueued  Catalog  request "
                  << logString(queryMsg->hdr);
    }
}

// TODO(Rao): Refactor dmCatReq to contain BlobNode information? Check with Andrew.
void DMSvcHandler::queryCatalogObjectCb(boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg,
                                        const Error &e, dmCatReq *req,
                                        BlobNode *bnode)
{
    DBG(GLOGDEBUG << logString(*queryMsg));

    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();
    queryMsg->hdr.msg_code = static_cast<int32_t>(e.GetErrno());
    if (bnode) {
        bnode->ToFdspPayload(queryMsg);
        delete bnode;
    }

    NetMgr::ep_mgr_singleton()->ep_send_async_resp(queryMsg->hdr, *queryMsg);

    delete req;
}

void DMSvcHandler::updateCatalog(boost::shared_ptr<fpi::UpdateCatalogMsg>& updcatMsg)
{
    DBG(GLOGDEBUG << logString(*updcatMsg));
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
        std::bind(&DMSvcHandler::updateCatalogCb, this,
                  updcatMsg,
                  std::placeholders::_1, std::placeholders::_2);

    Error err = dataMgr->qosCtrl->enqueueIO(dmUpdCatReq->getVolId(),
                                            static_cast<FDS_IOType*>(dmUpdCatReq));
    if (err != ERR_OK) {
        LOGNORMAL << "Unable to enqueue Update Catalog request "
                  << logString(updcatMsg->hdr);
        dmUpdCatReq->dmio_updatecat_resp_cb(err, dmUpdCatReq);
    } else {
        LOGNORMAL << "Successfully enqueued Update Catalog  request "
                  << logString(updcatMsg->hdr);
    }
}

void DMSvcHandler::updateCatalogCb(boost::shared_ptr<fpi::UpdateCatalogMsg>& updcatMsg,
                                        const Error &e, DmIoUpdateCat *req)
{
    DBG(GLOGDEBUG << logString(*updcatMsg));
    fpi::UpdateCatalogRspMsg updcatRspMsg;
    NetMgr::ep_mgr_singleton()->ep_send_async_resp(updcatMsg->hdr, updcatRspMsg);

    delete req;
}

}  // namespace fds
