/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <DMSvcHandler.h>

namespace fds {

extern DataMgr *dataMgr;

DMSvcHandler::DMSvcHandler()
{
}

void DMSvcHandler::queryCatalogObject(boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg)
{
    /*
     * allocate a new query cat log  class and  queue  to per volume queue.
     */
    // TODO(Rao): Provide a constructor that doens't take unnecessary
    // params
    auto dmQryReq = new DataMgr::dmCatReq(queryMsg->volume_id, queryMsg->blob_name,
                                          queryMsg->dm_transaction_id,
                                          queryMsg->dm_operation, 0,
                                          0, 0, 0, "",
                                          0, FDS_CAT_QRY2, NULL);
    // Set the version
    // TODO(Andrew): Have a better constructor so that I can
    // set it that way.
    dmQryReq->setBlobVersion(queryMsg->blob_version);
    dmQryReq->resp_cb = std::bind(&DMSvcHandler::queryCatalogObjectCb, this, queryMsg,
                                  std::placeholders::_1, std::placeholders::_2,
                                  std::placeholders::_3);

    Error err = dataMgr->qosCtrl->enqueueIO(dmQryReq->getVolId(),
                                      static_cast<FDS_IOType*>(dmQryReq));
    if (err != ERR_OK) {
        LOGNORMAL << "Unable to enqueue Query Catalog request "
                  << logString(queryMsg->hdr);
        dmQryReq->resp_cb(err, dmQryReq, nullptr);
    }
    else {
        LOGNORMAL << "Successfully enqueued  Catalog  request "
                  << logString(queryMsg->hdr);
    }
}

// TODO(Rao): Refactor dmCatReq to contain BlobNode information? Check with Andrew.
void DMSvcHandler::queryCatalogObjectCb(boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg,
                                        const Error &e, DataMgr::dmCatReq *req,
                                        BlobNode *bnode)
{
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();
    queryMsg->hdr.msg_code = static_cast<int32_t>(e.GetErrno());
    if (bnode) {
        bnode->ToFdspPayload(queryMsg);
        delete bnode;
    }
    delete req;
    NetMgr::ep_mgr_singleton()->ep_send_async_resp(queryMsg->hdr, *queryMsg);
}

}  // namespace fds
