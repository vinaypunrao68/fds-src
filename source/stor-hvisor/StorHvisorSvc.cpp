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



}
