/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <DMSvcHandler.h>
#include <platform/fds_flags.h>
#include <dm-platform.h>
#include <StatStreamAggregator.h>

namespace fds {

DMSvcHandler::DMSvcHandler()
{
    REGISTER_FDSP_MSG_HANDLER(fpi::QueryCatalogMsg, queryCatalogObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::UpdateCatalogMsg, updateCatalog);
    REGISTER_FDSP_MSG_HANDLER(fpi::UpdateCatalogOnceMsg, updateCatalogOnce);
    REGISTER_FDSP_MSG_HANDLER(fpi::StartBlobTxMsg, startBlobTx);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteCatalogObjectMsg, deleteCatalogObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::CommitBlobTxMsg, commitBlobTx);
    REGISTER_FDSP_MSG_HANDLER(fpi::AbortBlobTxMsg, abortBlobTx);
    REGISTER_FDSP_MSG_HANDLER(fpi::SetBlobMetaDataMsg, setBlobMetaData);
    REGISTER_FDSP_MSG_HANDLER(fpi::GetBlobMetaDataMsg, getBlobMetaData);
    REGISTER_FDSP_MSG_HANDLER(fpi::GetVolumeMetaDataMsg, getVolumeMetaData);
    REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamMsg, handleStatStream);
    REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamRegistrationMsg, registerStreaming);
    REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamDeregistrationMsg, deregisterStreaming);
    /* DM to DM service messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::VolSyncStateMsg, volSyncState);
    REGISTER_FDSP_MSG_HANDLER(fpi::ForwardCatalogMsg, fwdCatalogUpdateMsg);
    /* OM to DM snapshot messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CreateSnapshotMsg, createSnapshot);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteSnapshotMsg, deleteSnapshot);
    REGISTER_FDSP_MSG_HANDLER(fpi::CreateVolumeCloneMsg, createVolumeClone);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, NotifyAddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, NotifyRmVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
#if 0
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTClose, NotifyDMTClose);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTUpdate, NotifyDMTUpdate);
#endif
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTUpdate, NotifyDLTUpdate);
}

// notifySvcChange
// ---------------
//
void
DMSvcHandler::notifySvcChange(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                              boost::shared_ptr<fpi::NodeSvcInfo> &msg)
{
#if 0
    DomainAgent::pointer self;

    self = NetPlatform::nplat_singleton()->nplat_self();
    fds_verify(self != NULL);

    auto list = msg->node_svc_list;
    for (auto it = list.cbegin(); it != list.cend(); it++) {
        auto rec = *it;
        if (rec.svc_type == fpi::FDSP_DATA_MGR) {
            self->agent_fsm_input(msg, rec.svc_type,
                                  rec.svc_runtime_state, rec.svc_deployment_state);
        }
    }
#endif
}

// NotifyAddVol
// ------------
//
void
DMSvcHandler::NotifyAddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg)
{
    DBG(GLOGDEBUG << logString(*hdr) << "Vol Add");  // logString(*vol_msg));
    fds_verify(vol_msg->__isset.vol_desc);
    uint64_t vol_uuid = vol_msg->vol_desc.volUUID;

    fds_volid_t volumeId = vol_msg->vol_desc.volUUID;
    VolumeDesc vdb(vol_msg->vol_desc);
    GLOGNOTIFY << "Received create for vol "
               << "[" << std::hex << volumeId << std::dec << ", "
               << vdb.getName() << "]";

    Error err(ERR_OK);
    VolumeDesc desc(vol_msg->vol_desc);
    err = dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                    std::to_string(vol_uuid),
                                    vol_uuid, &desc,
                                    vol_msg->vol_flag == fpi::FDSP_NOTIFY_VOL_WILL_SYNC);
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolAdd), *vol_msg);
}

// NotifyRmVol
// -----------
//
void
DMSvcHandler::NotifyRmVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                          boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg)
{
    DBG(GLOGDEBUG << logString(*hdr) << "Vol Remove");  // logString(*vol_msg));
    fds_verify(vol_msg->__isset.vol_desc);
    uint64_t vol_uuid = vol_msg->vol_desc.volUUID;
    Error err(ERR_OK);
    bool fCheck = (vol_msg->vol_flag == fpi::FDSP_NOTIFY_VOL_CHECK_ONLY);

    if (vol_msg->vol_desc.fSnapshot) {
        err =  ERR_NOT_IMPLEMENTED;
    } else {
        if (fCheck) {
            // delete the volume blobs first
            err = dataMgr->deleteVolumeContents(vol_uuid);
            err = dataMgr->process_rm_vol(vol_uuid, fCheck);
        } else {
            err = dataMgr->process_rm_vol(vol_uuid, fCheck);
        }
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolRemove), *vol_msg);
}

// ------------
//
void
DMSvcHandler::NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg)
{
    DBG(GLOGDEBUG << logString(*hdr) << "vol modify");  //  logString(*vol_msg));
    fds_verify(vol_msg->__isset.vol_desc);
    uint64_t vol_uuid = vol_msg->vol_desc.volUUID;
    Error err(ERR_OK);
    VolumeDesc desc(vol_msg->vol_desc);
    err = dataMgr->_process_mod_vol(vol_uuid, desc);
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolMod), *vol_msg);
}



void DMSvcHandler::createSnapshot(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  boost::shared_ptr<fpi::CreateSnapshotMsg>& createSnapshot)
{
    Error err(ERR_OK);
    fds_assert(createSnapshot);

    /*
     * get the snapshot manager instanace
     * invoke the createSnapshot DM function
     */
    // TODO(Sanjay) revisit  this when we enable the server layer interface
    // err = dataMgr->createSnapshot(createSnapshot->snapshot);

    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    fpi::CreateSnapshotRespMsg createSnapshotResp;
    /*
     * init the response message with  snapshot id
     */
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::CreateSnapshotRespMsg), createSnapshotResp);
}

void DMSvcHandler::deleteSnapshot(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  boost::shared_ptr<fpi::DeleteSnapshotMsg>& deleteSnapshot)
{
    Error err(ERR_OK);
    fds_assert(deleteSnapshot);

    /*
     * get the snapshot manager instanace
     * invoke the deleteSnapshot DM function
     */
    err = dataMgr->process_rm_vol(deleteSnapshot->snapshotId, true);
    if (err.ok()) {
        err = dataMgr->process_rm_vol(deleteSnapshot->snapshotId, false);
    }

    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    fpi::DeleteSnapshotRespMsg deleteSnapshotResp;
    /*
     * init the response message with  snapshot id
     */
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::DeleteSnapshotMsg), deleteSnapshotResp);
}

void DMSvcHandler::createVolumeClone(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::CreateVolumeCloneMsg>& createClone)
{
    Error err(ERR_OK);
    fds_assert(createClone);

    /*
     * get the snapshot manager instanace
     * invoke the createClone DM function
     */
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    fpi::CreateVolumeCloneRespMsg createVolumeCloneResp;
    /*
     * init the response message with  Clone id
     */
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::CreateVolumeCloneRespMsg),
                  createVolumeCloneResp);
}

void DMSvcHandler::commitBlobTx(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::CommitBlobTxMsg>& commitBlbTx)
{
    if (dataMgr->testUturnAll == true) {
        LOGNOTIFY << "Uturn testing commit blob tx "
                  << logString(*asyncHdr) << logString(*commitBlbTx);
        commitBlobTxCb(asyncHdr, ERR_OK, NULL);
    }

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

    if (dataMgr->testUturnAll == true) {
        fds_verify(req == NULL);
        return;
    }

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
    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnStartTx == true)) {
        LOGNOTIFY << "Uturn testing start blob tx "
                  << logString(*asyncHdr) << logString(*startBlbTx);
        startBlobTxCb(asyncHdr, ERR_OK, NULL);
    }

    LOGDEBUG << logString(*asyncHdr) << logString(*startBlbTx);

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

    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnStartTx == true)) {
        fds_verify(req == NULL);
        return;
    }

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

    const VolumeDesc * voldesc = dataMgr->getVolumeDesc(dmQryReq->getVolId());
    Error err = dataMgr->qosCtrl->enqueueIO(voldesc && voldesc->isSnapshot() ?
                                            voldesc->qosQueueId : dmQryReq->getVolId(),
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

void
DMSvcHandler::updateCatalogOnce(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::UpdateCatalogOnceMsg>& updcatMsg)
{
    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnUpdateCat == true)) {
        GLOGDEBUG << "Uturn testing update catalog once "
                  << logString(*asyncHdr) << logString(*updcatMsg);
        updateCatalogOnceCb(asyncHdr, ERR_OK, NULL);
        return;
    }

    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*updcatMsg));
    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_updates > 0));

    // Allocate a commit request structure because it is needed by the
    // commit call that will be executed during update processing.
    auto dmCommitBlobOnceReq = new DmIoCommitBlobOnce(updcatMsg->volume_id,
                                                      updcatMsg->blob_name,
                                                      updcatMsg->blob_version,
                                                      updcatMsg->dmt_version);
    dmCommitBlobOnceReq->dmio_commit_blob_tx_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::commitBlobOnceCb, asyncHdr);
    PerfTracer::tracePointBegin(dmCommitBlobOnceReq->opReqLatencyCtx);

    // allocate a new query cat log  class and  queue  to per volume queue.
    auto dmUpdCatReq = new DmIoUpdateCatOnce(updcatMsg, dmCommitBlobOnceReq);
    dmUpdCatReq->dmio_updatecat_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::updateCatalogOnceCb, asyncHdr);
    dmCommitBlobOnceReq->parent = dmUpdCatReq;
    PerfTracer::tracePointBegin(dmUpdCatReq->opReqLatencyCtx);

    Error err = dataMgr->qosCtrl->enqueueIO(
        dmUpdCatReq->getVolId(),
        static_cast<FDS_IOType*>(dmUpdCatReq));
    fds_verify(err == ERR_OK);
}

void DMSvcHandler::updateCatalog(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                 boost::shared_ptr<fpi::UpdateCatalogMsg>& updcatMsg)
{
    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnUpdateCat == true)) {
        GLOGDEBUG << "Uturn testing update catalog "
                  << logString(*asyncHdr) << logString(*updcatMsg);
        updateCatalogCb(asyncHdr, ERR_OK, NULL);
    }

    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*updcatMsg));
    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(dm_drop_cat_updates > 0));

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

    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnUpdateCat == true)) {
        fds_verify(req == NULL);
        return;
    }

    delete req;
}

void
DMSvcHandler::commitBlobOnceCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &e,
                               DmIoCommitBlobTx *req) {
    DmIoCommitBlobOnce *commitOnceReq = static_cast<DmIoCommitBlobOnce*>(req);
    DmIoUpdateCatOnce *parent = commitOnceReq->parent;
    updateCatalogOnceCb(asyncHdr, e, parent);
    delete commitOnceReq;
}

void
DMSvcHandler::updateCatalogOnceCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  const Error &e,
                                  DmIoUpdateCatOnce *req) {
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    fpi::UpdateCatalogOnceRspMsg updcatRspMsg;
    sendAsyncResp(*asyncHdr,
                  FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceRspMsg),
                  updcatRspMsg);

    if ((dataMgr->testUturnAll == true) ||
        (dataMgr->testUturnUpdateCat == true)) {
        fds_verify(req == NULL);
        return;
    }

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

    const VolumeDesc * voldesc = dataMgr->getVolumeDesc(dmReq->getVolId());
    Error err = dataMgr->qosCtrl->enqueueIO(voldesc && voldesc->isSnapshot() ?
                                            voldesc->qosQueueId : dmReq->getVolId(),
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
    DBG(GLOGDEBUG << logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    fpi::SetBlobMetaDataRspMsg setBlobMetaRsp;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(SetBlobMetaDataRspMsg),
                  setBlobMetaRsp);
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

    LOGNORMAL << "get vol meta data msg ";
    // return getVolumeMetaDataCb(asyncHdr, message, Error(ERR_OK), nullptr);
    auto dmReq = new DmIoGetVolumeMetaData(message);
    dmReq->dmio_get_volmd_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::getVolumeMetaDataCb, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    const VolumeDesc * voldesc = dataMgr->getVolumeDesc(dmReq->getVolId());
    Error err = dataMgr->qosCtrl->enqueueIO(voldesc && voldesc->isSnapshot() ?
                                            voldesc->qosQueueId : dmReq->getVolId(),
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
    LOGNORMAL << "finished get volume meta";
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    sendAsyncResp(asyncHdr, fpi::GetVolumeMetaDataMsgTypeId, message);
    delete req;
}

void
DMSvcHandler::handleStatStream(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               boost::shared_ptr<fpi::StatStreamMsg>& message) {
    GLOGDEBUG << "received stat stream " << logString(*asyncHdr);

    auto dmReq = new DmIoStatStream(FdsDmSysTaskId, message);
    dmReq->dmio_statstream_resp_cb =
            BIND_MSG_CALLBACK2(DMSvcHandler::handleStatStreamCb, asyncHdr, message);


    Error err = dataMgr->qosCtrl->enqueueIO(FdsDmSysTaskId,
                                            static_cast<FDS_IOType*>(dmReq));

    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request "
                << logString(*asyncHdr) << ":" << *dmReq;
        dmReq->dmio_statstream_resp_cb(err, dmReq);
    }
}

void
DMSvcHandler::handleStatStreamCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                 boost::shared_ptr<fpi::StatStreamMsg>& message,
                                 const Error &e, DmIoStatStream *req) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << *req);

    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    sendAsyncResp(asyncHdr, fpi::StatStreamMsgTypeId, message);
    delete req;
}

void
DMSvcHandler::registerStreaming(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::StatStreamRegistrationMsg>& streamRegstrMsg) { //NOLINT
    StatStreamAggregator::ptr statAggr = dataMgr->statStreamAggregator();
    fds_assert(statAggr);
    fds_assert(streamRegstrMsg);

    Error err = statAggr->registerStream(streamRegstrMsg);

    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    fpi::StatStreamRegistrationRspMsg resp;
    // since OM did not implement response yet, just not send response for now..
    // sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(StatStreamRegistrationRspMsg), resp);
}

void
DMSvcHandler::deregisterStreaming(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  boost::shared_ptr<fpi::StatStreamDeregistrationMsg>& streamDeregstrMsg) { //NOLINT
    StatStreamAggregator::ptr statAggr = dataMgr->statStreamAggregator();
    fds_assert(statAggr);
    fds_assert(streamDeregstrMsg);

    Error err = statAggr->deregisterStream(streamDeregstrMsg->id);

    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    fpi::StatStreamDeregistrationRspMsg resp;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(StatStreamDeregistrationRspMsg), resp);
}

void
DMSvcHandler::NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &dlt)
{
    Error err(ERR_OK);
    LOGNOTIFY << "OMClient received new DLT commit version  "
              << dlt->dlt_data.dlt_type;
    err = dataMgr->omClient->updateDlt(dlt->dlt_data.dlt_type, dlt->dlt_data.dlt_data);
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDLTUpdate), *dlt);
}


}  // namespace fds
