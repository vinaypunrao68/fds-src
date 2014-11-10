/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include <fds_config.hpp>
#include <fds_process.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetSession.h"
#include <dlt.h>
#include <ObjectId.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <fdsp/DMSvc.h>
#include <fdsp/SMSvc.h>
#include <fdsp_utils.h>

#include "requests/requests.h"

extern StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;

std::atomic_uint nextIoReqId;

void
StorHvCtrl::enqueueAttachReq(const std::string& volumeName,
                             CallbackPtr cb) {
    LOGDEBUG << "Attach request for volume " << volumeName;

    // check if volume is already attached
    fds_volid_t volId = invalid_vol_id;
    if (invalid_vol_id != (volId = vol_table->getVolumeUUID(volumeName))) {
        LOGDEBUG << "Volume " << volumeName
                 << " with UUID " << volId
                 << " already attached";
        cb->call(ERR_OK);
        return;
    }

    // Create a noop request to put into wait queue
    AttachVolBlobReq *blobReq = new AttachVolBlobReq(volId, volumeName, cb);

    // Enqueue this request to process the callback
    // when the attach is complete
    vol_table->addBlobToWaitQueue(volumeName, blobReq);

    fds_verify(sendTestBucketToOM(volumeName,
                                  "",  // The access key isn't used
                                  "") == ERR_OK); // The secret key isn't used
}

Error
StorHvCtrl::pushBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);
    Error err(ERR_OK);

    PerfTracer::tracePointBegin(blobReq->e2e_req_perf_ctx); 
    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx); 

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
    fds_volid_t volId = blobReq->io_vol_id;

    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    if ((shVol == NULL) || (shVol->volQueue == NULL)) {
        if (shVol)
            shVol->readUnlock();
        LOGERROR << "Volume and queueus are NOT setup for volume " << volId;
        err = ERR_INVALID_ARG;
        PerfTracer::tracePointEnd(blobReq->qos_perf_ctx); 
        delete blobReq;
        return err;
    }
    /*
     * TODO: We should handle some sort of success/failure here?
     */
    qos_ctrl->enqueueIO(volId, blobReq);
    shVol->readUnlock();

    LOGDEBUG << "Queued IO for vol " << volId;

    return err;
}

void
StorHvCtrl::enqueueBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);

    // check if volume is attached to this AM
    if (invalid_vol_id == (blobReq->io_vol_id = vol_table->getVolumeUUID(blobReq->volume_name))) {
        vol_table->addBlobToWaitQueue(blobReq->volume_name, blobReq);
        fds_verify(sendTestBucketToOM(blobReq->volume_name,
                                      "",  // The access key isn't used
                                      "") == ERR_OK); // The secret key isn't used
        return;
    }

    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx);

    blobReq->io_req_id = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);

    fds_verify(qos_ctrl->enqueueIO(blobReq->io_vol_id, blobReq) == ERR_OK);
}

Error StorHvCtrl::putBlobSvc(fds::AmRequest *amReq)
{
    counters_.put_reqs.incr();

    fds::Error err(ERR_OK);

    // Pull out the blob request
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    ObjectID objId;
    bool fZeroSize = (blobReq->data_len == 0);
    fds_verify(blobReq->magicInUse() == true);

    // Get the volume context structure
    fds_volid_t   volId = blobReq->io_vol_id;
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        qos_ctrl->markIODone(amReq);
        blobReq->cbWithResult(FDSN_StatusErrorAccessDenied);
        delete blobReq;
        return FDSN_StatusErrorAccessDenied;
    }

    // TODO(Andrew): Here we're turning the offset aligned
    // blobOffset back into an absolute blob offset (i.e.,
    // not aligned to the maximum object size). This allows
    // the rest of the putBlob routines to still expect an
    // absolute offset in case we need it
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    blobReq->blob_offset = (blobReq->blob_offset * maxObjSize);

    // Track how long the request was queued before put() dispatch
    // TODO(Andrew): Consider moving to the QoS request
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    if (fZeroSize) {
        LOGWARN << "zero size object - "
                << " [objkey:" << blobReq->getBlobName() <<"]";
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(blobReq->hash_perf_ctx);
        objId = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                      blobReq->data_len);
    }
    blobReq->obj_id = objId;

    //TODO(matteo): maybe check other issueUpdateC...
    fds::PerfTracer::tracePointBegin(blobReq->dm_perf_ctx);

    if (blobReq->io_type == FDS_PUT_BLOB) {
        fds_uint64_t dmt_version;
        fds_verify(amTxMgr->getTxDmtVersion(*(blobReq->tx_desc), &dmt_version) == ERR_OK);
        // Update the tx manager with this update
        fds_verify(amTxMgr->updateStagedBlobDesc(*(blobReq->tx_desc),
                                                 blobReq->data_len) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(amTxMgr->updateStagedBlobOffset(*(blobReq->tx_desc),
                                                   blobReq->getBlobName(),
                                                   blobReq->blob_offset,
                                                   blobReq->obj_id) == ERR_OK);
        issueUpdateCatalogMsg(blobReq->obj_id,
                              blobReq->getBlobName(),
                              blobReq->blob_offset,
                              blobReq->data_len,
                              blobReq->last_buf,
                              volId,
                              blobReq->tx_desc->getValue(),
                              dmt_version,
                              RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobUpdateCatalogMsgResp, amReq));
    } else if(blobReq->io_type == FDS_PUT_BLOB_ONCE) {
        // Sending the update in a single request. Create transaction ID to
        // use for the single request
        BlobTxId::ptr txId = boost::make_shared<BlobTxId>(storHvisor->randNumGen->genNumSafe());
        blobReq->tx_desc = txId;

        // Track the updates in the tx manager
        fds_uint64_t dmt_version = om_client->getDMTVersion();
        fds_verify(amTxMgr->addTx(volId,
                                  *txId,
                                  dmt_version,
                                  blobReq->getBlobName()) == ERR_OK);
        // Stage the tx manager with this update's length
        fds_verify(amTxMgr->updateStagedBlobDesc(*txId,
                                                 blobReq->data_len) == ERR_OK);
        // Stage the transaction metadata changes
        fds_verify(amTxMgr->updateStagedBlobDesc(*txId,
                                                 blobReq->metadata) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(amTxMgr->updateStagedBlobOffset(*(blobReq->tx_desc),
                                                   blobReq->getBlobName(),
                                                   blobReq->blob_offset,
                                                   blobReq->obj_id) == ERR_OK);

        issueUpdateCatalogMsg(blobReq->obj_id,
                              blobReq->getBlobName(),
                              blobReq->blob_offset,
                              blobReq->data_len,
                              blobReq->blob_mode,
                              blobReq->metadata,
                              volId,
                              txId->getValue(),
                              RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobUpdateCatalogOnceMsgResp, amReq));
    } else {
        fds_panic("Unknown io_type request!");
    }

    // Send put request to SM
    fds::PerfTracer::tracePointBegin(blobReq->sm_perf_ctx);
    if (fZeroSize) {
        // If there's not object data to write, just update the
        // SM's response ack count
        blobReq->notifyResponse(qos_ctrl, err);
    } else {
        // Update the transaction manager with the stage object data
        // Removed since it needs to be shared_ptr and is unused...
        // fds_verify(amTxMgr->updateStagedBlobObject(*(blobReq->tx_desc),
        //                                        blobReq->obj_id,
        //                                         blobReq->getDataBuf(),
        //                                         blobReq->data_len)
        //         == ERR_OK);
        issuePutObjectMsg(blobReq->obj_id,
                          blobReq->getDataBuf(),
                          blobReq->data_len,
                          volId,
                          RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobPutObjectMsgResp, amReq));
    }

    // TODO(Rao): Check with andrew if this is the right place to unlock or
    // can we unlock before
    // TODO(Rao): Check if we can use scoped lock here
    shVol->readUnlock();
    return err;
}

void StorHvCtrl::issuePutObjectMsg(const ObjectID &objId,
                                   const char* dataBuf,
                                   const fds_uint64_t &len,
                                   const fds_volid_t& volId,
                                   QuorumSvcRequestRespCb respCb)
{
    if (len == 0) {
        fds_verify(!"Not impl");
        // TODO(Rao): Shortcut SM.  Issue a call back to indicate put successed
    }
    PutObjectMsgPtr putObjMsg(new PutObjectMsg);
    putObjMsg->volume_id = volId;
    putObjMsg->origin_timestamp = fds::util::getTimeStampMillis();
    putObjMsg->data_obj = std::string(dataBuf, len);
    putObjMsg->data_obj_len = len;
    putObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());

    auto asyncPutReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDLTNodesForDoidKey(objId)));
    asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
    asyncPutReq->onResponseCb(respCb);
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << fds::logString(*putObjMsg);
}

void StorHvCtrl::issueUpdateCatalogMsg(const ObjectID &objId,
                                       const std::string& blobName,
                                       const fds_uint64_t& blobOffset,
                                       const fds_uint64_t &len,
                                       const fds_int32_t &blobMode,
                                       boost::shared_ptr< std::map<std::string, std::string> > metadata,
                                       const fds_volid_t& volId,
                                       const fds_uint64_t& txId,
                                       QuorumSvcRequestRespCb respCb)
{
    UpdateCatalogOnceMsgPtr updCatMsg(new UpdateCatalogOnceMsg());
    updCatMsg->blob_name    = blobName;
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id    = volId;
    updCatMsg->txId         = txId;
    updCatMsg->blob_mode    = blobMode;
    updCatMsg->obj_list.clear();
    updCatMsg->meta_list.clear();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = blobOffset;
    updBlobInfo.size     = len;
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);

    // Setup blob metadata updates
    FDS_ProtocolInterface::FDSP_MetaDataPair metaDataPair;
    for (auto& meta : *metadata) {
        metaDataPair.key   = meta.first;
        metaDataPair.value = meta.second;
        updCatMsg->meta_list.push_back(metaDataPair);
    }

    // Always use the current DMT version since we're updating in a single request
    auto asyncUpdateCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId))));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg), updCatMsg);
    asyncUpdateCatReq->onResponseCb(respCb);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void StorHvCtrl::issueUpdateCatalogMsg(const ObjectID &objId,
                                       const std::string& blobName,
                                       const fds_uint64_t& blobOffset,
                                       const fds_uint64_t &len,
                                       const bool &lastBuf,
                                       const fds_volid_t& volId,
                                       const fds_uint64_t& txId,
                                       const fds_uint64_t& dmt_version,
                                       QuorumSvcRequestRespCb respCb)
{
    UpdateCatalogMsgPtr updCatMsg(new UpdateCatalogMsg());
    updCatMsg->blob_name   = blobName;
    updCatMsg->blob_version = blob_version_invalid;
    updCatMsg->volume_id = volId;

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    updCatMsg->obj_list.clear();
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = blobOffset;
    updBlobInfo.size     = len;
    updBlobInfo.blob_end = lastBuf;
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    // Add the offset info to the DM message
    updCatMsg->obj_list.push_back(updBlobInfo);
    updCatMsg->txId = txId;

    auto asyncUpdateCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId),
                                                                                  dmt_version)));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), updCatMsg);
    asyncUpdateCatReq->onResponseCb(respCb);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void StorHvCtrl::putBlobPutObjectMsgResp(fds::AmRequest* amReq,
                                         QuorumSvcRequest* svcReq,
                                         const Error& error,
                                         boost::shared_ptr<std::string> payload)
{
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(amReq);
    fds::PerfTracer::tracePointEnd(blobReq->sm_perf_ctx); 
    fpi::PutObjectRspMsgPtr putObjRsp =
        net::ep_deserialize<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->blob_offset << " Error: " << error; 
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*putObjRsp);
    }
    blobReq->notifyResponse(qos_ctrl, error);
}

void StorHvCtrl::putBlobUpdateCatalogMsgResp(fds::AmRequest* amReq,
                                             QuorumSvcRequest* svcReq,
                                             const Error& error,
                                             boost::shared_ptr<std::string> payload)
{
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(amReq);
    fds::PerfTracer::tracePointEnd(blobReq->dm_perf_ctx); 
    fpi::UpdateCatalogRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->blob_offset << " Error: " << error; 
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
    }
    blobReq->notifyResponse(qos_ctrl, error);
}

void
StorHvCtrl::putBlobUpdateCatalogOnceMsgResp(fds::AmRequest* amReq,
                                            QuorumSvcRequest* svcReq,
                                            const Error& error,
                                            boost::shared_ptr<std::string> payload) {
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(amReq);
    fds::PerfTracer::tracePointEnd(blobReq->dm_perf_ctx); 
    fpi::UpdateCatalogOnceRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogOnceRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->obj_id
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->blob_offset << " Error: " << error; 
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*updCatRsp);
    }
    blobReq->notifyResponse(qos_ctrl, error);
}

fds::Error
StorHvCtrl::deleteBlobSvc(fds::AmRequest *amReq)
{
    Error err = ERR_OK;
    // Pull out the delete request

    fds::DeleteBlobReq *blobReq = static_cast<fds::DeleteBlobReq *>(amReq);
    fds_verify(blobReq->magicInUse() == true);

    // Get volume id
    fds_volid_t vol_id = blobReq->io_vol_id;

    // Get blob name
    std::string blob_name = blobReq->getBlobName();

    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(vol_id);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "delete blob on a snapshot is not allowed.";
        shVol->readUnlock();
        return FDSN_StatusErrorAccessDenied;
    }

    // Send to the DM
    fds::PerfTracer::tracePointBegin(blobReq->dm_perf_ctx); 
    issueDeleteCatalogObject(vol_id, blob_name,
                             RESPONSE_MSG_HANDLER(StorHvCtrl::deleteObjectMsgResp, amReq));
    return err;
}

void
StorHvCtrl::issueDeleteCatalogObject(const fds_uint64_t& vol_id,
                                     const std::string& blob_name,
                                     QuorumSvcRequestRespCb respCb)
{
    DeleteCatalogObjectMsgPtr delMsg(new DeleteCatalogObjectMsg());
    delMsg->blob_name = blob_name;
    delMsg->blob_version = blob_version_invalid;
    delMsg->volume_id = vol_id;

    
    auto asyncDeleteCatReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(vol_id))));
    asyncDeleteCatReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteCatalogObjectMsg), delMsg);
    asyncDeleteCatReq->onResponseCb(respCb);
    asyncDeleteCatReq->invoke();

    LOGDEBUG << asyncDeleteCatReq->logString() << fds::logString(*delMsg);
}

void StorHvCtrl::deleteObjectMsgResp(fds::AmRequest* amReq,
                                      QuorumSvcRequest* svcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload)
{
    DeleteBlobReq *blobReq = static_cast<fds::DeleteBlobReq*>(amReq);
    fds::PerfTracer::tracePointEnd(blobReq->dm_perf_ctx); 
    fpi::DeleteCatalogObjectRspMsgPtr delCatRsp =
        net::ep_deserialize<fpi::DeleteCatalogObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->blob_offset << " Error: " << error; 
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*delCatRsp);
    }
    
    qos_ctrl->markIODone(amReq);
    blobReq->cb->call(error.GetErrno());
    delete blobReq;
}
