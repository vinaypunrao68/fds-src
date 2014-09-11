/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include "StorHvisorCPP.h"
#include "hvisor_lib.h"
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

extern StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;

Error
StorHvCtrl::abortBlobTxSvc(AmQosReq *qosReq) {
    fds_verify(qosReq != NULL);

    Error err(ERR_OK);
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->magicInUse() == true);
    fds_verify(blobReq->getIoType() == FDS_ABORT_BLOB_TX);

    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);
   
    issueAbortBlobTxMsg(blobReq->getBlobName(),
                        volId,
                        blobReq->getTxId()->getValue(),
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
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId))));
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

    fds_verify(amTxMgr->removeTx(*(blobReq->getTxId())) == ERR_OK);
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

    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    fds_uint64_t dmt_version;
    err = amTxMgr->getTxDmtVersion(*(blobReq->getTxId()), &dmt_version);
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
    stBlobTxMsg->volume_id = blobReq->getVolId();
    stBlobTxMsg->dmt_version = dmtVersion;

    fds_volid_t volId = blobReq->getVolId();
    stBlobTxMsg->txId = blobReq->getTxId()->getValue();

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
    fds_verify(amTxMgr->getTxDescriptor(*(blobReq->getTxId()),
                                        txDesc) == ERR_OK);
    fds_verify(amCache->putTxDescriptor(txDesc) == ERR_OK);
    fds_verify(amTxMgr->removeTx(*(blobReq->getTxId())) == ERR_OK);

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

    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);
   
    // Generate a random transaction ID to use
    // Note: construction, generates a random ID
    BlobTxId txId(storHvisor->randNumGen->genNumSafe());
    // Stash the newly created ID in the callback for later
    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  blobReq->cb);
    cb->blobTxId = txId;
    fds_uint64_t dmt_version = om_client->getDMTVersion();

    // Track the transaction we're starting. If the DM
    // request fails, we'll drop this entry from the mgr.
    err = amTxMgr->addTx(volId, txId, dmt_version, blobReq->getBlobName());
    fds_verify(err == ERR_OK);

    issueStartBlobTxMsg(blobReq->getBlobName(),
                        volId,
                        blobReq->getBlobMode(),
                        txId.getValue(),
                        dmt_version,
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
    bool fZeroSize = (blobReq->getDataLen() == 0);
    fds_verify(blobReq->magicInUse() == true);

    // Get the volume context structure
    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    // TODO(Andrew): Here we're turning the offset aligned
    // blobOffset back into an absolute blob offset (i.e.,
    // not aligned to the maximum object size). This allows
    // the rest of the putBlob routines to still expect an
    // absolute offset in case we need it
    fds_uint32_t maxObjSize = shVol->voldesc->maxObjSizeInBytes;
    blobReq->setBlobOffset(blobReq->getBlobOffset() * maxObjSize);

    // Track how long the request was queued before put() dispatch
    // TODO(Andrew): Consider moving to the QoS request
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    if (fZeroSize) {
        LOGWARN << "zero size object - "
                << " [objkey:" << blobReq->ObjKey <<"]";
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(blobReq->hashPerfCtx);
        objId = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                      blobReq->getDataLen());
    }
    blobReq->setObjId(objId);

    //TODO(matteo): maybe check other issueUpdateC...
    fds::PerfTracer::tracePointBegin(blobReq->dmPerfCtx);

    if (blobReq->getIoType() == FDS_PUT_BLOB) {
        fds_uint64_t dmt_version;
        fds_verify(amTxMgr->getTxDmtVersion(*(blobReq->getTxId()), &dmt_version) == ERR_OK);
        // Update the tx manager with this update
        fds_verify(amTxMgr->updateStagedBlobDesc(*(blobReq->getTxId()),
                                                 blobReq->getDataLen()) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(amTxMgr->updateStagedBlobOffset(*(blobReq->getTxId()),
                                                   blobReq->getBlobName(),
                                                   blobReq->getBlobOffset(),
                                                   blobReq->getObjId()) == ERR_OK);
        issueUpdateCatalogMsg(blobReq->getObjId(),
                              blobReq->getBlobName(),
                              blobReq->getBlobOffset(),
                              blobReq->getDataLen(),
                              blobReq->isLastBuf(),
                              volId,
                              blobReq->getTxId()->getValue(),
                              dmt_version,
                              RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobUpdateCatalogMsgResp, qosReq));
    } else if(blobReq->getIoType() == FDS_PUT_BLOB_ONCE) {
        // Sending the update in a single request. Create transaction ID to
        // use for the single request
        BlobTxId txId(storHvisor->randNumGen->genNumSafe());
        blobReq->setTxId(txId);

        // Track the updates in the tx manager
        fds_uint64_t dmt_version = om_client->getDMTVersion();
        fds_verify(amTxMgr->addTx(volId,
                                  txId,
                                  dmt_version,
                                  blobReq->getBlobName()) == ERR_OK);
        // Stage the tx manager with this update's length
        fds_verify(amTxMgr->updateStagedBlobDesc(txId,
                                                 blobReq->getDataLen()) == ERR_OK);
        // Stage the transaction metadata changes
        fds_verify(amTxMgr->updateStagedBlobDesc(txId,
                                                 blobReq->metadata) == ERR_OK);
        // Update the transaction manager with the stage offset update
        fds_verify(amTxMgr->updateStagedBlobOffset(*(blobReq->getTxId()),
                                                   blobReq->getBlobName(),
                                                   blobReq->getBlobOffset(),
                                                   blobReq->getObjId()) == ERR_OK);

        issueUpdateCatalogMsg(blobReq->getObjId(),
                              blobReq->getBlobName(),
                              blobReq->getBlobOffset(),
                              blobReq->getDataLen(),
                              blobReq->blobMode,
                              blobReq->metadata,
                              volId,
                              txId.getValue(),
                              RESPONSE_MSG_HANDLER(StorHvCtrl::putBlobUpdateCatalogOnceMsgResp, qosReq));
    } else {
        fds_panic("Unknown io_type request!");
    }

    // Send put request to SM
    fds::PerfTracer::tracePointBegin(blobReq->smPerfCtx);
    if (fZeroSize) {
        // If there's not object data to write, just update the
        // SM's response ack count
        blobReq->notifyResponse(qos_ctrl, qosReq, err);
    } else {
        // Update the transaction manager with the stage object data
        fds_verify(amTxMgr->updateStagedBlobObject(*(blobReq->getTxId()),
                                                   blobReq->getObjId(),
                                                   blobReq->getDataBuf(),
                                                   blobReq->getDataLen())
                   == ERR_OK);
        issuePutObjectMsg(blobReq->getObjId(),
                          blobReq->getDataBuf(),
                          blobReq->getDataLen(),
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
    fds::PerfTracer::tracePointEnd(blobReq->smPerfCtx); 
    fpi::PutObjectRspMsgPtr putObjRsp =
        net::ep_deserialize<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error; 
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
    fds::PerfTracer::tracePointEnd(blobReq->dmPerfCtx); 
    fpi::UpdateCatalogRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error; 
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
    fds::PerfTracer::tracePointEnd(blobReq->dmPerfCtx); 
    fpi::UpdateCatalogOnceRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogOnceRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error; 
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

    fds_volid_t   volId = blobReq->getVolId();
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
    blobReq->setBlobOffset(blobReq->getBlobOffset() * maxObjSize);

    // Check cache for object ID
    ObjectID::ptr objectId = amCache->getBlobOffsetObject(volId,
                                                          blobReq->getBlobName(),
                                                          blobReq->getBlobOffset(),
                                                          err);
    if (err != ERR_OK) {
        fds::PerfTracer::tracePointBegin(blobReq->dmPerfCtx); 
        issueQueryCatalog(blobReq->getBlobName(),
                          blobReq->getBlobOffset(),
                          volId,
                          RESPONSE_MSG_HANDLER(StorHvCtrl::getBlobQueryCatalogResp, qosReq));
    } else {
        fds_verify(*objectId != NullObjectID);
        blobReq->setObjId(*objectId);
        fds::PerfTracer::tracePointBegin(blobReq->smPerfCtx); 
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
    queryMsg->volume_id = volId;
    queryMsg->blob_name             = blobName;
    // We don't currently specify a version
    queryMsg->blob_version          = blob_version_invalid;
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
    fds_volid_t volId = blobReq->getVolId();
    ObjectID objId    = blobReq->getObjId();

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
        if (blobReq->getDataLen() < static_cast<fds_uint64_t>(objectData->size())) {
            LOGDEBUG  << "Returning " << blobReq->getDataLen() << " byte subset of "
                      << objectData->size() << " bytes of data";
            cb->returnSize = blobReq->getDataLen();
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

    fds::PerfTracer::tracePointEnd(blobReq->dmPerfCtx); 

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << error; 
        qos_ctrl->markIODone(qosReq);
        blobReq->cb->call(error);
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
        if ((fds_uint64_t)(*it).offset == blobReq->getBlobOffset()) {
            // found offset!!!
            ObjectID objId((*it).data_obj_id.digest);
            fds_verify(objId != NullObjectID);
            
            blobReq->setObjId(objId);
            fds::PerfTracer::tracePointBegin(blobReq->smPerfCtx); 
            issueGetObject(qosReq,
                           RESPONSE_MSG_HANDLER(StorHvCtrl::getBlobGetObjectResp,
                                                qosReq));

            return;
        }
    }

    // if we are here, we did not get response for offset we needed!
    LOGDEBUG << "blob name: " << blobReq->getBlobName() << "offset: "
             << blobReq->getBlobOffset() << " not in returned offset list from DM";
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
    fds::PerfTracer::tracePointEnd(blobReq->smPerfCtx); 

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << error; 
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
    if (blobReq->getDataLen() >= (4 * 1024)) {
        // Check that we didn't get too much data
        // Since 4K is our min, it's OK to get more
        // when less than 4K is requested
        // TODO(Andrew): Revisit for unaligned IO
        fds_verify((uint)(getObjRsp->data_obj_len) <= (blobReq->getDataLen()));
    }
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback,
                                                blobReq->cb);
    // Set the return size based on what was requested
    if (blobReq->getDataLen() < static_cast<fds_uint64_t>(getObjRsp->data_obj_len)) {
        LOGDEBUG  << "Returning " << blobReq->getDataLen() << " byte subset of "
                  << getObjRsp->data_obj_len << " bytes of data";
        cb->returnSize = blobReq->getDataLen();
    } else {
        cb->returnSize = getObjRsp->data_obj_len;
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
    fds_volid_t vol_id = blobReq->getVolId();

    // Get blob name
    std::string blob_name = blobReq->getBlobName();

    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(vol_id);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    // Send to the DM
    fds::PerfTracer::tracePointBegin(blobReq->dmPerfCtx); 
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
    fds::PerfTracer::tracePointEnd(blobReq->dmPerfCtx); 
    fpi::DeleteCatalogObjectRspMsgPtr delCatRsp =
        net::ep_deserialize<fpi::DeleteCatalogObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error; 
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
        LOGERROR << "Get metadata volume name: " << volMDReq->getVolumeName()
                << " Error: " << error;
        qos_ctrl->markIODone(qosReq);
        volMDReq->cb->call(error);
        delete volMDReq;
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
    fds_verify(amTxMgr->updateStagedBlobDesc(*(blobReq->getTxId()),
                                             blobReq->getMetaDataListPtr()) == ERR_OK);

    fds_volid_t   vol_id = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(vol_id);
    fds_verify(shVol != NULL);
    fds_verify(shVol->isValidLocked() == true);

    std::string blob_name = blobReq->getBlobName();
    fds_uint64_t dmt_version;
    err = amTxMgr->getTxDmtVersion(*(blobReq->getTxId()), &dmt_version);
    fds_verify(err == ERR_OK);
 
    LOGDEBUG << " Invoking issueSetBlobMetaData";
    issueSetBlobMetaData(vol_id, blob_name, blob_version_invalid, blobReq->getMetaDataListPtr(),
                         blobReq->getTxId()->getValue(), dmt_version,
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

#if 0
    // Manually copy the MD entri[es over to the new list
    // TODO(brian): Find a more elegant way to do this
    for (auto kv : *md_list) {
        setMDMsg->metaDataList.push_back(kv);
    }
    
    fds_assert(setMDMsg->metaDataList.size() > 0);
#endif

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
