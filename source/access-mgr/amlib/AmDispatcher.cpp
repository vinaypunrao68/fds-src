/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <string>
#include <fds_process.h>
#include <AmDispatcher.h>
#include <net/SvcRequestPool.h>
#include <net/net-service-tmpl.hpp>
#include <fiu-control.h>
#include <util/fiu_util.h>

namespace fds {

AmDispatcher::AmDispatcher(const std::string &modName,
                           DLTManagerPtr _dltMgr,
                           DMTManagerPtr _dmtMgr)
        : Module(modName.c_str()),
          dltMgr(_dltMgr),
          dmtMgr(_dmtMgr) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    if (conf.get<fds_bool_t>("testing.uturn_dispatcher_all")) {
        fiu_enable("am.uturn.dispatcher", 1, NULL, 0);
    }
}

AmDispatcher::~AmDispatcher() {
}

int
AmDispatcher::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
AmDispatcher::mod_startup() {
}

void
AmDispatcher::mod_shutdown() {
}

void
AmDispatcher::dispatchGetVolumeMetadata(AmQosReq *qosReq) {
    GetVolumeMetaDataReq* volReq = static_cast<GetVolumeMetaDataReq *>(qosReq->getBlobReqPtr());
    fds_verify(true == volReq->magicInUse());

    GetVolumeMetaDataMsgPtr volMDMsg(new fpi::GetVolumeMetaDataMsg());
    volMDMsg->volume_id = volReq->getVolId();
    FailoverSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::getVolumeMetadataCb,
                             qosReq));

    auto asyncGetVolMetadataReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(volReq->getVolId())));
    asyncGetVolMetadataReq->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeMetaDataMsg),
                                    volMDMsg);
    asyncGetVolMetadataReq->onResponseCb(respCb);
    asyncGetVolMetadataReq->invoke();
}

void
AmDispatcher::getVolumeMetadataCb(AmQosReq* qosReq,
                                  FailoverSvcRequest* svcReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    GetVolumeMetaDataReq * volReq =
            static_cast<fds::GetVolumeMetaDataReq*>(qosReq->getBlobReqPtr());
    GetVolumeMetaDataMsgPtr volMDMsg =
        net::ep_deserialize<fpi::GetVolumeMetaDataMsg>(const_cast<Error&>(error), payload);

    volReq->volumeMetadata = volMDMsg->volume_meta_data;
    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    volReq->processorCb(error);
    delete volReq;
}

void
AmDispatcher::dispatchAbortBlobTx(AmQosReq *qosReq) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());

    fiu_do_on("am.uturn.dispatcher", blobReq->processorCb(ERR_OK); delete blobReq; return;);

    fds_volid_t volId = blobReq->getVolId();

    AbortBlobTxMsgPtr stBlobTxMsg(new AbortBlobTxMsg());
    stBlobTxMsg->blob_name      = blobReq->getBlobName();
    stBlobTxMsg->blob_version   = blob_version_invalid;
    stBlobTxMsg->txId           = blobReq->getTxId()->getValue();
    stBlobTxMsg->volume_id      = volId;

    auto asyncAbortBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(dmtMgr->getCommittedNodeGroup(volId)));
    asyncAbortBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::AbortBlobTxMsg), stBlobTxMsg);
    asyncAbortBlobTxReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::abortBlobTxCb, qosReq));

    asyncAbortBlobTxReq->invoke();
    LOGDEBUG << asyncAbortBlobTxReq->logString() << fds::logString(*stBlobTxMsg);
}

void
AmDispatcher::abortBlobTxCb(AmQosReq *qosReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse());

    blobReq->processorCb(error);
}

void
AmDispatcher::dispatchStartBlobTx(AmQosReq *qosReq) {
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(
        qosReq->getBlobReqPtr());

    // Update DMT version in request
    // TODO(Andrew): There's a potential race here between the
    // version we cache in the blobReq now and the version we
    // actually dispatch on below. Make the update/dispatch consistent.
    blobReq->dmtVersion = dmtMgr->getCommittedVersion();

    fiu_do_on("am.uturn.dispatcher", blobReq->processorCb(ERR_OK); delete blobReq; return;);

    // Create callback
    QuorumSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::startBlobTxCb,
                             qosReq));

    // Create network message
    StartBlobTxMsgPtr startBlobTxMsg(new StartBlobTxMsg());
    startBlobTxMsg->blob_name    = blobReq->getBlobName();
    startBlobTxMsg->blob_version = blob_version_invalid;
    startBlobTxMsg->volume_id    = blobReq->getVolId();
    startBlobTxMsg->blob_mode    = blobReq->getBlobMode();
    startBlobTxMsg->txId         = blobReq->txId.getValue();
    startBlobTxMsg->dmt_version  = blobReq->dmtVersion;

    auto asyncStartBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->getVolId())));
    asyncStartBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg),
                                    startBlobTxMsg);
    asyncStartBlobTxReq->onResponseCb(respCb);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString()
             << logString(*startBlobTxMsg);
}

void
AmDispatcher::startBlobTxCb(AmQosReq *qosReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(qosReq != NULL);
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    blobReq->processorCb(error);
    delete blobReq;
}

void
AmDispatcher::dispatchDeleteBlob(AmQosReq *qosReq)
{
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher", blobReq->processorCb(ERR_OK); delete blobReq; return;);

    DeleteBlobMsgPtr message = boost::make_shared<DeleteBlobMsg>();
    message->volume_id = blobReq->getVolId();
    message->blob_name = blobReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = blobReq->txDesc->getValue();

    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->base_vol_id)));

    // Create callback
    QuorumSvcRequestRespCb respCb(RESPONSE_MSG_HANDLER(AmDispatcher::deleteBlobCb, qosReq));

    asyncReq->setPayload(fpi::DeleteBlobMsgTypeId, message);
    asyncReq->onResponseCb(respCb);
    asyncReq->invoke();
}

void
AmDispatcher::deleteBlobCb(AmQosReq* qosReq,
                           QuorumSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload)
{
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq*>(qosReq->getBlobReqPtr());
    LOGDEBUG << " volume:" << blobReq->getVolId()
             << " blob:" << blobReq->getBlobName()
             << " txn:" << blobReq->txDesc;

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
    }
    blobReq->processorCb(error);
}

void
AmDispatcher::dispatchUpdateCatalog(AmQosReq *qosReq) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(qosReq, ERR_OK);  \
              return;);

    UpdateCatalogMsgPtr updCatMsg(boost::make_shared<UpdateCatalogMsg>());
    updCatMsg->blob_name    = blobReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = blobReq->getVolId();
    updCatMsg->txId         = blobReq->txDesc->getValue();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = blobReq->getBlobOffset();
    updBlobInfo.size     = blobReq->getDataLen();
    updBlobInfo.blob_end = blobReq->isLastBuf();
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)blobReq->getObjId().GetId(),
                        (size_t)blobReq->getObjId().GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    fds::PerfTracer::tracePointBegin(blobReq->dmPerfCtx);

    auto asyncUpdateCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->getVolId())));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), updCatMsg);
    asyncUpdateCatReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogCb, qosReq));
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void
AmDispatcher::dispatchUpdateCatalogOnce(AmQosReq *qosReq) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(qosReq, ERR_OK); \
              return;);

    UpdateCatalogOnceMsgPtr updCatMsg(boost::make_shared<UpdateCatalogOnceMsg>());
    updCatMsg->blob_name    = blobReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = blobReq->getVolId();
    updCatMsg->txId         = blobReq->txDesc->getValue();
    updCatMsg->blob_mode    = blobReq->blobMode;

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    fpi::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = blobReq->getBlobOffset();
    updBlobInfo.size     = blobReq->getDataLen();
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)blobReq->getObjId().GetId(),
                        (size_t)blobReq->getObjId().GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    // Setup blob metadata updates
    FDS_ProtocolInterface::FDSP_MetaDataPair metaDataPair;
    for (auto& meta : *(blobReq->metadata)) {
        metaDataPair.key   = meta.first;
        metaDataPair.value = meta.second;
        updCatMsg->meta_list.push_back(metaDataPair);
    }

    PerfTracer::tracePointBegin(blobReq->dmPerfCtx);

    // Always use the current DMT version since we're updating in a single request
    auto asyncUpdateCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->getVolId())));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg), updCatMsg);
    asyncUpdateCatReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogCb, qosReq));
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << logString(*updCatMsg);
}

void
AmDispatcher::updateCatalogCb(AmQosReq* qosReq,
                              QuorumSvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) {
    PutBlobReq *blobReq = static_cast<PutBlobReq*>(qosReq->getBlobReqPtr());
    PerfTracer::tracePointEnd(blobReq->dmPerfCtx);
    fpi::UpdateCatalogRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
                 << " blob name: " << blobReq->getBlobName()
                 << " offset: " << blobReq->getBlobOffset()
                 << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
    }
    blobReq->notifyResponse(qosReq, error);
}

void
AmDispatcher::dispatchPutObject(AmQosReq *qosReq) {
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getDataLen() > 0);

    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(qosReq, ERR_OK); \
              return;);

    PutObjectMsgPtr putObjMsg(boost::make_shared<PutObjectMsg>());
    putObjMsg->volume_id        = blobReq->getVolId();
    putObjMsg->origin_timestamp = util::getTimeStampMillis();
    putObjMsg->data_obj.assign(blobReq->getDataBuf(), blobReq->getDataLen());
    putObjMsg->data_obj_len     = blobReq->getDataLen();
    putObjMsg->data_obj_id.digest = std::string(
        (const char *)blobReq->getObjId().GetId(),
        (size_t)blobReq->getObjId().GetLen());

    PerfTracer::tracePointBegin(blobReq->smPerfCtx);

    auto asyncPutReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(
            dltMgr->getDLT()->getNodes(blobReq->getObjId())));
    asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
    asyncPutReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::putObjectCb, qosReq));
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << logString(*putObjMsg);
}

void
AmDispatcher::putObjectCb(AmQosReq* qosReq,
                          QuorumSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload) {
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
    PerfTracer::tracePointEnd(blobReq->smPerfCtx);
    fpi::PutObjectRspMsgPtr putObjRsp =
            net::ep_deserialize<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << logString(*putObjRsp);
    }
    blobReq->notifyResponse(qosReq, error);
}

void
AmDispatcher::dispatchGetObject(AmQosReq *qosReq)
{
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher",
              GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb); \
              cb->returnSize = blobReq->getDataLen(); \
              memset(cb->returnBuffer, 0x00, cb->returnSize); \
              blobReq->processorCb(ERR_OK); \
              return;);

    PerfTracer::tracePointBegin(blobReq->smPerfCtx);
    fds_volid_t volId = blobReq->getVolId();
    ObjectID objId    = blobReq->getObjId();

    // TODO(Andrew): This is a hack to handle reading zero length blobs.
    // The DM or AM cache is going to return a invalid object Id (i.e., 0)
    // the represents an empty blob. Here's we're just going to set the data
    // length to 0 and return. We should figure out how we want this flow to
    // actually go (the entire read API and path should be improved).
    if (objId == NullObjectID) {
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);
        cb->returnSize = 0;

        blobReq->processorCb(ERR_OK);
        return;
    }

    fpi::GetObjectMsgPtr getObjMsg(boost::make_shared<fpi::GetObjectMsg>());
    getObjMsg->volume_id = volId;
    getObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(),
                                                (size_t)objId.GetLen());

    auto asyncGetReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(dltMgr->getDLT()->getNodes(objId)));
    asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
    asyncGetReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::getObjectCb, qosReq));
    asyncGetReq->invoke();

    LOGDEBUG << asyncGetReq->logString() << logString(*getObjMsg);
}

void
AmDispatcher::getObjectCb(AmQosReq* qosReq,
                        FailoverSvcRequest* svcReq,
                        const Error& error,
                        boost::shared_ptr<std::string> payload)
{
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);

    fpi::GetObjectRespPtr getObjRsp =
        net::ep_deserialize<fpi::GetObjectResp>(const_cast<Error&>(error), payload);
    PerfTracer::tracePointEnd(blobReq->smPerfCtx);

    if (error == ERR_OK) {
        LOGDEBUG << svcReq->logString() << logString(*getObjRsp);
        /* NOTE: we are currently supporting only getting the whole blob
         * so the requester does not know about the blob length, 
         * we get the blob length in response from SM;
         * will need to revisit when we also support (don't ignore) byteCount in native api.
         * For now, just verify the existing buffer is big enough to hold
         * the data.
         */
        if (blobReq->getDataLen() >= (4 * 1024)) {
            // Check that we didn't get too much data
            // Since 4K is our min, it's OK to get more
            // when less than 4K is requested
            // TODO(Andrew): Revisit for unaligned IO
            fds_verify(getObjRsp->data_obj.size() <= blobReq->getDataLen());
        }

        // Only return UP-TO the amount of data requested, never more
        cb->returnSize = std::min(blobReq->getDataLen(), getObjRsp->data_obj.size());
        memcpy(cb->returnBuffer, getObjRsp->data_obj.c_str(), cb->returnSize);
    } else {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << error;
    }

    blobReq->processorCb(error);
}

void
AmDispatcher::dispatchQueryCatalog(AmQosReq *qosReq) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher",
              blobReq->setObjId(ObjectID()); \
              blobReq->processorCb(ERR_OK); \
              return;);

    PerfTracer::tracePointBegin(blobReq->dmPerfCtx);
    std::string blobName = blobReq->getBlobName();
    size_t blobOffset = blobReq->getBlobOffset();
    fds_volid_t volId = blobReq->getVolId();

    LOGDEBUG << "blob name: " << blobName << " offset: " << blobOffset << " volid: " << volId;
    /*
     * TODO(Andrew): We should eventually specify the offset in the blob
     * we want...all objects won't work well for large blobs.
     */
    fpi::QueryCatalogMsgPtr queryMsg(new fpi::QueryCatalogMsg());
    queryMsg->volume_id    = volId;
    queryMsg->blob_name    = blobName;
    queryMsg->start_offset  = blobOffset;
    // TODO(umesh): need to use valid end_offset; -1 for all starting from start_offset
    queryMsg->end_offset   = -1;
    // We don't currently specify a version
    queryMsg->blob_version = blob_version_invalid;
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();

    auto asyncQueryReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->base_vol_id)));
    asyncQueryReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), queryMsg);
    asyncQueryReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::getQueryCatalogCb, qosReq));
    asyncQueryReq->invoke();

    LOGDEBUG << asyncQueryReq->logString() << logString(*queryMsg);
}

void
AmDispatcher::getQueryCatalogCb(AmQosReq* qosReq,
                                FailoverSvcRequest* svcReq,
                                const Error& error,
                                boost::shared_ptr<std::string> payload)
{
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(qosReq->getBlobReqPtr());
    fpi::QueryCatalogMsgPtr qryCatRsp =
        net::ep_deserialize<fpi::QueryCatalogMsg>(const_cast<Error&>(error), payload);

    PerfTracer::tracePointEnd(blobReq->dmPerfCtx);

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << error;
        // TODO(Andrew): We should change XDI to not expect OFFSET_INVALID, rather NOT_FOUND
        blobReq->processorCb(error == ERR_CAT_ENTRY_NOT_FOUND ? ERR_BLOB_OFFSET_INVALID : error);
        return;
    }

    LOGDEBUG << svcReq->logString() << logString(*qryCatRsp);

    // TODO(Andrew): Update the AM's blob offset cache here

    // TODO(xxx) should be able to have multiple object id + implement range
    // queries in DM
    for (fpi::FDSP_BlobObjectList::const_iterator it = qryCatRsp->obj_list.cbegin();
         it != qryCatRsp->obj_list.cend();
         ++it) {
        if ((fds_uint64_t)(*it).offset == blobReq->getBlobOffset()) {
            // found offset!!!
            ObjectID objId((*it).data_obj_id.digest);
            // TODO(Andrew): Consider adding this back when we revisit
            // zero length objects
            // fds_verify(objId != NullObjectID);

            blobReq->setObjId(objId);
            blobReq->processorCb(ERR_OK);
            return;
        }
    }

    // if we are here, we did not get response for offset we needed!
    LOGDEBUG << "blob name: " << blobReq->getBlobName() << "offset: "
             << blobReq->getBlobOffset() << " not in returned offset list from DM";
    blobReq->processorCb(ERR_BLOB_OFFSET_INVALID);
}

void
AmDispatcher::dispatchStatBlob(AmQosReq *qosReq)
{
    StatBlobReq* blobReq = static_cast<StatBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher",
              StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, blobReq->cb); \
              cb->blobDesc.setBlobName(blobReq->getBlobName()); \
              cb->blobDesc.setBlobSize(0); \
              blobReq->processorCb(ERR_OK); \
              return;);

    GetBlobMetaDataMsgPtr message = boost::make_shared<GetBlobMetaDataMsg>();
    message->volume_id = blobReq->getVolId();
    message->blob_name = blobReq->getBlobName();

    auto asyncReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->base_vol_id)));

    asyncReq->setPayload(fpi::GetBlobMetaDataMsgTypeId, message);
    asyncReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::statBlobCb, qosReq));

    asyncReq->invoke();
}

void
AmDispatcher::statBlobCb(AmQosReq* qosReq,
                         FailoverSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload)
{
    StatBlobReq* blobReq = static_cast<StatBlobReq *>(qosReq->getBlobReqPtr());

    // Deserialize if all OK
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetBlobMetaDataMsg, error, payload);

        StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, blobReq->cb);
        // Fill in the data here
        cb->blobDesc.setBlobName(blobReq->getBlobName());
        cb->blobDesc.setBlobSize(response->byteCount);
        for (const auto& meta : response->metaDataList) {
            cb->blobDesc.addKvMeta(meta.key,  meta.value);
        }
    }
    blobReq->processorCb(error);
}

void
AmDispatcher::dispatchCommitBlobTx(AmQosReq *qosReq) {
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(
        qosReq->getBlobReqPtr());

    fiu_do_on("am.uturn.dispatcher", blobReq->processorCb(ERR_OK); delete blobReq; return;);

    // Create callback
    QuorumSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::commitBlobTxCb,
                             qosReq));

    // Create network message
    CommitBlobTxMsgPtr commitBlobTxMsg = boost::make_shared<CommitBlobTxMsg>();
    commitBlobTxMsg->blob_name    = blobReq->getBlobName();
    commitBlobTxMsg->blob_version = blob_version_invalid;
    commitBlobTxMsg->volume_id    = blobReq->getVolId();
    commitBlobTxMsg->txId         = blobReq->getTxId()->getValue();
    commitBlobTxMsg->dmt_version  = dmtMgr->getCommittedVersion();

    auto asyncCommitBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->getVolId())));
    asyncCommitBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg),
                                    commitBlobTxMsg);
    asyncCommitBlobTxReq->onResponseCb(respCb);
    asyncCommitBlobTxReq->invoke();

    LOGDEBUG << asyncCommitBlobTxReq->logString()
             << logString(*commitBlobTxMsg);
}

void
AmDispatcher::commitBlobTxCb(AmQosReq *qosReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(qosReq != NULL);
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    blobReq->processorCb(error);
}

void
AmDispatcher::dispatchVolumeContents(AmQosReq *qosReq)
{
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse());

    GetBucketMsgPtr message = boost::make_shared<GetBucketMsg>();
    message->volume_id = blobReq->getVolId();
    message->startPos  = 0;
    message->maxKeys   = blobReq->maxkeys;

    auto asyncReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->base_vol_id)));

    asyncReq->setPayload(fpi::GetBucketMsgTypeId, message);
    asyncReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::volumeContentsCb, qosReq));

    asyncReq->invoke();
}

void
AmDispatcher::volumeContentsCb(AmQosReq* qosReq,
                               FailoverSvcRequest* svcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload)
{
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq *>(qosReq->getBlobReqPtr());

    // Return if err
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetBucketMsg, error, payload);

        GetBucketCallback::ptr cb = SHARED_DYN_CAST(GetBucketCallback, blobReq->cb);
        cb->contentsCount = response->blob_info_list.size();
        ListBucketContents* bucketContents = new ListBucketContents[cb->contentsCount];
        LOGDEBUG << " volid: " << response->volume_id
            << " numBlobs: " << response->blob_info_list.size();
        for (int i = 0; i < cb->contentsCount; ++i) {
            bucketContents[i].set(response->blob_info_list[i].blob_name,
                                  0,  // last modified
                                  "",  // eTag
                                  response->blob_info_list[i].blob_size,
                                  "",  // ownerId
                                  "");
        }
        cb->contents = bucketContents;
    }
    blobReq->processorCb(error);
}

}  // namespace fds
