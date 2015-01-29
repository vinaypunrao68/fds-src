/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <DMSvcHandler.h>
#include <dm-platform.h>
#include <StatStreamAggregator.h>

namespace fds {
DMSvcHandler::DMSvcHandler()
{
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteCatalogObjectMsg, deleteCatalogObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamRegistrationMsg, registerStreaming);
    REGISTER_FDSP_MSG_HANDLER(fpi::StatStreamDeregistrationMsg, deregisterStreaming);
    /* DM to DM service messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::VolSyncStateMsg, volSyncState);
    /* OM to DM snapshot messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CreateSnapshotMsg, createSnapshot);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteSnapshotMsg, deleteSnapshot);
    REGISTER_FDSP_MSG_HANDLER(fpi::CreateVolumeCloneMsg, createVolumeClone);
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, NotifyAddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, NotifyRmVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTClose, NotifyDMTClose);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTUpdate, NotifyDMTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTUpdate, NotifyDLTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::ShutdownMODMsg, shutdownDM);
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

void
DMSvcHandler::NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &dmt)
{
    Error err(ERR_OK);
    LOGNOTIFY << "DMSvcHandler received new DMT commit version  "
              << dmt->dmt_data.dmt_type;
    err = dataMgr->omClient->updateDmt(dmt->dmt_data.dmt_type, dmt->dmt_data.dmt_data);
    if (!err.ok()) {
        LOGERROR << "failed to update DMT " << err;
        NotifyDMTUpdateCb(hdr, ERR_OK);
        return;
    }

    // see if DM sync feature is enabled
    if (dataMgr->feature.isCatSyncEnabled()) {
        err = dataMgr->catSyncMgr->startCatalogSyncDelta(session_uuid,
                                                         std::bind(
                                                             &DMSvcHandler::NotifyDMTUpdateCb,
                                                             this, asyncHdr,
                                                             std::placeholders::_1));
    } else {
        LOGWARN << "catalog sync feature - NOT enabled";
        // ok we just respond...
        NotifyDMTUpdateCb(hdr, ERR_OK);
        return;
    }

    if (!err.ok()) {
        NotifyDMTUpdateCb(hdr, err);
    }
}

void DMSvcHandler::NotifyDMTUpdateCb(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                     Error &err)
{
    LOGDEBUG << "Sending async DMT update ack " << err;
    hdr->msg_code = err.GetErrno();
    fpi::CtrlNotifyDMTUpdatePtr msg(new fpi::CtrlNotifyDMTUpdate());
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDMTUpdate), *msg);
}

void
DMSvcHandler::NotifyDMTClose(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDMTClose> &dmtClose) {
    LOGNOTIFY << "DMSvcHandler received DMT close.";
    Error err(ERR_OK);

    // TODO(xxx) notify volume sync that we can stop forwarding
    // updates to other DM

    /*
    void (*async_f)(const FDS_ProtocolInterface::AsyncHdr &,
            const FDS_ProtocolInterface::FDSPMsgTypeId &,
            const FDS_ProtocolInterface::CtrlNotifyDMTClose &) = &sendAsyncResp;
    */

    dataMgr->sendDmtCloseCb = std::bind(&DMSvcHandler::NotifyDMTCloseCb, this,
            hdr, dmtClose, std::placeholders::_1);
    err = dataMgr->volcat_evt_handler(fds_catalog_dmt_close, FDSP_PushMetaPtr(), "0");

    if (!err.ok()) {
        LOGERROR << "DMT Close, volume meta may not be synced properly";
        // ignore not ready errors
        if (err == ERR_CATSYNC_NOT_PROGRESS)
            err = ERR_OK;
        hdr->msg_code = err.GetErrno();
        sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDMTClose), *dmtClose);
    }


}

void DMSvcHandler::NotifyDMTCloseCb(boost::shared_ptr<fpi::AsyncHdr> &hdr,
        boost::shared_ptr<fpi::CtrlNotifyDMTClose>& dmtClose,
        Error &err)
{
    LOGDEBUG << "Sending async DMT close ack";
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDMTClose), *dmtClose);
}

void DMSvcHandler::shutdownDM(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::ShutdownMODMsg>& shutdownMsg) {
    LOGDEBUG << "Received shutdown message DM ... shuttting down...";
    dataMgr->mod_shutdown();
}

}  // namespace fds
