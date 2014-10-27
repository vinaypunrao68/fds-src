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
    if (vol_table->volumeExists(volumeName)) {
        volId = storHvisor->vol_table->getVolumeUUID(volumeName);
        fds_verify(volId != invalid_vol_id);
        LOGDEBUG << "Volume " << volumeName
                 << " with UUID " << volId
                 << " already attached";
        cb->call(ERR_OK);
        return;
    }

    // Create a noop request to put into wait queue
    AttachVolBlobReq *blobReq =
            new AttachVolBlobReq(volId,
                                 volumeName,
                                 "",  // No blob name
                                 0,  // No blob offset
                                 0,  // No data length
                                 NULL,  // No buffer
                                 cb);

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
    /*
     * Pack the blobReq in to a qosReq to pass to QoS
     */
    fds_uint32_t reqId = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
    AmQosReq *qosReq  = new AmQosReq(blobReq, reqId);
    fds_volid_t volId = blobReq->vol_id;

    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    if ((shVol == NULL) || (shVol->volQueue == NULL)) {
        if (shVol)
            shVol->readUnlock();
        LOGERROR << "Volume and queueus are NOT setup for volume " << volId;
        err = ERR_INVALID_ARG;
        PerfTracer::tracePointEnd(blobReq->qos_perf_ctx); 
        delete qosReq;
        return err;
    }
    /*
     * TODO: We should handle some sort of success/failure here?
     */
    qos_ctrl->enqueueIO(volId, qosReq);
    shVol->readUnlock();

    LOGDEBUG << "Queued IO for vol " << volId;

    return err;
}

void
StorHvCtrl::enqueueBlobReq(AmRequest *blobReq) {
    fds_verify(blobReq->magicInUse() == true);

    // check if volume is attached to this AM
    if (vol_table->volumeExists(blobReq->volume_name)) {
        fds_volid_t volId = vol_table->getVolumeUUID(blobReq->volume_name);
        fds_verify(volId != invalid_vol_id);
        // Set the vol id because we may have only know the volume name
        blobReq->vol_id = volId;
    } else {
        vol_table->addBlobToWaitQueue(blobReq->volume_name, blobReq);
        fds_verify(sendTestBucketToOM(blobReq->volume_name,
                                      "",  // The access key isn't used
                                      "") == ERR_OK); // The secret key isn't used
        return;
    }

    PerfTracer::tracePointBegin(blobReq->e2e_req_perf_ctx);
    PerfTracer::tracePointBegin(blobReq->qos_perf_ctx);

    fds_uint32_t reqId = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
    // Pack the blobReq in to a qosReq to pass to QoS
    AmQosReq *qosReq = new AmQosReq(blobReq, reqId);

    fds_verify(qos_ctrl->enqueueIO(blobReq->vol_id, qosReq) == ERR_OK);
}

Error
StorHvCtrl::abortBlobTxSvc(AmQosReq *qosReq) {
    fds_verify(qosReq != NULL);

    Error err(ERR_OK);
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_ABORT_BLOB_TX);

    fds_volid_t   volId = blobReq->vol_id;
   
    issueAbortBlobTxMsg(blobReq->getBlobName(),
                        volId,
                        blobReq->tx_desc->getValue(),
                        RESPONSE_MSG_HANDLER(StorHvCtrl::abortBlobTxMsgResp, qosReq));
    return err;
}


void StorHvCtrl::issueAbortBlobTxMsg(const std::string& blobName,
                                     const fds_volid_t& volId,
                                     const fds_uint64_t& txId,
                                     QuorumSvcRequestRespCb respCb)
{

    AbortBlobTxMsgPtr stBlobTxMsg(new AbortBlobTxMsg());
    stBlobTxMsg->blob_name   = blobName;
    stBlobTxMsg->blob_version = blob_version_invalid;
    stBlobTxMsg->volume_id = volId;


    stBlobTxMsg->txId = txId;

    auto asyncAbortBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId))));
    asyncAbortBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::AbortBlobTxMsg), stBlobTxMsg);
    asyncAbortBlobTxReq->onResponseCb(respCb);
    asyncAbortBlobTxReq->invoke();

    LOGDEBUG << asyncAbortBlobTxReq->logString() << fds::logString(*stBlobTxMsg);

}


void StorHvCtrl::abortBlobTxMsgResp(fds::AmQosReq* qosReq,
                                    QuorumSvcRequest* svcReq,
                                    const Error& error,
                                    boost::shared_ptr<std::string> payload)
{
    fds_verify(qosReq != NULL);
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_ABORT_BLOB_TX);

    // AbortBlobTxCallback::ptr cb = SHARED_DYN_CAST(AbortBlobTxCallback,
    //                                              blobReq->cb);
    qos_ctrl->markIODone(qosReq);
    blobReq->cb->call(error.GetErrno());

    fds_verify(amTxMgr->removeTx(*(blobReq->tx_desc)) == ERR_OK);
    delete blobReq;
}


Error
StorHvCtrl::commitBlobTxSvc(AmQosReq *qosReq) {
    fds_verify(qosReq != NULL);

    Error err(ERR_OK);
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    fds_uint64_t dmt_version;
    err = amTxMgr->getTxDmtVersion(*(blobReq->tx_desc), &dmt_version);
    fds_verify(err == ERR_OK);

    issueCommitBlobTxMsg(blobReq, dmt_version,
                         RESPONSE_MSG_HANDLER(StorHvCtrl::commitBlobTxMsgResp, qosReq));
    return err;
}


void StorHvCtrl::issueCommitBlobTxMsg(CommitBlobTxReq *blobReq,
                                      fds_uint64_t dmtVersion,
                                      QuorumSvcRequestRespCb respCb)
{

    CommitBlobTxMsgPtr stBlobTxMsg(new CommitBlobTxMsg());
    stBlobTxMsg->blob_name   = blobReq->getBlobName();
    stBlobTxMsg->blob_version = blob_version_invalid;
    stBlobTxMsg->volume_id = blobReq->vol_id;
    stBlobTxMsg->dmt_version = dmtVersion;

    fds_volid_t volId = blobReq->vol_id;
    stBlobTxMsg->txId = blobReq->tx_desc->getValue();

    auto asyncCommitBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId),
                                                                                  dmtVersion)));
    asyncCommitBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg), stBlobTxMsg);
    asyncCommitBlobTxReq->onResponseCb(respCb);
    asyncCommitBlobTxReq->invoke();

    LOGDEBUG << asyncCommitBlobTxReq->logString() << fds::logString(*stBlobTxMsg);

}


void StorHvCtrl::commitBlobTxMsgResp(fds::AmQosReq* qosReq,
                                    QuorumSvcRequest* svcReq,
                                    const Error& error,
                                    boost::shared_ptr<std::string> payload)
{
    fds_verify(qosReq != NULL);
    CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_COMMIT_BLOB_TX);

    // Push the commited update to the cache and remove from manager
    // TODO(Andrew): Inserting the entire tx transaction currently
    // assumes that the tx descriptor has all of the contents needed
    // for a blob descriptor (e.g., size, version, etc..). Today this
    // is true for S3/Swift and doesn't get used anyways for block (so
    // the actual cached descriptor for block will not be correct).
    AmTxDescriptor::ptr txDesc;
    fds_verify(amTxMgr->getTxDescriptor(*(blobReq->tx_desc),
                                        txDesc) == ERR_OK);
    fds_verify(amCache->putTxDescriptor(txDesc) == ERR_OK);
    fds_verify(amTxMgr->removeTx(*(blobReq->tx_desc)) == ERR_OK);

    qos_ctrl->markIODone(qosReq);
    blobReq->cb->call(error);

    delete blobReq;
}

Error
StorHvCtrl::startBlobTxSvc(AmQosReq *qosReq) {
    fds_verify(qosReq != NULL);

    Error err(ERR_OK);
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    fds_volid_t   volId = blobReq->vol_id;
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback, blobReq->cb);
        qos_ctrl->markIODone(qosReq);
        cb->call(FDSN_StatusErrorAccessDenied);
        delete blobReq;
        return FDSN_StatusErrorAccessDenied;
    }

    // Generate a random transaction ID to use
    // Note: construction, generates a random ID
    BlobTxId::ptr txId = boost::make_shared<BlobTxId>(storHvisor->randNumGen->genNumSafe());

    // Stash the newly created ID in the callback for later
    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  blobReq->cb);
    cb->blobTxId  = *txId;

    // Update blob request with new fields
    blobReq->tx_desc = txId;
    blobReq->dmtVersion = om_client->getDMTVersion();

    // Track the transaction we're starting. If the DM
    // request fails, we'll drop this entry from the mgr.
    err = amTxMgr->addTx(volId, *txId, blobReq->dmtVersion, blobReq->getBlobName());
    fds_verify(err == ERR_OK);

    // amDispatcher->dispatchStartBlobTx(qosReq);

    issueStartBlobTxMsg(blobReq->getBlobName(),
                        volId,
                        blobReq->blob_mode,
                        txId->getValue(),
                        blobReq->dmtVersion,
                        RESPONSE_MSG_HANDLER(StorHvCtrl::startBlobTxMsgResp, qosReq));
    return err;
}


void StorHvCtrl::issueStartBlobTxMsg(const std::string& blobName,
                                     const fds_volid_t& volId,
                                     const fds_int32_t blobMode,
                                     const fds_uint64_t& txId,
                                     const fds_uint64_t dmtVer,
                                     QuorumSvcRequestRespCb respCb)
{

    StartBlobTxMsgPtr stBlobTxMsg(new StartBlobTxMsg());
    stBlobTxMsg->blob_name   = blobName;
    stBlobTxMsg->blob_version = blob_version_invalid;
    stBlobTxMsg->volume_id = volId;
    stBlobTxMsg->blob_mode = blobMode;
    stBlobTxMsg->txId = txId;
    stBlobTxMsg->dmt_version = dmtVer;

    auto asyncStartBlobTxReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId))));
    asyncStartBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg), stBlobTxMsg);
    asyncStartBlobTxReq->onResponseCb(respCb);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString() << fds::logString(*stBlobTxMsg);

}


void StorHvCtrl::startBlobTxMsgResp(fds::AmQosReq* qosReq,
                                    QuorumSvcRequest* svcReq,
                                    const Error& error,
                                    boost::shared_ptr<std::string> payload)
{
    fds_verify(qosReq != NULL);
    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_START_BLOB_TX);

    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                      blobReq->cb);
    qos_ctrl->markIODone(qosReq);
    cb->call(error.GetErrno());
    delete blobReq;
}


Error StorHvCtrl::putBlobSvc(fds::AmQosReq *qosReq)
{
    counters_.put_reqs.incr();

    fds::Error err(ERR_OK);

    // Pull out the blob request
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(qosReq->getBlobReqPtr());
    ObjectID objId;
    bool fZeroSize = (blobReq->data_len == 0);
    fds_verify(blobReq->magicInUse() == true);

    // Get the volume context structure
    fds_volid_t   volId = blobReq->vol_id;
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    // check if this is a snapshot
    if (shVol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        shVol->readUnlock();
        qos_ctrl->markIODone(qosReq);
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
                << " [objkey:" << blobReq->ObjKey <<"]";
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(blobReq->hash_perf_ctx);
        objId = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                      blobReq->data_len);
    }
    blobReq->obj_id = objId;

    //TODO(matteo): maybe check other issueUpdateC...
    fds::PerfTracer::tracePointBegin(blobReq->dm_perf_ctx);

    if (blobReq->getIoType() == FDS_PUT_BLOB) {
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
                              blobReq->isLastBuf(),
                              volId,
                              blobReq->tx_desc->getValue(),
                              dmt_version,
                              RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobUpdateCatalogMsgResp, qosReq));
    } else if(blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
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
                              blobReq->blobMode,
                              blobReq->metadata,
                              volId,
                              txId->getValue(),
                              RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobUpdateCatalogOnceMsgResp, qosReq));
    } else {
        fds_panic("Unknown io_type request!");
    }

    // Send put request to SM
    fds::PerfTracer::tracePointBegin(blobReq->sm_perf_ctx);
    if (fZeroSize) {
        // If there's not object data to write, just update the
        // SM's response ack count
        blobReq->notifyResponse(qos_ctrl, qosReq, err);
    } else {
        // Update the transaction manager with the stage object data
        fds_verify(amTxMgr->updateStagedBlobObject(*(blobReq->tx_desc),
                                                   blobReq->obj_id,
                                                   blobReq->getDataBuf(),
                                                   blobReq->data_len)
                   == ERR_OK);
        issuePutObjectMsg(blobReq->obj_id,
                          blobReq->getDataBuf(),
                          blobReq->data_len,
                          volId,
                          RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobPutObjectMsgResp, qosReq));
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

void StorHvCtrl::putBlobPutObjectMsgResp(fds::AmQosReq* qosReq,
                                         QuorumSvcRequest* svcReq,
                                         const Error& error,
                                         boost::shared_ptr<std::string> payload)
{
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
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
    blobReq->notifyResponse(qos_ctrl, qosReq, error);
}

void StorHvCtrl::putBlobUpdateCatalogMsgResp(fds::AmQosReq* qosReq,
                                             QuorumSvcRequest* svcReq,
                                             const Error& error,
                                             boost::shared_ptr<std::string> payload)
{
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
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
    blobReq->notifyResponse(qos_ctrl, qosReq, error);
}

void
StorHvCtrl::putBlobUpdateCatalogOnceMsgResp(fds::AmQosReq* qosReq,
                                            QuorumSvcRequest* svcReq,
                                            const Error& error,
                                            boost::shared_ptr<std::string> payload) {
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
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
    blobReq->notifyResponse(qos_ctrl, qosReq, error);
}


Error StorHvCtrl::getBlobSvc(fds::AmQosReq *qosReq)
{
    counters_.get_reqs.incr();

    /*
     * Pull out the blob request
     */
    Error err = ERR_OK;
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(
        qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    fds_volid_t   volId = blobReq->vol_id;
    StorHvVolume *shVol = vol_table->getVolume(volId);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        LOGCRITICAL << "getBlob failed to get volume for vol "
                    << volId;    
        qos_ctrl->markIODone(qosReq);
        blobReq->cb->call(ERR_INVALID);
        delete blobReq;
        err = ERR_DISK_WRITE_FAILED;
        return err;
    }

    // TODO(Anna) We are doing update catalog using absolute
    // offsets, so we need to be consistent in query catalog
    // Review this in the next sprint!
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    blobReq->blob_offset = (blobReq->blob_offset * maxObjSize);

    // Check cache for object ID
    ObjectID::ptr objectId = amCache->getBlobOffsetObject(volId,
                                                          blobReq->getBlobName(),
                                                          blobReq->blob_offset,
                                                          err);
    if (err != ERR_OK) {
        fds::PerfTracer::tracePointBegin(blobReq->dm_perf_ctx); 
        issueQueryCatalog(blobReq->getBlobName(),
                          blobReq->blob_offset,
                          volId,
                          RESPONSE_MSG_HANDLER(StorHvCtrl::getBlobQueryCatalogResp, qosReq));
    } else {
        // TODO(Andrew): Consider adding this back when we revisit
        // zero length objects
        // fds_verify(*objectId != NullObjectID);
        blobReq->obj_id = *objectId;
        fds::PerfTracer::tracePointBegin(blobReq->sm_perf_ctx); 
        issueGetObject(qosReq,
                       RESPONSE_MSG_HANDLER(StorHvCtrl::getBlobGetObjectResp,
                                            qosReq));
    }
    return ERR_OK;
}

void StorHvCtrl::issueQueryCatalog(const std::string& blobName,
                       const fds_uint64_t& blobOffset,
                       const fds_volid_t& volId,
                       FailoverSvcRequestRespCb respCb)
{
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
    queryMsg->end_offset = -1;
    // We don't currently specify a version
    queryMsg->blob_version = blob_version_invalid;
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();

    auto asyncQueryReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId))));
    asyncQueryReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), queryMsg);
    asyncQueryReq->onResponseCb(respCb);
    asyncQueryReq->invoke();

    LOGDEBUG << asyncQueryReq->logString() << fds::logString(*queryMsg);
}

void StorHvCtrl::issueGetObject(AmQosReq *qosReq,
                                FailoverSvcRequestRespCb respCb)
{
    Error error(ERR_OK);
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(
        qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);
    fds_volid_t volId = blobReq->vol_id;
    ObjectID objId    = blobReq->obj_id;

    // TODO(Andrew): This is a hack to handle reading zero length blobs.
    // The DM or AM cache is going to return a invalid object Id (i.e., 0)
    // the represents an empty blob. Here's we're just going to set the data
    // length to 0 and return. We should figure out how we want this flow to
    // actually go (the entire read API and path should be improved).
    if (objId == NullObjectID) {
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback,
                                                    blobReq->cb);
        cb->returnSize = 0;
        cb->call(error);
        qos_ctrl->markIODone(qosReq);
        delete blobReq;
        return;
    }

    // Check cache for object data
    boost::shared_ptr<std::string> objectData =
            amCache->getBlobObject(volId,
                                   objId,
                                   error);
    if (error == ERR_OK) {
        LOGTRACE << "Found cached object " << objId;
        // Data was found in cache, so fill data and callback
        qos_ctrl->markIODone(qosReq);
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback,
                                                    blobReq->cb);
        // Set the return size based on what was requested
        if (blobReq->data_len < static_cast<fds_uint64_t>(objectData->size())) {
            LOGDEBUG  << "Returning " << blobReq->data_len << " byte subset of "
                      << objectData->size() << " bytes of data";
            cb->returnSize = blobReq->data_len;
        } else {
            cb->returnSize = objectData->size();
        }
        memcpy(cb->returnBuffer,
               objectData->c_str(),
               cb->returnSize);
        cb->call(error);
        delete blobReq;
        return;
    }

    fpi::GetObjectMsgPtr getObjMsg(boost::make_shared<fpi::GetObjectMsg>());
    getObjMsg->volume_id = volId;
    getObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(),
                                                (size_t)objId.GetLen());

    auto asyncGetReq = gSvcRequestPool->newFailoverSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDLTNodesForDoidKey(objId)));
    asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
    asyncGetReq->onResponseCb(respCb);
    asyncGetReq->invoke();

    LOGDEBUG << asyncGetReq->logString() << fds::logString(*getObjMsg);
}

void StorHvCtrl::getBlobQueryCatalogResp(fds::AmQosReq* qosReq,
                                         FailoverSvcRequest* svcReq,
                                         const Error& error,
                                         boost::shared_ptr<std::string> payload)
{
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(qosReq->getBlobReqPtr());
    fpi::QueryCatalogMsgPtr qryCatRsp =
        net::ep_deserialize<fpi::QueryCatalogMsg>(const_cast<Error&>(error), payload);

    fds::PerfTracer::tracePointEnd(blobReq->dm_perf_ctx); 

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->blob_offset << " Error: " << error; 
        qos_ctrl->markIODone(qosReq);
        // TODO(Andrew): We should change XDI to not expect OFFSET_INVALID, rather NOT_FOUND
        if (error == ERR_CAT_ENTRY_NOT_FOUND) {
            blobReq->cb->call(ERR_BLOB_OFFSET_INVALID);
        } else {
            blobReq->cb->call(error);
        }
        delete blobReq;
        return;
    }

    LOGDEBUG << svcReq->logString() << fds::logString(*qryCatRsp);

    // TODO(Andrew): Update the AM's blob offset cache here

    // TODO(xxx) should be able to have multiple object id + implement range
    // queries in DM
    for (fpi::FDSP_BlobObjectList::const_iterator it = qryCatRsp->obj_list.cbegin();
         it != qryCatRsp->obj_list.cend();
         it++) {
        if ((fds_uint64_t)(*it).offset == blobReq->blob_offset) {
            // found offset!!!
            ObjectID objId((*it).data_obj_id.digest);
            // TODO(Andrew): Consider adding this back when we revisit
            // zero length objects
            // fds_verify(objId != NullObjectID);
            
            blobReq->obj_id = objId;
            fds::PerfTracer::tracePointBegin(blobReq->sm_perf_ctx); 
            issueGetObject(qosReq,
                           RESPONSE_MSG_HANDLER(StorHvCtrl::getBlobGetObjectResp,
                                                qosReq));

            return;
        }
    }

    // if we are here, we did not get response for offset we needed!
    LOGDEBUG << "blob name: " << blobReq->getBlobName() << "offset: "
             << blobReq->blob_offset << " not in returned offset list from DM";
    qos_ctrl->markIODone(qosReq);
    blobReq->cb->call(ERR_BLOB_OFFSET_INVALID);
    delete blobReq;
    return;
}

void StorHvCtrl::getBlobGetObjectResp(fds::AmQosReq* qosReq,
                                      FailoverSvcRequest* svcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload)
{
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(qosReq->getBlobReqPtr());
    fpi::GetObjectRespPtr getObjRsp =
        net::ep_deserialize<fpi::GetObjectResp>(const_cast<Error&>(error), payload);
    fds::PerfTracer::tracePointEnd(blobReq->sm_perf_ctx); 

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->blob_offset << " Error: " << error; 
        qos_ctrl->markIODone(qosReq);
        blobReq->cb->call(error);
        delete blobReq;
        return;
    }

    // TODO(Andrew): Update the AM's blob object cache here

    LOGDEBUG << svcReq->logString() << fds::logString(*getObjRsp);

    qos_ctrl->markIODone(qosReq);

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
        fds_verify((uint)(getObjRsp->data_obj.size()) <= (blobReq->data_len));
    }
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback,
                                                blobReq->cb);
    // Set the return size based on what was requested
    if (blobReq->data_len < static_cast<fds_uint64_t>(getObjRsp->data_obj.size())) {
        LOGDEBUG  << "Returning " << blobReq->data_len << " byte subset of "
                  << getObjRsp->data_obj_len << " bytes of data";
        cb->returnSize = blobReq->data_len;
    } else {
        cb->returnSize = getObjRsp->data_obj.size();
    }
    memcpy(cb->returnBuffer,
           getObjRsp->data_obj.c_str(),
           cb->returnSize);

    cb->call(error);
    delete blobReq;
}

fds::Error
StorHvCtrl::deleteBlobSvc(fds::AmQosReq *qosReq)
{
    Error err = ERR_OK;
    // Pull out the delete request

    fds::DeleteBlobReq *blobReq = static_cast<fds::DeleteBlobReq *>(
        qosReq->getBlobReqPtr());
    fds_verify(blobReq->magicInUse() == true);

    // Get volume id
    fds_volid_t vol_id = blobReq->vol_id;

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
                             RESPONSE_MSG_HANDLER(StorHvCtrl::deleteObjectMsgResp, qosReq));
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

void StorHvCtrl::deleteObjectMsgResp(fds::AmQosReq* qosReq,
                                      QuorumSvcRequest* svcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload)
{
    DeleteBlobReq *blobReq = static_cast<fds::DeleteBlobReq*>(qosReq->getBlobReqPtr());
    fds::PerfTracer::tracePointEnd(blobReq->dm_perf_ctx); 
    fpi::DeleteCatalogObjectRspMsgPtr delCatRsp =
        net::ep_deserialize<fpi::DeleteCatalogObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->blob_offset << " Error: " << error; 
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*delCatRsp);
    }
    
    qos_ctrl->markIODone(qosReq);
    blobReq->cb->call(error.GetErrno());
    delete blobReq;
}

fds::Error
StorHvCtrl::getVolumeMetaDataSvc(fds::AmQosReq* qosReq) {
    StorHvCtrl::RequestHelper helper(storHvisor, qosReq);

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }

    GetVolumeMetaDataReq* volMDReq = TO_DERIVED(GetVolumeMetaDataReq, helper.blobReq);
    fds_assert(volMDReq && volMDReq->magicInUse());

    fpi::GetVolumeMetaDataMsgPtr volMDMsg(new fpi::GetVolumeMetaDataMsg());
    volMDMsg->volume_id = helper.volId;

    auto asyncQueryReq = gSvcRequestPool->newFailoverSvcRequest(
            boost::make_shared<DmtVolumeIdEpProvider>(
                om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(helper.volId))));
    asyncQueryReq->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeMetaDataMsg), volMDMsg);
    FailoverSvcRequestRespCb respCb =
            RESPONSE_MSG_HANDLER(StorHvCtrl::getVolumeMetaDataMsgResp, qosReq);
    asyncQueryReq->onResponseCb(respCb);
    asyncQueryReq->invoke();

    return ERR_OK;
}

void StorHvCtrl::getVolumeMetaDataMsgResp(fds::AmQosReq* qosReq,
                                          FailoverSvcRequest* svcReq,
                                          const Error& error,
                                          boost::shared_ptr<std::string> payload) {
    GetVolumeMetaDataReq * volMDReq =
            static_cast<fds::GetVolumeMetaDataReq*>(qosReq->getBlobReqPtr());
    fpi::GetVolumeMetaDataMsgPtr volMDMsg =
        net::ep_deserialize<fpi::GetVolumeMetaDataMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Get metadata volume name: " << volMDReq->volume_name
                << " Error: " << error;
        qos_ctrl->markIODone(qosReq);
        volMDReq->cb->call(error);
        delete volMDReq;
        return;
    }

    GetVolumeMetaDataCallback::ptr cb =
            SHARED_DYN_CAST(GetVolumeMetaDataCallback, volMDReq->cb);
    cb->volumeMetaData = volMDMsg->volume_meta_data;
    cb->call(error);
    qos_ctrl->markIODone(qosReq);
    delete volMDReq;
}

fds::Error
StorHvCtrl::setBlobMetaDataSvc(fds::AmQosReq* qosReq)
{
    fds_verify(qosReq != NULL);
    LOGDEBUG << "processing SetBlobMetaData for vol:" << qosReq->io_vol_id;
    Error err(ERR_OK);
    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->cb);
    fds_verify(blobReq->magicInUse() == true);

    // Stage the transaction metadata changes
    fds_verify(amTxMgr->updateStagedBlobDesc(*(blobReq->tx_desc),
                                             blobReq->getMetaDataListPtr()) == ERR_OK);

    fds_volid_t   vol_id = blobReq->vol_id;

    std::string blob_name = blobReq->getBlobName();
    fds_uint64_t dmt_version;
    err = amTxMgr->getTxDmtVersion(*(blobReq->tx_desc), &dmt_version);
    fds_verify(err == ERR_OK);
 
    LOGDEBUG << " Invoking issueSetBlobMetaData";
    issueSetBlobMetaData(vol_id, blob_name, blob_version_invalid, blobReq->getMetaDataListPtr(),
                         blobReq->tx_desc->getValue(), dmt_version,
                         RESPONSE_MSG_HANDLER(StorHvCtrl::setBlobMetaDataMsgResp, qosReq));
    return err;
}

void
StorHvCtrl::issueSetBlobMetaData(const fds_volid_t& vol_id,
                                 const std::string& blob_name,
                                 const blob_version_t& blob_version,
                                 const boost::shared_ptr<FDSP_MetaDataList>& md_list,
                                 const fds_uint64_t& txId,
                                 fds_uint64_t dmt_version,
                                 QuorumSvcRequestRespCb respCb)
{
    LOGDEBUG << " inside issueSetBlobMetaData";
    SetBlobMetaDataMsgPtr setMDMsg(new SetBlobMetaDataMsg());
    setMDMsg->blob_name = blob_name;
    setMDMsg->blob_version = blob_version;
    setMDMsg->volume_id = vol_id;
    setMDMsg->txId = txId;
    setMDMsg->metaDataList.clear();

    fds_verify(md_list != nullptr);
    fds_verify(respCb != nullptr);
    
    setMDMsg->metaDataList = std::move(*md_list);

    LOGDEBUG << " Invoking  Message Interface";
    auto asyncSetMDReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(vol_id),
                                                                                  dmt_version)));
    asyncSetMDReq->setPayload(FDSP_MSG_TYPEID(fpi::SetBlobMetaDataMsg), setMDMsg);
    asyncSetMDReq->onResponseCb(respCb);
    asyncSetMDReq->invoke();

     // LOGDEBUG << asyncSetMDReq->logString() << fds::logString(*setMDMsg);
}

void
StorHvCtrl::setBlobMetaDataMsgResp(fds::AmQosReq* qosReq,
                                   QuorumSvcRequest* svcReq,
                                   const Error& error,
                                   boost::shared_ptr<std::string> payload)
{
    SetBlobMetaDataReq *blobReq = static_cast<fds::SetBlobMetaDataReq*>(qosReq->getBlobReqPtr());
    fpi::SetBlobMetaDataRspMsgPtr setMDRsp =
        net::ep_deserialize<fpi::SetBlobMetaDataRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Set metadata blob name: " << blobReq->getBlobName() << " Error: " << error; 
    } else {
        LOGDEBUG << svcReq->logString() << fds::logString(*setMDRsp);
    }
    qos_ctrl->markIODone(qosReq);
    blobReq->cb->call(error.GetErrno());
    delete blobReq;
}
