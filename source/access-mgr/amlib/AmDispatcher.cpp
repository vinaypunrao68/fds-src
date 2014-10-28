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
#include "responsehandler.h"

#include "requests/requests.h"

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
AmDispatcher::dispatchGetVolumeMetadata(AmRequest *amReq) {
    GetVolumeMetaDataReq* volReq = static_cast<GetVolumeMetaDataReq *>(amReq);
    fds_verify(true == volReq->magicInUse());

    GetVolumeMetaDataMsgPtr volMDMsg(new fpi::GetVolumeMetaDataMsg());
    volMDMsg->volume_id = volReq->io_vol_id;
    FailoverSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::getVolumeMetadataCb, amReq));

    auto asyncGetVolMetadataReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(volReq->io_vol_id)));
    asyncGetVolMetadataReq->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeMetaDataMsg),
                                    volMDMsg);
    asyncGetVolMetadataReq->onResponseCb(respCb);
    asyncGetVolMetadataReq->invoke();
}

void
AmDispatcher::getVolumeMetadataCb(AmRequest* amReq,
                                  FailoverSvcRequest* svcReq,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload) {
    GetVolumeMetaDataReq * volReq =
            static_cast<fds::GetVolumeMetaDataReq*>(amReq);
    GetVolumeMetaDataMsgPtr volMDMsg =
        net::ep_deserialize<fpi::GetVolumeMetaDataMsg>(const_cast<Error&>(error), payload);

    volReq->volumeMetadata = volMDMsg->volume_meta_data;
    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    volReq->proc_cb(error);
    delete volReq;
}

void
AmDispatcher::dispatchAbortBlobTx(AmRequest *amReq) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(amReq);

    fiu_do_on("am.uturn.dispatcher", blobReq->proc_cb(ERR_OK); delete blobReq; return;);

    fds_volid_t volId = blobReq->io_vol_id;

    AbortBlobTxMsgPtr stBlobTxMsg(new AbortBlobTxMsg());
    stBlobTxMsg->blob_name      = blobReq->getBlobName();
    stBlobTxMsg->blob_version   = blob_version_invalid;
    stBlobTxMsg->txId           = blobReq->tx_desc->getValue();
    stBlobTxMsg->volume_id      = volId;

    auto asyncAbortBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(dmtMgr->getCommittedNodeGroup(volId)));
    asyncAbortBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::AbortBlobTxMsg), stBlobTxMsg);
    asyncAbortBlobTxReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::abortBlobTxCb, amReq));

    asyncAbortBlobTxReq->invoke();
    LOGDEBUG << asyncAbortBlobTxReq->logString() << fds::logString(*stBlobTxMsg);
}

void
AmDispatcher::abortBlobTxCb(AmRequest *amReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(amReq);
    fds_verify(blobReq->magicInUse());

    blobReq->proc_cb(error);
}

void
AmDispatcher::dispatchStartBlobTx(AmRequest *amReq) {
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(amReq);

    // Update DMT version in request
    // TODO(Andrew): There's a potential race here between the
    // version we cache in the blobReq now and the version we
    // actually dispatch on below. Make the update/dispatch consistent.
    blobReq->dmtVersion = dmtMgr->getCommittedVersion();

    fiu_do_on("am.uturn.dispatcher", blobReq->proc_cb(ERR_OK); delete blobReq; return;);

    // Create callback
    QuorumSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::startBlobTxCb, amReq));

    // Create network message
    StartBlobTxMsgPtr startBlobTxMsg(new StartBlobTxMsg());
    startBlobTxMsg->blob_name    = blobReq->getBlobName();
    startBlobTxMsg->blob_version = blob_version_invalid;
    startBlobTxMsg->volume_id    = blobReq->io_vol_id;
    startBlobTxMsg->blob_mode    = blobReq->blob_mode;
    startBlobTxMsg->txId         = blobReq->tx_desc->getValue();
    startBlobTxMsg->dmt_version  = blobReq->dmtVersion;

    auto asyncStartBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));
    asyncStartBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg),
                                    startBlobTxMsg);
    asyncStartBlobTxReq->onResponseCb(respCb);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString()
             << logString(*startBlobTxMsg);
}

void
AmDispatcher::startBlobTxCb(AmRequest *amReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(amReq != NULL);
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    blobReq->proc_cb(error);
    delete blobReq;
}

void
AmDispatcher::dispatchDeleteBlob(AmRequest *amReq)
{
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher", blobReq->proc_cb(ERR_OK); delete blobReq; return;);

    DeleteBlobMsgPtr message = boost::make_shared<DeleteBlobMsg>();
    message->volume_id = blobReq->io_vol_id;
    message->blob_name = blobReq->getBlobName();
    message->blob_version = blob_version_invalid;
    message->txId = blobReq->tx_desc->getValue();

    auto asyncReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));

    // Create callback
    QuorumSvcRequestRespCb respCb(RESPONSE_MSG_HANDLER(AmDispatcher::deleteBlobCb, amReq));

    asyncReq->setPayload(fpi::DeleteBlobMsgTypeId, message);
    asyncReq->onResponseCb(respCb);
    asyncReq->invoke();
}

void
AmDispatcher::deleteBlobCb(AmRequest* amReq,
                           QuorumSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload)
{
    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq*>(amReq);
    LOGDEBUG << " volume:" << blobReq->io_vol_id
             << " blob:" << blobReq->getBlobName()
             << " txn:" << blobReq->tx_desc;

    // Return if err
    if (error != ERR_OK) {
        LOGWARN << "error in response: " << error;
    }
    blobReq->proc_cb(error);
}

void
AmDispatcher::dispatchUpdateCatalog(AmRequest *amReq) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);

    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(amReq, ERR_OK);  \
              return;);

    UpdateCatalogMsgPtr updCatMsg(boost::make_shared<UpdateCatalogMsg>());
    updCatMsg->blob_name    = blobReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = blobReq->io_vol_id;
    updCatMsg->txId         = blobReq->tx_desc->getValue();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = blobReq->blob_offset;
    updBlobInfo.size     = blobReq->data_len;
    updBlobInfo.blob_end = blobReq->isLastBuf();
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)blobReq->obj_id.GetId(),
                        (size_t)blobReq->obj_id.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    fds::PerfTracer::tracePointBegin(blobReq->dm_perf_ctx);

    auto asyncUpdateCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), updCatMsg);
    asyncUpdateCatReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogCb, amReq));
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void
AmDispatcher::dispatchUpdateCatalogOnce(AmRequest *amReq) {
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);

    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(amReq, ERR_OK); \
              return;);

    UpdateCatalogOnceMsgPtr updCatMsg(boost::make_shared<UpdateCatalogOnceMsg>());
    updCatMsg->blob_name    = blobReq->getBlobName();
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = blobReq->io_vol_id;
    updCatMsg->txId         = blobReq->tx_desc->getValue();
    updCatMsg->blob_mode    = blobReq->blobMode;

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    fpi::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = blobReq->blob_offset;
    updBlobInfo.size     = blobReq->data_len;
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)blobReq->obj_id.GetId(),
                        (size_t)blobReq->obj_id.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    // Setup blob metadata updates
    FDS_ProtocolInterface::FDSP_MetaDataPair metaDataPair;
    for (auto& meta : *(blobReq->metadata)) {
        metaDataPair.key   = meta.first;
        metaDataPair.value = meta.second;
        updCatMsg->meta_list.push_back(metaDataPair);
    }

    PerfTracer::tracePointBegin(blobReq->dm_perf_ctx);

    // Always use the current DMT version since we're updating in a single request
    auto asyncUpdateCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg), updCatMsg);
    asyncUpdateCatReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::updateCatalogCb, amReq));
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << logString(*updCatMsg);
}

void
AmDispatcher::updateCatalogCb(AmRequest* amReq,
                              QuorumSvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) {
    PutBlobReq *blobReq = static_cast<PutBlobReq*>(amReq);
    PerfTracer::tracePointEnd(blobReq->dm_perf_ctx);
    fpi::UpdateCatalogRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
                 << " blob name: " << blobReq->getBlobName()
                 << " offset: " << blobReq->blob_offset
                 << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
    }
    blobReq->notifyResponse(amReq, error);
}

void
AmDispatcher::dispatchPutObject(AmRequest *amReq) {
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(amReq);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->data_len > 0);

    fiu_do_on("am.uturn.dispatcher",
              blobReq->notifyResponse(amReq, ERR_OK); \
              return;);

    PutObjectMsgPtr putObjMsg(boost::make_shared<PutObjectMsg>());
    putObjMsg->volume_id        = blobReq->io_vol_id;
    putObjMsg->origin_timestamp = util::getTimeStampMillis();
    putObjMsg->data_obj.assign(blobReq->getDataBuf(), blobReq->data_len);
    putObjMsg->data_obj_len     = blobReq->data_len;
    putObjMsg->data_obj_id.digest = std::string(
        (const char *)blobReq->obj_id.GetId(),
        (size_t)blobReq->obj_id.GetLen());

    PerfTracer::tracePointBegin(blobReq->sm_perf_ctx);

    auto asyncPutReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(
            dltMgr->getDLT()->getNodes(blobReq->obj_id)));
    asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
    asyncPutReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::putObjectCb, amReq));
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << logString(*putObjMsg);
}

void
AmDispatcher::putObjectCb(AmRequest* amReq,
                          QuorumSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload) {
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(amReq);
    PerfTracer::tracePointEnd(blobReq->sm_perf_ctx);
    fpi::PutObjectRspMsgPtr putObjRsp =
            net::ep_deserialize<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->blob_offset << " Error: " << error;
    } else {
        LOGDEBUG << svcReq->logString() << logString(*putObjRsp);
    }
    blobReq->notifyResponse(amReq, error);
}

void
AmDispatcher::dispatchGetObject(AmRequest *amReq)
{
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher",
              GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb); \
              cb->returnSize = blobReq->data_len; \
              memset(cb->returnBuffer, 0x00, cb->returnSize); \
              blobReq->proc_cb(ERR_OK); \
              return;);

    PerfTracer::tracePointBegin(blobReq->sm_perf_ctx);
    fds_volid_t volId = blobReq->io_vol_id;
    ObjectID objId    = blobReq->obj_id;

    // TODO(Andrew): This is a hack to handle reading zero length blobs.
    // The DM or AM cache is going to return a invalid object Id (i.e., 0)
    // the represents an empty blob. Here's we're just going to set the data
    // length to 0 and return. We should figure out how we want this flow to
    // actually go (the entire read API and path should be improved).
    if (objId == NullObjectID) {
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);
        cb->returnSize = 0;

        blobReq->proc_cb(ERR_OK);
        return;
    }

    fpi::GetObjectMsgPtr getObjMsg(boost::make_shared<fpi::GetObjectMsg>());
    getObjMsg->volume_id = volId;
    getObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(),
                                                (size_t)objId.GetLen());

    auto asyncGetReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(dltMgr->getDLT()->getNodes(objId)));
    asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
    asyncGetReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::getObjectCb, amReq));
    asyncGetReq->invoke();

    LOGDEBUG << asyncGetReq->logString() << logString(*getObjMsg);
}

void
AmDispatcher::getObjectCb(AmRequest* amReq,
                        FailoverSvcRequest* svcReq,
                        const Error& error,
                        boost::shared_ptr<std::string> payload)
{
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);

    fpi::GetObjectRespPtr getObjRsp =
        net::ep_deserialize<fpi::GetObjectResp>(const_cast<Error&>(error), payload);
    PerfTracer::tracePointEnd(blobReq->sm_perf_ctx);

    if (error == ERR_OK) {
        LOGDEBUG << svcReq->logString() << logString(*getObjRsp);
        /* NOTE: we are currently supporting only getting the whole blob
         * so the requester does not know about the blob length, 
         * we get the blob length in response from SM;
         * will need to revisit when we also support (don't ignore) byteCount in native api.
         * For now, just verify the existing buffer is big enough to hold
         * the data.
         */
        if (blobReq->data_len >= (4 * 1024)) {
            // Check that we didn't get too much data
            // Since 4K is our min, it's OK to get more
            // when less than 4K is requested
            // TODO(Andrew): Revisit for unaligned IO
            fds_verify(getObjRsp->data_obj.size() <= blobReq->data_len);
        }

        // Only return UP-TO the amount of data requested, never more
        cb->returnSize = std::min(blobReq->data_len, getObjRsp->data_obj.size());
        memcpy(cb->returnBuffer, getObjRsp->data_obj.c_str(), cb->returnSize);
    } else {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->blob_offset << " Error: " << error;
    }

    blobReq->proc_cb(error);
}

void
AmDispatcher::dispatchQueryCatalog(AmRequest *amReq) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher",
              blobReq->obj_id = ObjectID(); \
              blobReq->proc_cb(ERR_OK); \
              return;);

    PerfTracer::tracePointBegin(blobReq->dm_perf_ctx);
    std::string blobName = blobReq->getBlobName();
    size_t blobOffset = blobReq->blob_offset;
    fds_volid_t volId = blobReq->io_vol_id;

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
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));
    asyncQueryReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), queryMsg);
    asyncQueryReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::getQueryCatalogCb, amReq));
    asyncQueryReq->invoke();

    LOGDEBUG << asyncQueryReq->logString() << logString(*queryMsg);
}

void
AmDispatcher::getQueryCatalogCb(AmRequest* amReq,
                                FailoverSvcRequest* svcReq,
                                const Error& error,
                                boost::shared_ptr<std::string> payload)
{
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
    fpi::QueryCatalogMsgPtr qryCatRsp =
        net::ep_deserialize<fpi::QueryCatalogMsg>(const_cast<Error&>(error), payload);

    PerfTracer::tracePointEnd(blobReq->dm_perf_ctx);

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->blob_offset << " Error: " << error;
        // TODO(Andrew): We should change XDI to not expect OFFSET_INVALID, rather NOT_FOUND
        blobReq->proc_cb(error == ERR_CAT_ENTRY_NOT_FOUND ? ERR_BLOB_OFFSET_INVALID : error);
        return;
    }

    LOGDEBUG << svcReq->logString() << logString(*qryCatRsp);

    // TODO(Andrew): Update the AM's blob offset cache here

    // TODO(xxx) should be able to have multiple object id + implement range
    // queries in DM
    for (fpi::FDSP_BlobObjectList::const_iterator it = qryCatRsp->obj_list.cbegin();
         it != qryCatRsp->obj_list.cend();
         ++it) {
        if ((fds_uint64_t)(*it).offset == blobReq->blob_offset) {
            // found offset!!!
            ObjectID objId((*it).data_obj_id.digest);
            // TODO(Andrew): Consider adding this back when we revisit
            // zero length objects
            // fds_verify(objId != NullObjectID);

            blobReq->obj_id = objId;
            blobReq->proc_cb(ERR_OK);
            return;
        }
    }

    // if we are here, we did not get response for offset we needed!
    LOGDEBUG << "blob name: " << blobReq->getBlobName() << "offset: "
             << blobReq->blob_offset << " not in returned offset list from DM";
    blobReq->proc_cb(ERR_BLOB_OFFSET_INVALID);
}

void
AmDispatcher::dispatchStatBlob(AmRequest *amReq)
{
    StatBlobReq* blobReq = static_cast<StatBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse());

    fiu_do_on("am.uturn.dispatcher",
              StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, blobReq->cb); \
              cb->blobDesc.setBlobName(blobReq->getBlobName()); \
              cb->blobDesc.setBlobSize(0); \
              blobReq->proc_cb(ERR_OK); \
              return;);

    GetBlobMetaDataMsgPtr message = boost::make_shared<GetBlobMetaDataMsg>();
    message->volume_id = blobReq->io_vol_id;
    message->blob_name = blobReq->getBlobName();

    auto asyncReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));

    asyncReq->setPayload(fpi::GetBlobMetaDataMsgTypeId, message);
    asyncReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::statBlobCb, amReq));

    asyncReq->invoke();
}

void
AmDispatcher::dispatchSetBlobMetadata(AmRequest *amReq) {
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(amReq);

    fds_volid_t   vol_id = blobReq->io_vol_id;

    SetBlobMetaDataMsgPtr setMDMsg = boost::make_shared<SetBlobMetaDataMsg>();
    setMDMsg->blob_name = blobReq->getBlobName();
    setMDMsg->blob_version = blob_version_invalid;
    setMDMsg->volume_id = vol_id;
    setMDMsg->txId = blobReq->dmt_version;

    setMDMsg->metaDataList = std::move(*blobReq->getMetaDataListPtr());

    auto asyncSetMDReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getVersionNodeGroup(vol_id, blobReq->dmt_version)));

    // Create callback
    QuorumSvcRequestRespCb respCb(RESPONSE_MSG_HANDLER(AmDispatcher::setBlobMetadataCb, amReq));

    asyncSetMDReq->setPayload(FDSP_MSG_TYPEID(fpi::SetBlobMetaDataMsg), setMDMsg);
    asyncSetMDReq->onResponseCb(respCb);
    asyncSetMDReq->invoke();
}

void
AmDispatcher::setBlobMetadataCb(AmRequest *amReq,
                                QuorumSvcRequest *svcReq,
                                const Error &error,
                                boost::shared_ptr<std::string> payload) {
    SetBlobMetaDataReq *blobReq = static_cast<fds::SetBlobMetaDataReq*>(amReq);

    if (error != ERR_OK) {
        LOGERROR << "Set metadata blob name: " << blobReq->getBlobName() << " Error: " << error;
    } else {
        fpi::SetBlobMetaDataRspMsgPtr setMDRsp =
            net::ep_deserialize<fpi::SetBlobMetaDataRspMsg>(const_cast<Error&>(error), payload);
        LOGDEBUG << svcReq->logString() << fds::logString(*setMDRsp);
    }
    blobReq->proc_cb(error);
}

void
AmDispatcher::statBlobCb(AmRequest* amReq,
                         FailoverSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload)
{
    StatBlobReq* blobReq = static_cast<StatBlobReq *>(amReq);

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
    blobReq->proc_cb(error);
}

void
AmDispatcher::dispatchCommitBlobTx(AmRequest *amReq) {
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(amReq);

    fiu_do_on("am.uturn.dispatcher", blobReq->proc_cb(ERR_OK); delete blobReq; return;);

    // Create callback
    QuorumSvcRequestRespCb respCb(
        RESPONSE_MSG_HANDLER(AmDispatcher::commitBlobTxCb, amReq));

    // Create network message
    CommitBlobTxMsgPtr commitBlobTxMsg = boost::make_shared<CommitBlobTxMsg>();
    commitBlobTxMsg->blob_name    = blobReq->getBlobName();
    commitBlobTxMsg->blob_version = blob_version_invalid;
    commitBlobTxMsg->volume_id    = blobReq->io_vol_id;
    commitBlobTxMsg->txId         = blobReq->tx_desc->getValue();
    commitBlobTxMsg->dmt_version  = dmtMgr->getCommittedVersion();

    auto asyncCommitBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));
    asyncCommitBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg),
                                    commitBlobTxMsg);
    asyncCommitBlobTxReq->onResponseCb(respCb);
    asyncCommitBlobTxReq->invoke();

    LOGDEBUG << asyncCommitBlobTxReq->logString()
             << logString(*commitBlobTxMsg);
}

void
AmDispatcher::commitBlobTxCb(AmRequest *amReq,
                            QuorumSvcRequest *svcReq,
                            const Error &error,
                            boost::shared_ptr<std::string> payload) {
    fds_verify(amReq != NULL);
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Notify upper layers that the request is done. When this
    // completes, all upper layers should be notified and we
    // can safely delete the request
    blobReq->proc_cb(error);
}

void
AmDispatcher::dispatchVolumeContents(AmRequest *amReq)
{
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq *>(amReq);
    fds_verify(blobReq->magicInUse());

    GetBucketMsgPtr message = boost::make_shared<GetBucketMsg>();
    message->volume_id = blobReq->io_vol_id;
    message->startPos  = 0;
    message->maxKeys   = blobReq->maxkeys;

    auto asyncReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(
            dmtMgr->getCommittedNodeGroup(blobReq->io_vol_id)));

    asyncReq->setPayload(fpi::GetBucketMsgTypeId, message);
    asyncReq->onResponseCb(RESPONSE_MSG_HANDLER(AmDispatcher::volumeContentsCb, amReq));

    asyncReq->invoke();
}

void
AmDispatcher::volumeContentsCb(AmRequest* amReq,
                               FailoverSvcRequest* svcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload)
{
    VolumeContentsReq* blobReq = static_cast<VolumeContentsReq *>(amReq);

    // Return if err
    if (ERR_OK == error) {
        // using the same structure for input and output
        auto response = MSG_DESERIALIZE(GetBucketMsg, error, payload);

        ListBucketResponseHandler::ptr cb = SHARED_DYN_CAST(ListBucketResponseHandler, blobReq->cb);
        size_t count = response->blob_info_list.size();
        LOGDEBUG << " volid: " << response->volume_id << " numBlobs: " << count;
        for (size_t i = 0; i < count; ++i) {
            apis::BlobDescriptor bd;
            bd.name = response->blob_info_list[i].blob_name;
            bd.byteCount = response->blob_info_list[i].blob_size;
            cb->vecBlobs.push_back(bd);
        }
    }
    blobReq->proc_cb(error);
}

}  // namespace fds
