/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include "./handler.h"
#include <NetSession.h>
#include "../StorHvisorNet.h"

namespace fds {

Error GetVolumeMetaDataHandler::handleRequest(const std::string& volumeName, CallbackPtr cb) {
    StorHvCtrl::BlobRequestHelper helper(storHvisor, volumeName);
    LOGDEBUG << "volume: " << volumeName;

    helper.blobReq = new GetVolumeMetaDataReq(helper.volId,
                                              volumeName,
                                              cb);
    return helper.processRequest();
}

Error GetVolumeMetaDataHandler::handleResponse(fpi::FDSP_MsgHdrTypePtr& header,
                                               fpi::FDSP_VolumeMetaDataPtr& volumeMeta) {
    Error err(ERR_OK);
    LOGDEBUG << "vol:" << header->glob_volume_id
             << " txnid:" << header->req_cookie
             << " blobCount:" << volumeMeta->blobCount
             << " size:" << volumeMeta->size;

    StorHvCtrl::TxnResponseHelper txnHelper(storHvisor, header->glob_volume_id, header->req_cookie);

    // Return if err
    if (header->result != FDSP_ERR_OK) {
        LOGWARN << "error in response: " << header->result;
        txnHelper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_MAX;
    }

    GetVolumeMetaDataCallback::ptr cb =
                SHARED_DYN_CAST(GetVolumeMetaDataCallback, txnHelper.blobReq->cb);

    cb->volumeMetaData = *volumeMeta;
    return err;
}

Error GetVolumeMetaDataHandler::handleQueueItem(AmQosReq *qosReq) {
    Error err(ERR_OK);

    StorHvCtrl::TxnRequestHelper helper(storHvisor, qosReq);

    if (!helper.isValidVolume()) {
        LOGCRITICAL << "unable to get volume info for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusErrorUnknown);
        return ERR_DISK_READ_FAILED;
    }

    if (!helper.setupTxn()) {
        LOGWARN << "unable to get txn for vol: " << helper.volId;
        helper.setStatus(FDSN_StatusTxnInProgress);
        return ERR_DISK_READ_FAILED;
    }

    LOGDEBUG << "setup txnid:" << helper.txnId
             << " for vol:" << helper.volId;

    helper.txn->op          = FDS_GET_VOLUME_METADATA;
    helper.txn->io          = qosReq;

    fpi::FDSP_MsgHdrTypePtr msgHdr(new FDSP_MsgHdrType);
    storHvisor->InitDmMsgHdr(msgHdr);
    msgHdr->glob_volume_id = helper.volId;
    msgHdr->req_cookie = helper.txnId;
    GetVolumeMetaDataReq* blobReq = TO_DERIVED(GetVolumeMetaDataReq, helper.blobReq);

    fds_uint32_t ip, port;
    helper.getPrimaryDM(ip, port);
    METADATA_SESSION(session, storHvisor->rpcSessionTbl, ip, port);
    auto client = session->getClient();
    msgHdr->session_uuid = session->getSessionId();

    boost::shared_ptr<std::string> volNamePtr(new std::string(blobReq->volumeName));

    // Send async RPC request
    try {
        client->GetVolumeMetaData(msgHdr, volNamePtr);
    } catch(att::TTransportException &e) {
        LOGERROR << "error during network call: " << e.what();
        err = ERR_NETWORK_TRANSPORT;
        helper.setStatus(FDSN_StatusErrorUnknown);
    }
    return err;
}

}  // namespace fds
