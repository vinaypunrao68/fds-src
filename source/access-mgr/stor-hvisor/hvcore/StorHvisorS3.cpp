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
 * Dispatches async FDSP messages to DMs for a blob update.
 * Note, assumes the journal entry lock is held already
 * and that the messages are ready to be dispatched.
 * If send_uuid is specified, put is targeted only to send_uuid.
 */
fds::Error
StorHvCtrl::dispatchDmUpdMsg(StorHvJournalEntry *journEntry,
                             const NodeUuid &send_uuid) {
    fds::Error err(ERR_OK);

    fds_verify(journEntry != NULL);

    FDSP_MsgHdrTypePtr dmMsgHdr = journEntry->dm_msg;
    fds_verify(dmMsgHdr != NULL);
    FDSP_UpdateCatalogTypePtr updCatMsg = journEntry->updCatMsg;
    fds_verify(updCatMsg != NULL);

    fds_volid_t volId = dmMsgHdr->glob_volume_id;

    // Get DMT node list from dmt
    DmtColumnPtr nodeIds = dataPlacementTbl->getDMTNodesForVolume(vol_table->getBaseVolumeId(volId));
    fds_verify(nodeIds->getLength() > 0);

    // Issue a blob update for each DM in the DMT
    fds_uint32_t errcount = 0;
    for (fds_uint32_t i = 0; i < nodeIds->getLength(); i++) {
        fds_uint32_t node_ip   = 0;
        fds_uint32_t node_port = 0;
        fds_int32_t node_state = -1;
        dataPlacementTbl->getNodeInfo(nodeIds->get(i).uuid_get_val(),
                                      &node_ip,
                                      &node_port,
                                      &node_state);
        journEntry->dm_ack[i].ipAddr        = node_ip;
        journEntry->dm_ack[i].port          = node_port;
        // TODO(Andrew): This seems like a race...
        // It's the same ptr that we modify in a loop
        dmMsgHdr->dst_ip_lo_addr            = node_ip;
        dmMsgHdr->dst_port                  = node_port;
        journEntry->dm_ack[i].ack_status    = FDS_CLS_ACK;
        journEntry->dm_ack[i].commit_status = FDS_CLS_ACK;
        journEntry->num_dm_nodes            = nodeIds->getLength();
    
        // Call Update Catalog RPC call to DM
        try {
            netMetaDataPathClientSession *sessionCtx =
                    storHvisor->rpcSessionTbl->\
                    getClientSession<netMetaDataPathClientSession>(node_ip, node_port);
            fds_verify(sessionCtx != NULL);

            boost::shared_ptr<FDSP_MetaDataPathReqClient> client = sessionCtx->getClient();
            dmMsgHdr->session_uuid = sessionCtx->getSessionId();
            journEntry->session_uuid = dmMsgHdr->session_uuid;
            client->UpdateCatalogObject(dmMsgHdr, updCatMsg);

            LOGDEBUG << "For transaction " << journEntry->trans_id
                     << " sent async UP_CAT_REQ to DM ip "
                     << node_ip << " port " << node_port;
        } catch (att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what() ;
            errcount++;
        }
    }

    return err;
}

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

/**
 * Handles initial processing for putting objects to SMs
 */
fds::Error
StorHvCtrl::processSmPutObj(PutBlobReq *putBlobReq,
                            StorHvJournalEntry *journEntry) {
    fds::Error err(ERR_OK);

    fds_volid_t  volId   = putBlobReq->io_vol_id;
    ObjectID     objId   = putBlobReq->obj_id;
    fds_uint32_t transId = journEntry->trans_id;

    // Setup message header
    FDSP_MsgHdrTypePtr msgHdrSm(new FDSP_MsgHdrType);
    InitSmMsgHdr(msgHdrSm);
    msgHdrSm->glob_volume_id   = volId;
    msgHdrSm->origin_timestamp = fds::util::getTimeStampMillis();
    msgHdrSm->req_cookie       = transId;

    // Setup operation specific message
    FDSP_PutObjTypePtr putObjReq(new FDSP_PutObjType);
    putObjReq->data_obj = std::string((const char *)putBlobReq->getDataBuf(),
                                      (size_t )putBlobReq->data_len);
    putObjReq->data_obj_len = putBlobReq->data_len;
    putObjReq->data_obj_id.digest =
            std::string((const char *)objId.GetId(), (size_t)objId.GetLen());

    // Update the SM-related journal fields
    journEntry->sm_msg = msgHdrSm;
    journEntry->putMsg = putObjReq;

    LOGDEBUG << "Putting object " << objId << " for blob "
              << putBlobReq->getBlobName() << " at offset "
              << putBlobReq->blob_offset << " with length "
              << putBlobReq->data_len << " in trans "
              << transId;
 // Dispatch SM put object requests
    bool fZeroSize = (putBlobReq->data_len == 0);
    if (!fZeroSize) {
        err = dispatchSmPutMsg(journEntry, INVALID_RESOURCE_UUID);
        fds_verify(err == ERR_OK);
    } else {
        // special case for zero size
        // trick the state machine to think that it 
        // has gotten responses from SMs since there's
        // no actual data to write
        DltTokenGroupPtr dltPtr;
        dltPtr = dataPlacementTbl->getDLTNodesForDoidKey(objId);
        fds_verify(dltPtr != NULL);

        fds_uint32_t numNodes = dltPtr->getLength();
        fds_verify(numNodes > 0);
        journEntry->sm_ack_cnt = numNodes;
        LOGWARN << "not sending put request to sm as obj is zero size - ["
                << " id:" << objId
                << " objkey:" << putBlobReq->getBlobName()
                << " name:" << putBlobReq->getBlobName()
                << "]";
    }

    return err;
}


/**
 * Handles initial processing for blob catalog updates to DMs
 */
fds::Error
StorHvCtrl::processDmUpdateBlob(PutBlobReq *putBlobReq,
                                StorHvJournalEntry *journEntry) {
    fds::Error err(ERR_OK);

    fds_volid_t  volId   = putBlobReq->io_vol_id;
    ObjectID     objId   = putBlobReq->obj_id;
    fds_uint32_t transId = journEntry->trans_id;

    // Setup message header
    FDSP_MsgHdrTypePtr msgHdrDm(new FDSP_MsgHdrType);
    InitDmMsgHdr(msgHdrDm);
    msgHdrDm->glob_volume_id   = volId;
    msgHdrDm->origin_timestamp = fds::util::getTimeStampMillis();
    msgHdrDm->req_cookie       = transId;

    // Setup operation specific message
    FDSP_UpdateCatalogTypePtr updCatReq(new FDSP_UpdateCatalogType);
    updCatReq->txDesc.txId = putBlobReq->tx_desc->getValue();
    updCatReq->blob_name   = putBlobReq->getBlobName();
    // TODO(Andrew): These can be removed when real transactions work
    updCatReq->dm_transaction_id = 1;
    updCatReq->dm_operation      = FDS_DMGR_TXN_STATUS_OPEN;
    updCatReq->dmt_version       = storHvisor->om_client->getDMTVersion();

    // Setup blob offset updates
    // TODO(Andrew): Today we only expect one offset update
    // TODO(Andrew): Remove lastBuf when we have real transactions
    updCatReq->obj_list.clear();
    FDS_ProtocolInterface::FDSP_BlobObjectInfo updBlobInfo;
    updBlobInfo.offset   = putBlobReq->blob_offset;
    updBlobInfo.size     = putBlobReq->data_len;
    updBlobInfo.blob_end = putBlobReq->last_buf;
    updBlobInfo.data_obj_id.digest =
            std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    // Add the offset info to the DM message
    updCatReq->obj_list.push_back(updBlobInfo);

    // Setup blob metadata updates
    updCatReq->meta_list.clear();

    // Update the DM-related journal fields
    journEntry->trans_state   = FDS_TRANS_OPEN;
    journEntry->dm_msg        = msgHdrDm;
    journEntry->updCatMsg     = updCatReq;
    journEntry->dm_ack_cnt    = 0;
    journEntry->dm_commit_cnt = 0;

    // Dispatch DM update blob requests
    err = dispatchDmUpdMsg(journEntry, INVALID_RESOURCE_UUID);
    fds_verify(err == ERR_OK);

    return err;
}

/**
 * Handles processing putBlob requests from a previously
 * constructed journal entry. The journal entry is expected
 * to already be locked be the caller.
 * TODO(Andrew): This code currently only handles putting a
 * single object into a blob and assumes it's offset aligned.
 * We should remove these assumptions.
 */
fds::Error
StorHvCtrl::resumePutBlob(StorHvJournalEntry *journEntry) {
    fds::Error err(ERR_OK);

    LOGDEBUG << "Processing putBlob journal entry "
             << journEntry->trans_id << " to put request";

    PutBlobReq *blobReq = static_cast<PutBlobReq*>(journEntry->io);

    // Get the object ID and set it in the request
    bool fZeroSize = (blobReq->data_len == 0);
    ObjectID objId;
    if (fZeroSize) {
        LOGWARN << "zero size object - "
                << " [objkey:" << blobReq->getBlobName() <<"]";
    } else {
        objId = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                      blobReq->data_len);
    }
    blobReq->obj_id = objId;

    // Initialize the journal's transaction state
    journEntry->trans_state = FDS_TRANS_OPEN;

    // Process SM object put messages
    err = processSmPutObj(blobReq, journEntry);
    fds_verify(err == ERR_OK);

    // Process DM blob update messages
    err = processDmUpdateBlob(blobReq, journEntry);
    fds_verify(err == ERR_OK);

    return err;
}

void
StorHvCtrl::procNewDlt(fds_uint64_t newDltVer) {
    // Check that this version has already been
    // written to the omclient
    fds_verify(newDltVer == om_client->getDltVersion());

    StorHvVolVec volIds = vol_table->getVolumeIds();
    for (StorHvVolVec::const_iterator it = volIds.cbegin();
         it != volIds.cend();
         it++) {
        fds_volid_t volId = (*it);
        StorHvVolume *vol = vol_table->getVolume(volId);

        // TODO(Andrew): It's possible that a volume gets deleted
        // while we're iterating this list. We should find a way
        // to iterate the table while it's locked.
        fds_verify(vol != NULL);

        // Get all of the requests for this volume that are waiting on the DLT
        PendingDltQueue pendingTrans = vol->journal_tbl->popAllDltTrans();

        // Iterate each transaction and re-issue
        while (pendingTrans.empty() == false) {
            fds_uint32_t transId = pendingTrans.front();
            pendingTrans.pop();

            // Grab the journal entry that was waiting for a DLT
            StorHvJournalEntry *journEntry =
                    vol->journal_tbl->get_journal_entry(transId);
            fds_verify(journEntry != NULL);
            fds_verify(journEntry->trans_state == FDS_TRANS_PENDING_DLT);

            // Acquire RAII lock
            StorHvJournalEntryLock je_lock(journEntry);

            // Re-dispatch the SM requests.
            if (journEntry->op == FDS_PUT_BLOB) {
                dispatchSmPutMsg(journEntry, INVALID_RESOURCE_UUID);
            } else if (journEntry->op == FDS_GET_BLOB) {
                dispatchSmGetMsg(journEntry);
            } else {
                fds_panic("Trying to dispatch unknown pending command");
            }
        }
    }
}

/**
 * Handles requests that are pending a new DLT version.
 * This error case means an SM contacted has committed a
 * new DLT version that is different than the one used
 * in our original request. Token ownership may have
 * changed in the new version and we need to resend the
 * request to the new token owner.
 * Note, assumes the journal entry lock is already held
 * and the volume lock is already held.
 */
void
StorHvCtrl::handleDltMismatch(StorHvVolume *vol,
                              StorHvJournalEntry *journEntry) {
    fds_verify(vol != NULL);
    fds_verify(journEntry != NULL);

    // Mark the entry as pending a newer DLT
    journEntry->trans_state = FDS_TRANS_PENDING_DLT;

    // Queue up the transaction to get processed when we
    // receive a newer DLT
    vol->journal_tbl->pushPendingDltTrans(journEntry->trans_id);
}

fds::Error StorHvCtrl::putObjResp(const FDSP_MsgHdrTypePtr& rxMsg,
                                  const FDSP_PutObjTypePtr& putObjRsp) {
    fds::Error  err(ERR_OK);
    fds_int32_t result = 0;
    fds_verify(rxMsg->msg_code == FDSP_MSG_PUT_OBJ_RSP);

    fds_uint32_t transId = rxMsg->req_cookie; 
    fds_volid_t  volId   = rxMsg->glob_volume_id;
    StorHvVolume *vol    =  vol_table->getVolume(volId);
    fds_verify(vol != NULL);  // We should not receive resp for unknown vol

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);  // We should not receive resp for unknown vol

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);

    StorHvJournalEntryLock je_lock(txn); 

    // Verify transaction is valid and matches cookie from resp message
    fds_verify(txn->isActive() == true);

    // Check response code
    Error msgRespErr(rxMsg->err_code);
    if (msgRespErr == ERR_IO_DLT_MISMATCH) {
        ObjectID objId(putObjRsp->data_obj_id.digest);
        // Note: We're expecting the server to specify the version
        // expected in the response field.
        LOGERROR << "For transaction " << transId
                 << " received DLT version mismatch, have "
                 << om_client->getDltVersion()
                 << ", but SM service expected "
                 << putObjRsp->dlt_version;
        // response message  has the  latest  DLT , update the local copy 
        // and update the DLT version 

        storHvisor->om_client->updateDlt(true, putObjRsp->dlt_data);
        // find the replica index
        int idx = storHvisor->om_client->\
                getCurrentDLT()->getIndex(objId, NodeUuid(rxMsg->src_service_uuid.uuid));
        fds_verify(idx != -1);

        // resend to the same replica index in the new dlt
        storHvisor->dispatchSmPutMsg(txn,
                storHvisor->om_client->getCurrentDLT()->getNode(objId, idx));
        // Return here since we haven't received successful acks to
        // move the state machine forward.
        return err;
    }
    fds_verify(msgRespErr == ERR_OK);

    result = txn->fds_set_smack_status(rxMsg->src_ip_lo_addr,
                                       rxMsg->src_port);
    fds_verify(result == 0);
    LOGDEBUG << "For transaction " << transId
              << " recvd PUT_OBJ_RESP from SM ip "
              << rxMsg->src_ip_lo_addr
              << " port " << rxMsg->src_port;
    result = fds_move_wr_req_state_machine(rxMsg);
    fds_verify(result == 0);

    return err;
}

void
StorHvCtrl::startBlobTxResp(const FDSP_MsgHdrTypePtr rxMsg) {
    Error err(ERR_OK);

    // TODO(Andrew): We don't really need this...
    fds_verify(rxMsg->msg_code == FDSP_START_BLOB_TX);

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
    fds_verify(txn->trans_state == FDS_TRANS_BLOB_START);

    // Get request from transaction
    fds::StartBlobTxReq *blobReq = static_cast<fds::StartBlobTxReq*>(txn->io);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_START_BLOB_TX);

    // Return if err
    if (rxMsg->result != FDSP_ERR_OK) {
        vol->journal_tbl->releaseTransId(transId);
        txn->reset();
        qos_ctrl->markIODone(txn->io);
        blobReq->cb->call(FDSN_StatusErrorUnknown);
        delete blobReq;
        return;
    }

    // Increment the ack status
    fds_int32_t result = txn->fds_set_dmack_status(rxMsg->src_ip_lo_addr,
                                                   rxMsg->src_port);
    fds_verify(result == 0);
    LOGDEBUG << "For AM transaction " << transId
             << " recvd start blob tx response from DM ip "
             << rxMsg->src_ip_lo_addr
             << " port " << rxMsg->src_port;

    // Move the state machine, which will call the callback
    // if we received enough messages
    // TODO(Andrew): It should just manage the state, not
    // actually do the work...
    result = fds_move_wr_req_state_machine(rxMsg);

    if (result == 1) {
        StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                      blobReq->cb);
        txn->reset();
        vol->journal_tbl->releaseTransId(transId);
        qos_ctrl->markIODone(txn->io);
        cb->call(ERR_OK);
        delete blobReq;
    } else {
        fds_verify(result == 0);
    }
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

fds::Error StorHvCtrl::upCatResp(const FDSP_MsgHdrTypePtr& rxMsg,
                                 const FDSP_UpdateCatalogTypePtr& catObjRsp) {
    fds::Error  err(ERR_OK);
    fds_int32_t result = 0;
    fds_verify(rxMsg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_RSP);

    fds_uint32_t transId = rxMsg->req_cookie;
    fds_volid_t volId    = rxMsg->glob_volume_id;

    StorHvVolume *vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);
    StorHvJournalEntryLock je_lock(txn);
    fds_verify(txn->isActive() == true);  // Should not get resp for inactive txns
    fds_verify(txn->trans_state != FDS_TRANS_EMPTY);  // Should not get resp for empty txns

    // Check response code
    Error msgRespErr(rxMsg->err_code);
    fds_verify(msgRespErr == ERR_OK);  
    if (catObjRsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
        result = txn->fds_set_dmack_status(rxMsg->src_ip_lo_addr,
                                           rxMsg->src_port);
        fds_verify(result == 0);
        LOGDEBUG << "Recvd DM TXN_STATUS_OPEN RSP for transId "
                  << transId  << " ip " << rxMsg->src_ip_lo_addr
                  << " port " << rxMsg->src_port;
    } else {
        fds_verify(catObjRsp->dm_operation == FDS_DMGR_TXN_STATUS_COMMITED);
        result = txn->fds_set_dm_commit_status(rxMsg->src_ip_lo_addr,
                                               rxMsg->src_port);
        fds_verify(result == 0);
        LOGDEBUG << "Recvd DM TXN_STATUS_COMMITED RSP for transId "
                  << transId  << " ip " << rxMsg->src_ip_lo_addr
                  << " port " << rxMsg->src_port;
    }  
    result = fds_move_wr_req_state_machine(rxMsg);
    fds_verify(result == 0);

    return err;
}

fds::Error StorHvCtrl::deleteCatResp(const FDSP_MsgHdrTypePtr& rxMsg,
                                     const FDSP_DeleteCatalogTypePtr& delCatRsp) {
    fds::Error err(ERR_OK);
    fds_int32_t result = 0;

    fds_verify(rxMsg->msg_code == FDSP_MSG_DELETE_BLOB_RSP);

    fds_uint32_t transId = rxMsg->req_cookie;
    fds_volid_t  volId   = rxMsg->glob_volume_id;

    StorHvVolume* vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);  // Should not receive resp for non existant vol

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);

    StorHvJournalEntryLock je_lock(txn);
    if (txn->isActive() != true) {
        /*
         * TODO: This is a HACK to get multi-node delete working!
         * We're going to ignore inactive transactions for now because
         * we always respond and clean up the transaction on the first
         * response, meaning the second response doesn't have a transaction
         * to update. This is a result of only partially implementing the
         * delete transaction state.
         * We don't call the callback or delete the blob request because we
         * assume it's been done already by the first response received.    
         */
        return err;
    }
    fds_verify(txn->isActive() == true);  // Should not receive resp for inactive txn
    fds_verify(txn->trans_state == FDS_TRANS_DEL_OBJ);

    /*
     * TODO: We're short cutting here and responding to the client when we get
     * only the resonse from a single DM. We need to finish this transaction by
     * tracking all of the responses from all SMs/DMs in the journal.
     */

    /*
     *  check the version of the object, return if the version is NULL
     */

    /*
     * start accumulating the ack from  DM and  check for the min ack
     */

    // Check response code
    Error msgRespErr(rxMsg->err_code);
    fds_verify(msgRespErr == ERR_OK || msgRespErr == ERR_BLOB_NOT_FOUND);

    if (rxMsg->msg_code == FDSP_MSG_DELETE_BLOB_RSP) {
        txn->fds_set_dmack_status(rxMsg->src_ip_lo_addr,
                                  rxMsg->src_port);
        LOGDEBUG << " StorHvisorRx:" << "IO-XID:" << transId
                  << " volID: 0x" << std::hex << volId << std::dec
                  << " -  Recvd DM TXN_STATUS_OPEN RSP "
                  << " ip " << rxMsg->src_ip_lo_addr
                  << " port " << rxMsg->src_port;
        result = fds_move_del_req_state_machine(rxMsg);
        fds_verify(result == 0);
        return err;
    }

    return err;
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

fds::Error StorHvCtrl::getBucketResp(const FDSP_MsgHdrTypePtr& rxMsg,
				     const FDSP_GetVolumeBlobListRespTypePtr& blobListResp)
{
    fds::Error err(ERR_OK);
    fds_uint32_t transId = rxMsg->req_cookie;
    fds_volid_t volId    = rxMsg->glob_volume_id;

    fds_verify(rxMsg->msg_code == FDS_ProtocolInterface::FDSP_MSG_GET_VOL_BLOB_LIST_RSP);

    StorHvVolume* vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);  // Should not receive resp for non existant vol

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);

    StorHvJournalEntryLock je_lock(txn);
    fds_verify(txn->isActive() == true);  // Should not receive resp for inactive txn
    fds_verify(txn->trans_state == FDS_TRANS_GET_BUCKET);


    /*
     * List of blobs ready, respond to callback
     */
    fds::AmRequest *blobReq = static_cast<fds::AmRequest *>(txn->io);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_VOLUME_CONTENTS);
    LOGDEBUG << "Responding to getBucket trans " << transId
              <<" for bucket " << blobReq->getBlobName()
              << " num of blobs " << blobListResp->num_blobs_in_resp
              << " end_of_list? " << blobListResp->end_of_list
              << " with result " << rxMsg->result;
    /*
     * Mark the IO complete, clean up txn, and callback
     */
    qos_ctrl->markIODone(txn->io);
    txn->reset();
    vol->journal_tbl->releaseTransId(transId);

    if (rxMsg->result == FDSP_ERR_OK) {
        ListBucketContents* contents = new ListBucketContents[blobListResp->num_blobs_in_resp];
        fds_verify(contents != NULL);
        fds_verify((uint)(blobListResp->num_blobs_in_resp) == (blobListResp->blob_info_list).size());
        for (int i = 0; i < blobListResp->num_blobs_in_resp; ++i)
        {
            contents[i].set((blobListResp->blob_info_list)[i].blob_name,
                            0, // last modified
                            "",  // eTag
                            (blobListResp->blob_info_list)[i].blob_size, 
                            "", // ownerId
                            "");
        }

        /* in case there are more blobs in the list, remember iter_cookie */
        VolumeContentsReq* list_buck = static_cast<VolumeContentsReq*>(blobReq);
        list_buck->iter_cookie = blobListResp->iterator_cookie;

        /* call VolumeContentsReq's callback directly */
        list_buck->DoCallback( (blobListResp->end_of_list == true) ? 0 : 1, //isTrancated == 0 if no more blobs to return?
                               "", // next_marker ?
                               blobListResp->num_blobs_in_resp,
                               contents,
                               ERR_OK,
                               NULL);

    } else {
        /*
         * We received an error from SM
         */
        blobReq->cbWithResult(-1);
    }

    /* if there are more blobs to return, we update blobReq with new iter_cookie 
     * that we got from SM and queue back to QoS queue 
     * TODO: or should we not release transaction ? */
    if (blobListResp->end_of_list == false) {
        LOGNOTIFY << "GetBucketResp -- bucket " << blobReq->getBlobName()
                  << " has more blobs, queueing request for next list of blobs";
        pushBlobReq(blobReq);
        return err;
    }

    /*
     * TODO: We're deleting the request structure. This assumes
     * that the caller got everything they needed when the callback
     * was invoked.
     */
    delete blobReq;

    return err;

}

fds::Error StorHvCtrl::getBucketStats(fds::AmRequest *amReq) {
    fds::Error err(ERR_OK);
    int om_err = 0;
  
    LOGDEBUG << "Doing a get bucket stats operation!";
  
    /*
     * Pull out the blob request
     */
    fds_verify(amReq->magicInUse() == true);

    fds_volid_t   volId = amReq->io_vol_id;
    fds_verify(volId == admin_vol_id); /* must be in admin queue */
  
    StorHvVolume *shVol = vol_table->getVolume(volId);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        LOGCRITICAL << "getBucketStats failed to get admin volume";
        amReq->cbWithResult(-1);
        err = ERR_NOT_FOUND;
        delete amReq;
        return err;
    }

    /*
     * Track how long the request was queued before get() dispatch
     * TODO: Consider moving to the QoS request
     */
    amReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    bool trans_in_progress = false;
    fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(amReq->getBlobName(),
                                                                     amReq->blob_offset,
                                                                     trans_in_progress);
    StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
  
    StorHvJournalEntryLock je_lock(journEntry);

    if ((trans_in_progress) || (journEntry->isActive())) {
        LOGERROR <<" StorHvisorTx:" << "IO-XID:" << transId 
                 << " - Transaction  is already in ACTIVE state, completing request "
                 << transId << " with ERROR(-2) ";
        // There is an ongoing transaciton for this bucket.
        // We should queue this up for later processing once that completes.
    
        // For now, return an error.
        amReq->cbWithResult(-2);
        err = ERR_NOT_IMPLEMENTED;
        delete amReq;
        return err;
    }

    journEntry->setActive();

    LOGDEBUG <<" StorHvisorTx:" << "IO-XID:" << transId << " volID: 0x"
              << std::hex << admin_vol_id << std::dec << " - Activated txn for req :" << transId;
 
    /*
     * Setup journal entry
     */
    journEntry->trans_state = FDS_TRANS_BUCKET_STATS;
    journEntry->sm_msg = NULL; 
    journEntry->dm_msg = NULL;
    journEntry->sm_ack_cnt = 0;
    journEntry->dm_ack_cnt = 0;
    journEntry->op = FDS_BUCKET_STATS;
    journEntry->data_obj_id.digest.clear(); 
    journEntry->data_obj_len = 0;
    journEntry->io = amReq;
  
    /* send request to OM */
    om_err = om_client->pushGetBucketStatsToOM(transId);

    if(om_err != 0) {
        LOGERROR <<" StorHvisorTx:" << "IO-XID:" << transId << " volID: 0x"
                 << std::hex << admin_vol_id << std::dec
                 << " -  Couldn't send get bucket stats to OM. Completing request with ERROR(-1)";
        amReq->cbWithResult(-1);
        err = ERR_NOT_FOUND;
        delete amReq;
        return err;
    }

    LOGDEBUG << " StorHvisorTx:" << "IO-XID:"
              << transId 
              << " - Sent async Get Bucket Stats request to OM";

    // Schedule a timer here to track the responses and the original request
    shVol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
    return err;
}

void StorHvCtrl::bucketStatsRespHandler(const FDSP_MsgHdrTypePtr& rx_msg,
					const FDSP_BucketStatsRespTypePtr& buck_stats) {
    storHvisor->getBucketStatsResp(rx_msg, buck_stats);
}

void StorHvCtrl::getBucketStatsResp(const FDSP_MsgHdrTypePtr& rx_msg,
				    const FDSP_BucketStatsRespTypePtr& buck_stats)
{
    fds_uint32_t transId = rx_msg->req_cookie;
 
    fds_verify(rx_msg->msg_code == FDS_ProtocolInterface::FDSP_MSG_GET_BUCKET_STATS_RSP);

    StorHvVolume* vol = vol_table->getVolume(admin_vol_id);
    fds_verify(vol != NULL);  // admin vol must always exist

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);

    StorHvJournalEntryLock je_lock(txn);
    fds_verify(txn->isActive() == true);  // Should not receive resp for inactive txn
    fds_verify(txn->trans_state == FDS_TRANS_BUCKET_STATS);

    /*
     * respond to caller with buckets' stats
     */
    fds::AmRequest *blobReq = static_cast<fds::AmRequest *>(txn->io);
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->io_type == FDS_BUCKET_STATS);
    LOGDEBUG << "Responding to getBucketStats trans " << transId
              << " number of buckets " << (buck_stats->bucket_stats_list).size()
              << " with result " << rx_msg->result;

    /*
     * Mark the IO complete, clean up txn, and callback
     */
    qos_ctrl->markIODone(txn->io);
    txn->reset();
    vol->journal_tbl->releaseTransId(transId);
    
    if (rx_msg->result == FDSP_ERR_OK) {
        BucketStatsContent* contents = NULL;
        int count = (buck_stats->bucket_stats_list).size();

        if (count > 0) {
            contents = new BucketStatsContent[count];
            fds_verify(contents != NULL);
            for (int i = 0; i < count; ++i)
            {
                contents[i].set((buck_stats->bucket_stats_list)[i].vol_name,
                                (buck_stats->bucket_stats_list)[i].rel_prio,
                                (buck_stats->bucket_stats_list)[i].performance,
                                (buck_stats->bucket_stats_list)[i].sla,
                                (buck_stats->bucket_stats_list)[i].limit);
            }
        }

        /* call BucketStats callback directly */
        VolumeStatsReq *stats_req = static_cast<VolumeStatsReq*>(blobReq);
        stats_req->DoCallback(buck_stats->timestamp, count, contents, ERR_OK, NULL);

    } else {    
        /*
         * We received an error from OM
         */
        blobReq->cbWithResult(-1);
    }
      /*
     * TODO: We're deleting the request structure. This assumes
     * that the caller got everything they needed when the callback
     * was invoked.
     */
    delete blobReq;
}

