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
#include <fdsp/DMSvc.h>
#include <fdsp/SMSvc.h>
#include <fdsp_utils.h>

#include "requests/requests.h"

extern StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;
typedef fds::hash::Sha1 GeneratorHash;

StorHvCtrl::TxnResponseHelper::TxnResponseHelper(StorHvCtrl* storHvisor,
                                                 fds_volid_t  volId, fds_uint32_t txnId)
        : storHvisor(storHvisor), txnId(txnId), volId(volId)
{
    vol = storHvisor->vol_table->getVolume(volId);
    vol_lock = new StorHvVolumeLock(vol);
    txn = vol->journal_tbl->get_journal_entry(txnId);
    je_lock = new StorHvJournalEntryLock(txn);

    fds_verify(txn != NULL);
    fds_verify(txn->isActive() == true);
    blobReq = TO_DERIVED(fds::AmRequest, txn->io);
    fds_verify(blobReq != NULL);
    blobReq->cb->status = ERR_OK;
}

void StorHvCtrl::TxnResponseHelper::setStatus(FDSN_Status  status) {
    blobReq->cb->status = status;
}

StorHvCtrl::TxnResponseHelper::~TxnResponseHelper() {
    storHvisor->qos_ctrl->markIODone(txn->io);
    GLOGDEBUG << "releasing txnid:" << txnId;
    txn->reset();
    vol->journal_tbl->releaseTransId(txnId);
    GLOGDEBUG << "doing callback for txnid:" << txnId;
    blobReq->cb->call();
    delete blobReq;
    delete je_lock;
    delete vol_lock;
}

StorHvCtrl::ResponseHelper::ResponseHelper(StorHvCtrl* storHvisor,
                                           AmRequest *amReq)
        : storHvisor(storHvisor), blobReq(amReq) {
    fds_verify(blobReq != NULL);

    volId = blobReq->io_vol_id;
    vol = storHvisor->vol_table->getVolume(volId);
    vol_lock = new StorHvVolumeLock(vol);

    blobReq->cb->status = ERR_OK;
}

void StorHvCtrl::ResponseHelper::setStatus(FDSN_Status  status) {
    blobReq->cb->status = status;
    blobReq->cb->error  = status;
}

StorHvCtrl::ResponseHelper::~ResponseHelper() {
    storHvisor->qos_ctrl->markIODone(blobReq);
    blobReq->cb->call();
    delete blobReq;
    delete vol_lock;
}


StorHvCtrl::TxnRequestHelper::TxnRequestHelper(StorHvCtrl* storHvisor,
                                               AmRequest *amReq)
        : storHvisor(storHvisor), blobReq(amReq) {
    volId = blobReq->io_vol_id;
    shVol = storHvisor->vol_table->getLockedVolume(volId);
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));
}

bool StorHvCtrl::TxnRequestHelper::isValidVolume() {
    return ((shVol != NULL) && (shVol->isValidLocked()));
}

bool StorHvCtrl::TxnRequestHelper::setupTxn() {
    bool trans_in_progress = false;
    txnId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                      blobReq->blob_offset,
                                                      trans_in_progress);

    txn = shVol->journal_tbl->get_journal_entry(txnId);
    fds_verify(txn != NULL);
    jeLock = new StorHvJournalEntryLock(txn);

    if (trans_in_progress || txn->isActive()) {
        GLOGWARN << "txn: " << txnId << " is already ACTIVE";
        return false;
    }
    txn->setActive();
    return true;
}

bool StorHvCtrl::TxnRequestHelper::getPrimaryDM(fds_uint32_t& ip, fds_uint32_t& port) {
    // Get DMT node list from dmt
    DmtColumnPtr nodeIds = storHvisor->dataPlacementTbl->getDMTNodesForVolume(storHvisor->vol_table->getBaseVolumeId(volId)); //NOLINT
    fds_verify(nodeIds->getLength() > 0);
    fds_int32_t node_state = -1;
    storHvisor->dataPlacementTbl->getNodeInfo(nodeIds->get(0).uuid_get_val(),
                                              &ip,
                                              &port,
                                              &node_state);
    return true;
}

void StorHvCtrl::TxnRequestHelper::scheduleTimer() {
    GLOGDEBUG << "scheduling timer for txnid:" << txnId;
    shVol->journal_tbl->schedule(txn->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
}

void StorHvCtrl::TxnRequestHelper::setStatus(FDSN_Status status) {
    this->status = status;
}

bool StorHvCtrl::TxnRequestHelper::hasError() {
    return ((status != ERR_OK) && (status != FDSN_StatusNOTSET));
}

StorHvCtrl::TxnRequestHelper::~TxnRequestHelper() {
    if (jeLock) delete jeLock;
    if (shVol) shVol->readUnlock();

    if (hasError()) {
        GLOGWARN << "error processing txnid:" << txnId << " : " << status;
        txn->reset();
        shVol->journal_tbl->releaseTransId(txnId);
        if (blobReq->cb.get() != NULL) {
            GLOGDEBUG << "doing callback for txnid:" << txnId;
            blobReq->cb->call(status);
        }
        delete blobReq;
        //delete blobReq;
    } else {
        // scheduleTimer();
    }
}

StorHvCtrl::RequestHelper::RequestHelper(StorHvCtrl* storHvisor,
                                         AmRequest *amReq)
        : storHvisor(storHvisor), blobReq(amReq) {
    volId = blobReq->io_vol_id;
    shVol = storHvisor->vol_table->getLockedVolume(volId);
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));
}

bool StorHvCtrl::RequestHelper::isValidVolume() {
    return ((shVol != NULL) && (shVol->isValidLocked()));
}

void StorHvCtrl::RequestHelper::setStatus(FDSN_Status status) {
    this->status = status;
}

bool StorHvCtrl::RequestHelper::hasError() {
    return ((status != ERR_OK) && (status != FDSN_StatusNOTSET));
}

StorHvCtrl::RequestHelper::~RequestHelper() {
    if (shVol) shVol->readUnlock();

    if (hasError()) {
        if (blobReq->cb.get() != NULL) {
            GLOGDEBUG << "doing callback";
            blobReq->cb->call(status);
        }
        delete blobReq;
        //delete blobReq;
    }
}

StorHvCtrl::BlobRequestHelper::BlobRequestHelper(StorHvCtrl* storHvisor,
                                                 const std::string& volumeName)
        : storHvisor(storHvisor), volumeName(volumeName) {
                setupVolumeInfo();
            }

void StorHvCtrl::BlobRequestHelper::setupVolumeInfo() {
    volId = storHvisor->vol_table->getVolumeUUID(volumeName);
}

fds::Error StorHvCtrl::BlobRequestHelper::processRequest() {
    if (volId != invalid_vol_id) {
        blobReq->io_vol_id = volId;
        GLOGDEBUG << "volid:" << blobReq->io_vol_id;
        storHvisor->pushBlobReq(blobReq);
        return ERR_OK;
    } else {
        // If we don't have the volume, queue up the request
        GLOGDEBUG << "volume id not found:" << volumeName;
        storHvisor->vol_table->addBlobToWaitQueue(volumeName, blobReq);
    }

    // if we are here, bucket is not attached to this AM, send test bucket msg to OM
    return storHvisor->sendTestBucketToOM(volumeName, "", "");
}

/*
 * TODO: Actually calculate the host's IP
 */
#define SRC_IP           0x0a010a65

/**
 * Dispatches async FDSP messages to SMs for a put msg.
 * Note, assumes the journal entry lock is held already
 * and that the messages are ready to be dispatched.
 * If send_uuid is specified, put is targeted only to send_uuid.
 */
fds::Error
StorHvCtrl::dispatchSmPutMsg(StorHvJournalEntry *journEntry,
                             const NodeUuid &send_uuid) {
    fds::Error err(ERR_OK);

    fds_verify(journEntry != NULL);

    FDSP_MsgHdrTypePtr smMsgHdr = journEntry->sm_msg;
    fds_verify(smMsgHdr != NULL);
    FDSP_PutObjTypePtr putMsg = journEntry->putMsg;
    fds_verify(putMsg != NULL);

    ObjectID objId(putMsg->data_obj_id.digest);

    // Get DLT node list from dlt
    DltTokenGroupPtr dltPtr;
    dltPtr = dataPlacementTbl->getDLTNodesForDoidKey(objId);
    fds_verify(dltPtr != NULL);

    fds_uint32_t numNodes = dltPtr->getLength();
    fds_verify(numNodes > 0);
    
    putMsg->dlt_version = om_client->getDltVersion();
    fds_verify(putMsg->dlt_version != DLT_VER_INVALID);

    // checksum calculation for putObj  class and the payload.  we may haveto bundle this
    // into a function .
    storHvisor->chksumPtr->checksum_update(putMsg->data_obj_id.digest);
    storHvisor->chksumPtr->checksum_update(putMsg->data_obj_len);
    storHvisor->chksumPtr->checksum_update(putMsg->volume_offset);
    storHvisor->chksumPtr->checksum_update(putMsg->dlt_version);
    storHvisor->chksumPtr->checksum_update(putMsg->data_obj);
    storHvisor->chksumPtr->get_checksum(smMsgHdr->payload_chksum);
    LOGDEBUG << "RPC Checksum: " << smMsgHdr->payload_chksum;

    uint errcount = 0;
    // Issue a put for each SM in the DLT list
    for (fds_uint32_t i = 0; i < numNodes; i++) {
        fds_uint32_t node_ip   = 0;
        fds_uint32_t node_port = 0;
        fds_int32_t node_state = -1;
        NodeUuid target_uuid = dltPtr->get(i);

        // Get specific SM's info
        dataPlacementTbl->getNodeInfo(target_uuid.uuid_get_val(),
                                      &node_ip,
                                      &node_port,
                                      &node_state);
        if (send_uuid != INVALID_RESOURCE_UUID && send_uuid != target_uuid) {
            continue;
        }

        journEntry->sm_ack[i].ipAddr = node_ip;
        journEntry->sm_ack[i].port   = node_port;
        smMsgHdr->dst_ip_lo_addr     = node_ip;
        smMsgHdr->dst_port           = node_port;
        journEntry->sm_ack[i].ack_status = FDS_CLS_ACK;
        journEntry->num_sm_nodes     = numNodes;

        try {
            // Call Put object RPC to SM
            netDataPathClientSession *sessionCtx =
                    rpcSessionTbl->\
                    getClientSession<netDataPathClientSession>(node_ip, node_port);
            fds_verify(sessionCtx != NULL);

            // Set session UUID in journal and msg
            boost::shared_ptr<FDSP_DataPathReqClient> client = sessionCtx->getClient();
            smMsgHdr->session_uuid = sessionCtx->getSessionId();
            journEntry->session_uuid = smMsgHdr->session_uuid;

            client->PutObject(smMsgHdr, putMsg);
            LOGDEBUG << "For transaction " << journEntry->trans_id
                      << " sent async PUT_OBJ_REQ to SM ip "
                      << node_ip << " port " << node_port;
        } catch (att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what() ;
            errcount++;
        }

        if (send_uuid != INVALID_RESOURCE_UUID) {
            LOGDEBUG << " Peformed a targeted send to: " << send_uuid;
            break;
        }
    }

    if (errcount >= int(ceil(numNodes*0.5))) {
        LOGCRITICAL << "Too many network errors : " << errcount ;
        return ERR_NETWORK_TRANSPORT;
    }

    return err;
}

/**
 * Dispatches async FDSP messages to SMs for a get msg.
 * Note, assumes the journal entry lock is held already
 * and that the messages are ready to be dispatched.
 */
fds::Error
StorHvCtrl::dispatchSmGetMsg(StorHvJournalEntry *journEntry) {
    fds::Error err(ERR_OK);

    fds_verify(journEntry != NULL);

    FDSP_MsgHdrTypePtr smMsgHdr = journEntry->sm_msg;
    fds_verify(smMsgHdr != NULL);
    FDSP_GetObjTypePtr getMsg = journEntry->getMsg;
    fds_verify(getMsg != NULL);

    ObjectID objId(getMsg->data_obj_id.digest);

    fds_volid_t   volId =smMsgHdr->glob_volume_id;
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);

    // Look up primary SM from DLT entries
    boost::shared_ptr<DltTokenGroup> dltPtr;
    dltPtr = dataPlacementTbl->getDLTNodesForDoidKey(objId);
    fds_verify(dltPtr != NULL);

    fds_int32_t numNodes = dltPtr->getLength();
    fds_verify(numNodes > 0);

    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;

    // Get primary SM's node info
    // TODO: We're just assuming it's the first in the list!
    // We should be verifying this somehow.
    dataPlacementTbl->getNodeInfo(dltPtr->get(journEntry->nodeSeq).uuid_get_val(),
                                  &node_ip,
                                  &node_port,
                                  &node_state);
    smMsgHdr->dst_ip_lo_addr = node_ip;
    smMsgHdr->dst_port       = node_port;

    journEntry->sm_ack[journEntry->nodeSeq].ipAddr = node_ip;
    journEntry->sm_ack[journEntry->nodeSeq].port   = node_port;
    journEntry->sm_ack[journEntry->nodeSeq].ack_status = FDS_CLS_ACK;
    journEntry->num_sm_nodes     = numNodes;

    LOGDEBUG << "For transaction: " << journEntry->trans_id << "node_ip:" << node_ip << "node_port" << node_port;
    getMsg->dlt_version = om_client->getDltVersion();
    fds_verify(getMsg->dlt_version != DLT_VER_INVALID);

    try {

        netDataPathClientSession *sessionCtx =
                rpcSessionTbl->\
                getClientSession<netDataPathClientSession>(node_ip, node_port);
        fds_verify(sessionCtx != NULL);

        boost::shared_ptr<FDSP_DataPathReqClient> client = sessionCtx->getClient();
        smMsgHdr->session_uuid = sessionCtx->getSessionId();
        journEntry->session_uuid = smMsgHdr->session_uuid;

        // Schedule a timer here to track the responses and the original request
        shVol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));

        // RPC getObject to StorMgr
        client->GetObject(smMsgHdr, getMsg);

        LOGDEBUG << "For trans " << journEntry->trans_id
                  << " sent async GetObj req to SM";
    } catch (att::TTransportException& e) {
        LOGERROR << "error during network call : " << e.what() ;
    }

    return err;
}

void
StorHvCtrl::statBlobResp(const FDSP_MsgHdrTypePtr rxMsg, 
                         const FDS_ProtocolInterface::
                         BlobDescriptorPtr blobDesc) {
    Error err(ERR_OK);

    // TODO(Andrew): We don't really need this...
    fds_verify(rxMsg->msg_code == FDSP_STAT_BLOB);

    fds_uint32_t transId = rxMsg->req_cookie;
    fds_volid_t  volId   = rxMsg->glob_volume_id;

    StorHvVolume* vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);  // Should not receive resp for non existant vol

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);

    StorHvJournalEntryLock je_lock(txn);
    fds_verify(txn->isActive() == true);

    // Get request from transaction
    fds::AmRequest *blobReq = static_cast<fds::StatBlobReq *>(txn->io);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_STAT_BLOB);

    qos_ctrl->markIODone(txn->io);

    // Return if err
    if (rxMsg->result != FDSP_ERR_OK) {
        vol->journal_tbl->releaseTransId(transId);
        blobReq->cb->call(FDSN_StatusErrorUnknown);
        txn->reset();
        delete blobReq;
        return;
    }

    StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback,blobReq->cb);

    cb->blobDesc.setBlobName(blobDesc->name);
    cb->blobDesc.setBlobSize(blobDesc->byteCount);
    for (auto const &it : blobDesc->metadata) {
        cb->blobDesc.addKvMeta(it.first, it.second);
    }
    txn->reset();
    vol->journal_tbl->releaseTransId(transId);
    cb->call(ERR_OK);
    delete blobReq;
}

/**
 * Function called when volume waiting queue is drained.
 * When it's called a volume has just been attached and
 * we can call the callback to tell any waiters that the
 * volume is now ready.
 */
void
StorHvCtrl::attachVolume(AmRequest *amReq) {
    // Get request from qos object
    fds_verify(amReq != NULL);
    AttachVolBlobReq *blobReq = static_cast<AttachVolBlobReq *>(amReq);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_ATTACH_VOL);

    LOGDEBUG << "Attach for volume " << blobReq->volume_name
             << " complete. Notifying waiters";
    blobReq->cb->call(ERR_OK);
}

/*****************************************************************************

 *****************************************************************************/
