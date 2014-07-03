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
#include <net/RpcRequestPool.h>
#include <fdsp/DMSvc.h>
#include <fdsp/SMSvc.h>
#include <fdsp_utils.h>

extern StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;
typedef fds::hash::Sha1 GeneratorHash;


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
    BlobTxId txId(storHvisor->randNumGen->genNum());
    // Stash the newly created ID in the callback for later
    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  blobReq->cb);
    cb->blobTxId = txId;

    issueStartBlobTxMsg(blobReq->getBlobName(),
                        volId,
                        txId.getValue(),
                        std::bind(&StorHvCtrl::putBlobUpdateCatalogMsgResp,
                                  this, qosReq,
                                  std::placeholders::_1,
                                  std::placeholders::_2,std::placeholders::_3));
    return err;
}


void StorHvCtrl::issueStartBlobTxMsg(const std::string& blobName,
                                     const fds_volid_t& volId,
                                     const fds_uint64_t& txId,
                                      QuorumRpcRespCb respCb)
{

    StartBlobTxMsgPtr stBlobTxMsg(new StartBlobTxMsg());
    stBlobTxMsg->blob_name   = blobName;
    stBlobTxMsg->blob_version = blob_version_invalid;
    stBlobTxMsg->volume_id = volId;


    stBlobTxMsg->txId = txId;

#ifdef RPC_BASED_ASYNC_COMM
    auto asyncStartBlobTxReq = gRpcRequestPool->newQuorumRpcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncStartBlobTxReq->setRpcFunc(
        CREATE_RPC(fpi::DMSvcClient, startBlobTx, stBlobTxMsg));
#else
    auto asyncStartBlobTxReq = gRpcRequestPool->newQuorumNetRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncStartBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg), stBlobTxMsg);
#endif
    asyncStartBlobTxReq->setTimeoutMs(500);
    asyncStartBlobTxReq->onResponseCb(respCb);
    asyncStartBlobTxReq->invoke();

    LOGDEBUG << asyncStartBlobTxReq->logString() << fds::logString(*stBlobTxMsg);

}


void StorHvCtrl::startBlobTxMsgResp(fds::AmQosReq* qosReq,
                                    QuorumRpcRequest* rpcReq,
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
    cb->call(ERR_OK);
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
        objId = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                      blobReq->getDataLen());
    }
    blobReq->setObjId(objId);

    // Send put request to SM
    issuePutObjectMsg(blobReq->getObjId(),
                      blobReq->getDataBuf(),
                      blobReq->getDataLen(),
                      volId,
                      std::bind(&StorHvCtrl::putBlobPutObjectMsgResp,
                                this, qosReq,
                                std::placeholders::_1,
                                std::placeholders::_2,std::placeholders::_3));

    issueUpdateCatalogMsg(blobReq->getObjId(),
                          blobReq->getBlobName(),
                          blobReq->getBlobOffset(),
                          blobReq->getDataLen(),
                          blobReq->isLastBuf(),
                          volId,
                          std::bind(&StorHvCtrl::putBlobUpdateCatalogMsgResp,
                                    this, qosReq,
                                    std::placeholders::_1,
                                    std::placeholders::_2,std::placeholders::_3));
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
                                   QuorumRpcRespCb respCb)
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

#ifdef RPC_BASED_ASYNC_COMM
    auto asyncPutReq = gRpcRequestPool->newQuorumRpcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDLTNodesForDoidKey(objId)));
    asyncPutReq->setRpcFunc(
        CREATE_RPC(fpi::SMSvcClient, putObject, putObjMsg));
#else
    auto asyncPutReq = gRpcRequestPool->newQuorumNetRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDLTNodesForDoidKey(objId)));
    asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg), putObjMsg);
#endif
    asyncPutReq->setTimeoutMs(500);
    asyncPutReq->onResponseCb(respCb);
    asyncPutReq->invoke();

    LOGDEBUG << asyncPutReq->logString() << fds::logString(*putObjMsg);
}

void StorHvCtrl::issueUpdateCatalogMsg(const ObjectID &objId,
                                       const std::string& blobName,
                                       const fds_uint64_t& blobOffset,
                                       const fds_uint64_t &len,
                                       const bool &lastBuf,
                                       const fds_volid_t& volId,
                                       QuorumRpcRespCb respCb)
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

#ifdef RPC_BASED_ASYNC_COMM
    auto asyncUpdateCatReq = gRpcRequestPool->newQuorumRpcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncUpdateCatReq->setRpcFunc(
        CREATE_RPC(fpi::DMSvcClient, updateCatalog, updCatMsg));
#else
    auto asyncUpdateCatReq = gRpcRequestPool->newQuorumNetRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), updCatMsg);
#endif
    asyncUpdateCatReq->setTimeoutMs(500);
    asyncUpdateCatReq->onResponseCb(respCb);
    asyncUpdateCatReq->invoke();

    LOGDEBUG << asyncUpdateCatReq->logString() << fds::logString(*updCatMsg);
}

void StorHvCtrl::putBlobPutObjectMsgResp(fds::AmQosReq* qosReq,
                                         QuorumRpcRequest* rpcReq,
                                         const Error& error,
                                         boost::shared_ptr<std::string> payload)
{
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
    fpi::PutObjectRspMsgPtr putObjRsp =
        net::ep_deserialize<fpi::PutObjectRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error; 
    } else {
        LOGDEBUG << rpcReq->logString() << fds::logString(*putObjRsp);
    }
    blobReq->notifyResponse(qos_ctrl, qosReq, error);
}

void StorHvCtrl::putBlobUpdateCatalogMsgResp(fds::AmQosReq* qosReq,
                                             QuorumRpcRequest* rpcReq,
                                             const Error& error,
                                             boost::shared_ptr<std::string> payload)
{
    PutBlobReq *blobReq = static_cast<fds::PutBlobReq*>(qosReq->getBlobReqPtr());
    fpi::UpdateCatalogRspMsgPtr updCatRsp =
        net::ep_deserialize<fpi::UpdateCatalogRspMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "Obj ID: " << blobReq->getObjId()
            << " blob name: " << blobReq->getBlobName()
            << " offset: " << blobReq->getBlobOffset() << " Error: " << error; 
    } else {
        LOGDEBUG << rpcReq->logString() << fds::logString(*updCatRsp);
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
        blobReq->cbWithResult(ERR_INVALID);
        delete blobReq;
        err = ERR_DISK_WRITE_FAILED;
        return err;
    }

    // TODO(Rao): Do we need to make sure other get reqs aren't in progress

    ObjectID objId;
    bool inCache = shVol->vol_catalog_cache->LookupObjectId(blobReq->getBlobName(),
                                          blobReq->getBlobOffset(),
                                          objId);
    if (true || !inCache) {
        issueQueryCatalog(blobReq->getBlobName(),
                          blobReq->getBlobOffset(),
                          volId,
                          std::bind(&StorHvCtrl::getBlobQueryCatalogResp, this, qosReq,
                                    std::placeholders::_1, std::placeholders::_2,
                                    std::placeholders::_3));
    } else {
        fds_verify(objId != NullObjectID);
        issueGetObject(volId, objId,
                       std::bind(&StorHvCtrl::getBlobGetObjectResp, this, qosReq,
                                 std::placeholders::_1, std::placeholders::_2,
                                 std::placeholders::_3));
    }
    return err;
}

void StorHvCtrl::issueQueryCatalog(const std::string& blobName,
                       const fds_uint64_t& blobOffset,
                       const fds_volid_t& volId,
                       FailoverRpcRespCb respCb)
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
    // TODO(rao): Figure out where this is used
    queryMsg->dm_transaction_id     = 1;
    queryMsg->dm_operation          =
        FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
    queryMsg->obj_list.clear();
    queryMsg->meta_list.clear();

#ifdef RPC_BASED_ASYNC_COMM
    auto asyncQueryReq = gRpcRequestPool->newFailoverRpcRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncQueryReq->setRpcFunc(
        CREATE_RPC(fpi::DMSvcClient, queryCatalogObject, queryMsg));
#else 
    auto asyncQueryReq = gRpcRequestPool->newFailoverNetRequest(
        boost::make_shared<DmtVolumeIdEpProvider>(om_client->getDMTNodesForVolume(volId)));
    asyncQueryReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), queryMsg);
#endif
    asyncQueryReq->setTimeoutMs(500);
    asyncQueryReq->onResponseCb(respCb);
    asyncQueryReq->invoke();

    LOGDEBUG << asyncQueryReq->logString() << fds::logString(*queryMsg);
}

void StorHvCtrl::issueGetObject(const fds_volid_t& volId,
                                const ObjectID& objId,
                                FailoverRpcRespCb respCb)
{
    fpi::GetObjectMsgPtr getObjMsg(boost::make_shared<fpi::GetObjectMsg>());
    getObjMsg->volume_id = volId;
    getObjMsg->data_obj_id.digest = std::string((const char *)objId.GetId(),
                                                (size_t)objId.GetLen());

#ifdef RPC_BASED_ASYNC_COMM
    auto asyncGetReq = gRpcRequestPool->newFailoverRpcRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDLTNodesForDoidKey(objId)));
    asyncGetReq->setRpcFunc(
        CREATE_RPC(fpi::SMSvcClient, getObject, getObjMsg));
#else 
    auto asyncGetReq = gRpcRequestPool->newFailoverNetRequest(
        boost::make_shared<DltObjectIdEpProvider>(om_client->getDLTNodesForDoidKey(objId)));
    asyncGetReq->setPayload(FDSP_MSG_TYPEID(fpi::GetObjectMsg), getObjMsg);
#endif
    asyncGetReq->setTimeoutMs(500);
    asyncGetReq->onResponseCb(respCb);
    asyncGetReq->invoke();

    LOGDEBUG << asyncGetReq->logString() << fds::logString(*getObjMsg);
}

void StorHvCtrl::getBlobQueryCatalogResp(fds::AmQosReq* qosReq,
                                         FailoverRpcRequest* rpcReq,
                                         const Error& error,
                                         boost::shared_ptr<std::string> payload)
{
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(qosReq->getBlobReqPtr());
    fpi::QueryCatalogMsgPtr qryCatRsp =
        net::ep_deserialize<fpi::QueryCatalogMsg>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << error; 
        qos_ctrl->markIODone(qosReq);
        blobReq->cbWithResult(ERR_INVALID);
        delete blobReq;
        return;
    }

    LOGDEBUG << rpcReq->logString() << fds::logString(*qryCatRsp);

    Error e = updateCatalogCache(blobReq,
                                 qryCatRsp->obj_list);
    if (e != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << e; 
        qos_ctrl->markIODone(qosReq);
        blobReq->cbWithResult(e.GetErrno());
        delete blobReq;
        return;
    }

    /* NOTE: For now making the assumption that there is only one object id */
    fds_verify(qryCatRsp->obj_list.size() == 1);
    ObjectID objId(qryCatRsp->obj_list[0].data_obj_id.digest);
    fds_verify(objId != NullObjectID);

    issueGetObject(blobReq->getVolId(), objId,
                   std::bind(&StorHvCtrl::getBlobGetObjectResp, this, qosReq,
                             std::placeholders::_1, std::placeholders::_2,
                             std::placeholders::_3));
}

void StorHvCtrl::getBlobGetObjectResp(fds::AmQosReq* qosReq,
                                      FailoverRpcRequest* rpcReq,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload)
{
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(qosReq->getBlobReqPtr());
    fpi::GetObjectRespPtr getObjRsp =
        net::ep_deserialize<fpi::GetObjectResp>(const_cast<Error&>(error), payload);

    if (error != ERR_OK) {
        LOGERROR << "blob name: " << blobReq->getBlobName() << "offset: "
            << blobReq->getBlobOffset() << " Error: " << error; 
        qos_ctrl->markIODone(qosReq);
        blobReq->cbWithResult(ERR_INVALID);
        delete blobReq;
        return;
    }

    LOGDEBUG << rpcReq->logString() << fds::logString(*getObjRsp);

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
    blobReq->setDataLen(getObjRsp->data_obj_len);    
    blobReq->setDataBuf(getObjRsp->data_obj.c_str());
    blobReq->cbWithResult(ERR_OK);
    delete blobReq;
}
