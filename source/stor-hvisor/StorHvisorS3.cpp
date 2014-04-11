#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include "StorHvisorCPP.h"
#include "hvisor_lib.h"
#include <hash/MurmurHash3.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetSession.h"
#include <dlt.h>
#include <ObjectId.h>

extern StorHvCtrl *storHvisor;
using namespace std;
using namespace FDS_ProtocolInterface;
typedef fds::hash::Sha1 GeneratorHash;

std::atomic_uint nextIoReqId;

fds::Error StorHvCtrl::pushBlobReq(fds::FdsBlobReq *blobReq) {
    fds_verify(blobReq->magicInUse() == true);
    fds::Error err(ERR_OK);

    /*
     * Pack the blobReq in to a qosReq to pass to QoS
     */
    fds_uint32_t reqId = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
    AmQosReq *qosReq  = new AmQosReq(blobReq, reqId);
    fds_volid_t volId = blobReq->getVolId();

    fds::StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    if ((shVol == NULL) || (shVol->volQueue == NULL)) {
        if (shVol)
            shVol->readUnlock();
        LOGERROR << "Volume and queueus are NOT setup for volume " << volId;
        err = fds::ERR_INVALID_ARG;
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

/*
 * TODO: Actually calculate the host's IP
 */
#define SRC_IP           0x0a010a65

/**
 * Dispatches async FDSP messages to SMs for a put msg.
 * Note, assumes the journal entry lock is held already
 * and that the messages are ready to be dispatched.
 */
fds::Error
StorHvCtrl::dispatchSmPutMsg(StorHvJournalEntry *journEntry) {
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

        // Get specific SM's info
        dataPlacementTbl->getNodeInfo(dltPtr->get(i).uuid_get_val(),
                                      &node_ip,
                                      &node_port,
                                      &node_state);
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
            LOGNORMAL << "For transaction " << journEntry->trans_id
                      << " sent async PUT_OBJ_REQ to SM ip "
                      << node_ip << " port " << node_port;
        } catch (att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what() ;
            errcount++;
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

    LOGNORMAL << "For transaction: " << journEntry->trans_id << "node_ip:" << node_ip << "node_port" << node_port;
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

        LOGNORMAL << "For trans " << journEntry->trans_id
                  << " sent async GetObj req to SM";
    } catch (att::TTransportException& e) {
        LOGERROR << "error during network call : " << e.what() ;
    }

    return err;
}

fds::Error StorHvCtrl::putBlob(fds::AmQosReq *qosReq) {
    fds::Error err(ERR_OK);
    
    /*
     * Pull out the blob request
     */
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(qosReq->getBlobReqPtr());
    bool fZeroSize = (blobReq->getDataLen() == 0);
    fds_verify(blobReq->magicInUse() == true);

    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        if (shVol) {
            shVol->readUnlock();
        }
        LOGCRITICAL << "putBlob failed to get volume for vol "
                    << volId;
    
        blobReq->cbWithResult(-1);
        err = ERR_DISK_WRITE_FAILED;
        delete qosReq;
        return err;
    }

    /*
     * Track how long the request was queued before put() dispatch
     * TODO: Consider moving to the QoS request
     */
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    /*
     * Get/lock a journal entry for the request.
     */
    bool trans_in_progress = false;
    fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                     blobReq->getBlobOffset(),
                                                                     trans_in_progress);

    LOGNORMAL << "Assigning transaction ID " << transId
              << " to put request";

    StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
    fds_verify(journEntry != NULL);
    StorHvJournalEntryLock jeLock(journEntry);

    /*
     * Check if the entry is already active.
     */
    if ((trans_in_progress) || (journEntry->isActive() == true)) {
        /*
         * There is an on-going transaction for this offset
         * Queue this up for later processing.
         */

        // TODO: For now, just return error :-(
        shVol->readUnlock();
        LOGCRITICAL << "Transaction " << transId << " is already ACTIVE"
                    << ", just give up and return error.";
        blobReq->cbWithResult(-2);
        err = ERR_NOT_IMPLEMENTED;
        journEntry->reset();
        shVol->journal_tbl->releaseTransId(transId);
        delete qosReq;
        return err;
    }
    journEntry->setActive();

    /*
     * Hash the data to obtain the ID
     */
    ObjectID objId;
     if (fZeroSize) {
        LOGWARN << "zero size object - "
                << " [objkey:" << blobReq->ObjKey <<"]";
     } else {
         objId = ObjIdGen::genObjectId(blobReq->getDataBuf(),
                                       blobReq->getDataLen());
     }

    blobReq->setObjId(objId);

    FDSP_MsgHdrTypePtr msgHdrSm(new FDSP_MsgHdrType);
    FDSP_MsgHdrTypePtr msgHdrDm(new FDSP_MsgHdrType);
    msgHdrSm->glob_volume_id    = volId;
    msgHdrDm->glob_volume_id    = volId;
    msgHdrSm->origin_timestamp = fds::util::getTimeStampMillis();
    msgHdrDm->origin_timestamp = fds::util::getTimeStampMillis();

    /*
     * Setup network put object & catalog request
     */
    FDSP_PutObjTypePtr put_obj_req(new FDSP_PutObjType);
    put_obj_req->data_obj = std::string((const char *)blobReq->getDataBuf(),
                                        (size_t )blobReq->getDataLen());
    put_obj_req->data_obj_len = blobReq->getDataLen();

    FDSP_UpdateCatalogTypePtr upd_obj_req(new FDSP_UpdateCatalogType);
    upd_obj_req->obj_list.clear();

    FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
    upd_obj_info.offset   = blobReq->getBlobOffset();  // May need to change to 0 for now?
    upd_obj_info.size     = blobReq->getDataLen();
    // Mark whether this update updates the end of the blob
    upd_obj_info.blob_end = blobReq->isLastBuf();

    put_obj_req->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    upd_obj_info.data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());

    upd_obj_req->obj_list.push_back(upd_obj_info);
    upd_obj_req->meta_list.clear();
    upd_obj_req->blob_size = blobReq->getDataLen();  // Size of the whole blob? Or just what I'm putting
    // If this is the last put in the stream, lets
    // write the etag/md5 with the blob's metadata
    if (blobReq->isLastBuf() == true) {
        std::string etagKey   = "etag";
        std::string etagValue = blobReq->getEtag();
        if (blobReq->getDataLen() > 0) {
            fds_verify(etagValue.size() == 32);
        }
        FDS_ProtocolInterface::FDSP_MetaDataPair mdPair;
        mdPair.__set_key(etagKey);
        mdPair.__set_value(etagValue);
        upd_obj_req->meta_list.push_back(mdPair);
    }

    /*
     * Initialize the journEntry with a open txn
     */
    journEntry->trans_state = FDS_TRANS_OPEN;
    journEntry->io = qosReq;
    journEntry->sm_msg = msgHdrSm;
    journEntry->dm_msg = msgHdrDm;
    journEntry->sm_ack_cnt = 0;
    journEntry->dm_ack_cnt = 0;
    journEntry->dm_commit_cnt = 0;
    journEntry->op = FDS_IO_WRITE;
    journEntry->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    journEntry->data_obj_len = blobReq->getDataLen();
    journEntry->putMsg = put_obj_req;

    InitSmMsgHdr(msgHdrSm);
    msgHdrSm->src_ip_lo_addr = SRC_IP;
    //  msgHdrSm->src_node_name = my_node_name;
    msgHdrSm->src_node_name = storHvisor->myIp;
    msgHdrSm->req_cookie = transId;

    LOGNOTIFY << "Putting object " << objId << " for blob "
              << blobReq->getBlobName() << " at offset "
              << blobReq->getBlobOffset() << " with length "
              << blobReq->getDataLen() << " src node ip"
              << msgHdrSm->src_node_name << " in trans"
              << transId;

    if (!fZeroSize) {
        err = dispatchSmPutMsg(journEntry);
    } else {
        // special case for zero size
        // trick the state machine to think that it 
        // has gotten responses from SM's
        DltTokenGroupPtr dltPtr;
        dltPtr = dataPlacementTbl->getDLTNodesForDoidKey(objId);
        fds_verify(dltPtr != NULL);

        fds_uint32_t numNodes = dltPtr->getLength();
        fds_verify(numNodes > 0);
        journEntry->sm_ack_cnt = numNodes;
        LOGWARN << "not sending put request to sm as obj is zero size - ["
                << " id:" << objId
                << " objkey:" << blobReq->ObjKey
                << " name:" << blobReq->getBlobName()
                << "]";
    }

    if (err == ERR_NETWORK_TRANSPORT) { 
        shVol->readUnlock();
        LOGCRITICAL << "Transaction " << transId << " hits network transport error "
                    << ", just give up and return error.";
        blobReq->cbWithResult(-2);
        err = ERR_NOT_IMPLEMENTED;
        journEntry->reset();
        shVol->journal_tbl->releaseTransId(transId);
        delete qosReq;
        return err;
    }

    fds_verify(err == ERR_OK );

    /*
     * Setup DM messages
     */
    fds_uint32_t numNodes = MAX_DM_NODES;  // TODO: Why 8? Use vol/blob repl factor
    InitDmMsgHdr(msgHdrDm);
    upd_obj_req->blob_name = blobReq->getBlobName();
    upd_obj_req->dm_transaction_id  = 1;  // TODO: Don't hard code
    upd_obj_req->dm_operation       = FDS_DMGR_TXN_STATUS_OPEN;
    msgHdrDm->req_cookie     = transId;
    msgHdrDm->src_ip_lo_addr = SRC_IP;
    //  msgHdrDm->src_node_name  = my_node_name;
    msgHdrSm->src_node_name = storHvisor->myIp;
    msgHdrDm->src_port       = 0;
    fds_uint64_t nodeIds[numNodes];
    memset(nodeIds, 0x00, sizeof(fds_int64_t) * numNodes);
    dataPlacementTbl->getDMTNodesForVolume(volId, nodeIds, (int*)&numNodes);
    fds_verify(numNodes > 0);

    /*
     * Update the catalog for each DMT entry
     */
    uint errcount = 0;
    for (fds_uint32_t i = 0; i < numNodes; i++) {
        fds_uint32_t node_ip   = 0;
        fds_uint32_t node_port = 0;
        fds_int32_t node_state = -1;
        dataPlacementTbl->getNodeInfo(nodeIds[i],
                                      &node_ip,
                                      &node_port,
                                      &node_state);
    
        journEntry->dm_ack[i].ipAddr        = node_ip;
        journEntry->dm_ack[i].port          = node_port;
        msgHdrDm->dst_ip_lo_addr            = node_ip;
        msgHdrDm->dst_port                  = node_port;
        journEntry->dm_ack[i].ack_status    = FDS_CLS_ACK;
        journEntry->dm_ack[i].commit_status = FDS_CLS_ACK;
        journEntry->num_dm_nodes            = numNodes;
    
        // Call Update Catalog RPC call to DM
        try {
            netMetaDataPathClientSession *sessionCtx =
                    storHvisor->rpcSessionTbl->\
                    getClientSession<netMetaDataPathClientSession>(node_ip, node_port);
            fds_verify(sessionCtx != NULL);

            boost::shared_ptr<FDSP_MetaDataPathReqClient> client = sessionCtx->getClient();
            msgHdrDm->session_uuid = sessionCtx->getSessionId();
            journEntry->session_uuid = msgHdrDm->session_uuid;
            client->UpdateCatalogObject(msgHdrDm, upd_obj_req);

            LOGNORMAL << "For transaction " << transId
                      << " sent async UP_CAT_REQ to DM ip "
                      << node_ip << " port " << node_port;
        } catch (att::TTransportException& e) {
            LOGERROR << "error during network call : " << e.what() ;
            errcount++;
        }
    }

    // Schedule a timer here to track the responses and the original request
    shVol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));

    // Release the vol read lock
    shVol->readUnlock();

    /*
     * Note je_lock destructor will unlock the journal entry automatically
     */
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
                dispatchSmPutMsg(journEntry);
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
        // Note: We're expecting the server to specify the version
        // expected in the response field.
        LOGERROR << "For transaction " << transId
                 << " received DLT version mismatch, have "
                 << om_client->getDltVersion()
                 << ", but SM service expected "
                 << putObjRsp->dlt_version;
        // handleDltMismatch(vol, txn);
       
        // response message  has the  latest  DLT , update the local copy 
        // and update the DLT version 
        storHvisor->om_client->updateDlt(true, putObjRsp->dlt_data);

        // resend the IO with latest DLT
        storHvisor->dispatchSmPutMsg(txn);
        // Return here since we haven't received successful acks to
        // move the state machine forward.
        return err;
    }
    fds_verify(msgRespErr == ERR_OK);

    result = txn->fds_set_smack_status(rxMsg->src_ip_lo_addr,
                                       rxMsg->src_port);
    fds_verify(result == 0);
    LOGNORMAL << "For transaction " << transId
              << " recvd PUT_OBJ_RESP from SM ip "
              << rxMsg->src_ip_lo_addr
              << " port " << rxMsg->src_port;
    result = fds_move_wr_req_state_machine(rxMsg);
    fds_verify(result == 0);

    return err;
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
    fds_verify(txn->isActive() == true); // Should not get resp for inactive txns
    fds_verify(txn->trans_state != FDS_TRANS_EMPTY);  // Should not get resp for empty txns

    // Check response code
    Error msgRespErr(rxMsg->err_code);
    fds_verify(msgRespErr == ERR_OK);
  
    if (catObjRsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
        result = txn->fds_set_dmack_status(rxMsg->src_ip_lo_addr,
                                           rxMsg->src_port);
        fds_verify(result == 0);
        LOGNORMAL << "Recvd DM TXN_STATUS_OPEN RSP for transId "
                  << transId  << " ip " << rxMsg->src_ip_lo_addr
                  << " port " << rxMsg->src_port;
    } else {
        fds_verify(catObjRsp->dm_operation == FDS_DMGR_TXN_STATUS_COMMITED);
        result = txn->fds_set_dm_commit_status(rxMsg->src_ip_lo_addr,
                                               rxMsg->src_port);
        fds_verify(result == 0);
        LOGNORMAL << "Recvd DM TXN_STATUS_COMMITED RSP for transId "
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
        LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << transId
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

fds::Error StorHvCtrl::getBlob(fds::AmQosReq *qosReq) {
    fds::Error err(ERR_OK);

    LOGNORMAL << "Doing a get blob operation!";

    /*
     * Pull out the blob request
     */
    FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq->magicInUse() == true);

    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = vol_table->getVolume(volId);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        LOGCRITICAL << "getBlob failed to get volume for vol "
                    << volId;    
        blobReq->cbWithResult(-1);
        err = ERR_DISK_WRITE_FAILED;
        delete qosReq;
        return err;
    }

    /*
     * Track how long the request was queued before get() dispatch
     * TODO: Consider moving to the QoS request
     */
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    /*
     * Get/lock a journal entry for the request.
     */
    bool trans_in_progress = false;
    fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                     blobReq->getBlobOffset(),
                                                                     trans_in_progress);

    LOGNORMAL << "Assigning transaction ID " << transId
              << " to get request";

    StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
    fds_verify(journEntry != NULL);
    StorHvJournalEntryLock jeLock(journEntry);

    /*
     * Check if the entry is already active.
     */
    if ((trans_in_progress) || (journEntry->isActive() == true)) {
        /*
         * There is an on-going transaction for this offset
         * Queue this up for later processing.
         */

        // TODO: For now, just return error :-(
        LOGCRITICAL << "Transaction " << transId << " is already ACTIVE"
                    << ", just give up and return error.";
        blobReq->cbWithResult(-2);
        err = ERR_NOT_IMPLEMENTED;
        delete qosReq;
        return err;
    }
    journEntry->setActive();

    /*
     * Setup msg header
     */
    //  FDSP_MsgHdrTypePtr msgHdr = new FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr(new FDSP_MsgHdrType);
    InitSmMsgHdr(msgHdr);
    msgHdr->msg_code       = FDSP_MSG_GET_OBJ_REQ;
    msgHdr->msg_id         =  1;
    msgHdr->req_cookie     = transId;
    msgHdr->glob_volume_id = volId;
    msgHdr->src_ip_lo_addr = SRC_IP;
    msgHdr->src_port       = 0;
    //  msgHdr->src_node_name  = my_node_name;
    msgHdr->src_node_name = storHvisor->myIp;

    /*
     * Setup journal entry
     */
    journEntry->trans_state = FDS_TRANS_OPEN;
    // journEntry->write_ctx = (void *)req;
    // journEntry->comp_req = comp_req;
    // journEntry->comp_arg1 = arg1; // vbd
    // journEntry->comp_arg2 = arg2; //vreq
    journEntry->sm_msg = msgHdr; 
    journEntry->dm_msg = NULL;
    journEntry->sm_ack_cnt = 0;
    journEntry->dm_ack_cnt = 0;
    // TODO(Andrew): Depricate FDS_IO_READ in favor
    // of FDS_GET_BLOB
    journEntry->op = FDS_IO_READ;
    journEntry->data_obj_id.digest.clear(); 
    journEntry->data_obj_len = blobReq->getDataLen();
    journEntry->io = qosReq;
    journEntry->trans_state = FDS_TRANS_GET_OBJ;

    FDSP_GetObjTypePtr get_obj_req(new FDSP_GetObjType);
    journEntry->getMsg = get_obj_req;

    /*
     * Get the object ID from vcc and add it to journal entry and get msg
     */
    if (useVcc == true) {
        shVol->vol_catalog_cache->Clear();
    }
    // TODO(Andrew): Here we need to check if the offset is aligned or handle
    // unaligned offsets by returning multiple object ids and a desired length
    ObjectID objId;
    err = shVol->vol_catalog_cache->Query(blobReq->getBlobName(),
                                          blobReq->getBlobOffset(),
                                          transId,
                                          &objId);
    if (err == ERR_PENDING_RESP) {
        LOGNOTIFY << "Vol catalog cache query pending: "
                  << " for blob " << blobReq->getBlobName()
                  << " and offset " << blobReq->getBlobOffset()
                  << " with err " << err;
        journEntry->trans_state = FDS_TRANS_VCAT_QUERY_PENDING;
        err = ERR_OK;
        return err;
    }

    bool fSizeZero = (objId == NullObjectID);

    fds_verify(err == ERR_OK);
    journEntry->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());

    get_obj_req->data_obj_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
    get_obj_req->data_obj_len = blobReq->getDataLen();
    if (fSizeZero) {
        LOGWARN << "zero size object - not sending get request to sm -"
                << " name:" << blobReq->getBlobName()
                << " txnid:" << transId;
        
        storHvisor->qos_ctrl->markIODone(journEntry->io);
        journEntry->trans_state = FDS_TRANS_DONE;

        blobReq->setDataLen(0);
        blobReq->cbWithResult(FDSN_StatusEntityEmpty);
        journEntry->reset();
        shVol->journal_tbl->releaseTransId(transId);
    } else {
        journEntry->trans_state = FDS_TRANS_GET_OBJ;
        LOGNORMAL << "Getting object " << objId << " for blob "
                  << blobReq->getBlobName() << " and offset "
                  << blobReq->getBlobOffset() << " in trans "
                  << transId;
        journEntry->nodeSeq = 0; // Primary node
        err = dispatchSmGetMsg(journEntry);
        fds_verify(err == ERR_OK);
        // Schedule a timer here to track the responses and the original request
        shVol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
    }
    /*
     * Note je_lock destructor will unlock the journal entry automatically
     */
    return err;
}

fds::Error StorHvCtrl::getObjResp(const FDSP_MsgHdrTypePtr& rxMsg,
                                  const FDSP_GetObjTypePtr& getObjRsp) {
    fds::Error err(ERR_OK);
    fds_verify(rxMsg->msg_code == FDSP_MSG_GET_OBJ_RSP);

    fds_uint32_t transId = rxMsg->req_cookie;
    fds_volid_t volId    = rxMsg->glob_volume_id;

    StorHvVolume* vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);  // Should not receive resp for non existant vol

    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);

    StorHvJournalEntryLock je_lock(txn);
    fds_verify(txn->isActive() == true);  // Should not receive resp for inactive txn
    // Check response code
    Error msgRespErr(rxMsg->err_code);
    if (msgRespErr == ERR_IO_DLT_MISMATCH) {
        // Note: We're expecting the server to specify the version
        // expected in the response field.
        LOGERROR << "For transaction " << transId
                 << " received DLT version mismatch, have "
                 << om_client->getDltVersion()
                 << ", but SM service expected "
                 << getObjRsp->dlt_version;
        //handleDltMismatch(vol, txn);

        // response message  has the  latest  DLT , update the local copy 
        // and update the DLT version 
        storHvisor->om_client->updateDlt(true, getObjRsp->dlt_data);

        txn->nodeSeq = 0; // Primary node
        // resend the IO with latest DLT
        storHvisor->dispatchSmGetMsg(txn);
        // Return here since we haven't received successful acks to
        // move the state machine forward.
        return err;
    }

    fds_verify(rxMsg->result == FDS_ProtocolInterface::FDSP_ERR_OK);
    fds_verify(txn->trans_state == FDS_TRANS_GET_OBJ);

    /*
     * how to handle the length miss-match (requested  length and  recived legth from SM differ)?
     * We will have to handle sending more data due to length difference
     */

    /*
     * Data ready, respond to callback
     */
    fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
    fds_verify(qosReq != NULL);
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(
        qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);

    fds_uint64_t blobSize = 0;
    err = vol->vol_catalog_cache->getBlobSize(blobReq->getBlobName(),
                                              &blobSize);
    fds_verify(err == ERR_OK);
    fds_verify(blobSize > 0);

    std::string blobEtag;
    err = vol->vol_catalog_cache->getBlobEtag(blobReq->getBlobName(),
                                              &blobEtag);
    fds_verify(err == ERR_OK);
    // Either the etag is empty or its set to
    // the proper length
    fds_verify((blobEtag.size() == 0) ||
               (blobEtag.size() == 32));

    LOGNOTIFY << "Responding to getBlob trans " << transId
              <<" for blob " << blobReq->getBlobName()
              << " and offset " << blobReq->getBlobOffset()
              << " length " << getObjRsp->data_obj_len
              << " total blob size " << blobSize
              << " etag " << blobEtag
              << " with result " << rxMsg->result;
    /*
     * Mark the IO complete, clean up txn, and callback
     */
    qos_ctrl->markIODone(txn->io);
    if (rxMsg->result == FDSP_ERR_OK) {
        fds_verify(blobReq->getIoType() == FDS_GET_BLOB);
        /* NOTE: we are currently supporting only getting the whole blob
         * so the requester does not know about the blob length, 
         * we get the blob length in response from SM;
         * will need to revisit when we also support (don't ignore) byteCount in native api.
         * For now, just verify the existing buffer is big enough to hold
         * the data.
         */
        fds_verify((uint)(getObjRsp->data_obj_len) <= (blobReq->getDataLen()));
        blobReq->setDataLen(getObjRsp->data_obj_len);    
        blobReq->setDataBuf(getObjRsp->data_obj.c_str());
        blobReq->setBlobSize(blobSize);
        blobReq->setBlobEtag(blobEtag);
        blobReq->cbWithResult(0);
        txn->reset();
        vol->journal_tbl->releaseTransId(transId);
    } else {
        /*
         * We received an error from SM. check the Error. If the Obj Not found 
         * send a get object request to secondary SM
         */
       if (txn->nodeSeq != txn->num_sm_nodes) { 
           txn->nodeSeq += 1; // secondary node 
           err = dispatchSmGetMsg(txn);
           fds_verify(err == ERR_OK);
 
       }
       else {
        vol->journal_tbl->releaseTransId(transId);
        blobReq->cbWithResult(-1);
        txn->reset();
        delete blobReq;
        blobReq = NULL;
       }
    }

    /*
     * TODO: We're deleting the request structure. This assumes
     * that the caller got everything they needed when the callback
     * was invoked.
     */
    delete blobReq;

    return err;
}

/*****************************************************************************

 *****************************************************************************/
fds::Error StorHvCtrl::deleteBlob(fds::AmQosReq *qosReq) {
    netSession *endPoint = NULL;
    unsigned int node_ip = 0;
    fds_uint32_t node_port = 0;
    unsigned int doid_dlt_key=0;
    int num_nodes = MAX_DM_NODES, i =0;
    int node_state = -1;
    fds::Error err(ERR_OK);
    ObjectID oid;
    FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq->magicInUse() == true);
    DeleteBlobReq *del_blob_req = (DeleteBlobReq *)blobReq;

    fds_volid_t   vol_id = blobReq->getVolId();
    StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(vol_id);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        shVol->readUnlock();
        LOGCRITICAL << "deleteBlob failed to get volume for vol 0x"
                    << std::hex << vol_id << std::dec;
    
        blobReq->cbWithResult(-1);
        err = ERR_DISK_WRITE_FAILED;
        delete qosReq;
        return err;
    }

    /* Check if there is an outstanding transaction for this block offset  */
    bool trans_in_progress = false;
    fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                     blobReq->getBlobOffset(),
                                                                     trans_in_progress);
    StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
  
    StorHvJournalEntryLock je_lock(journEntry);
  
    if ((trans_in_progress) || (journEntry->isActive())) {
        shVol->readUnlock();

        LOGNORMAL <<" StorHvisorTx:" << "IO-XID:" << transId << " - Transaction  is already in ACTIVE state, completing request "
                  << transId << " with ERROR(-2) ";
        // There is an ongoing transaciton for this offset.
        // We should queue this up for later processing once that completes.
    
        // For now, return an error.
        blobReq->cbWithResult(-2);
        err = ERR_NOT_IMPLEMENTED;
        delete qosReq;
        return err;
    }

    journEntry->setActive();

    FDS_ProtocolInterface::FDSP_DeleteObjTypePtr del_obj_req(new FDSP_DeleteObjType);
  
    //  journEntry->trans_state = FDS_TRANS_OPEN;
    journEntry->sm_msg = NULL; 
    journEntry->dm_msg = NULL;
    journEntry->sm_ack_cnt = 0;
    journEntry->dm_ack_cnt = 0;
    journEntry->op = FDS_DELETE_BLOB;
    journEntry->data_obj_id.digest.clear(); 
    journEntry->data_obj_len = blobReq->getDataLen();
    journEntry->io = qosReq;
    journEntry->delMsg = del_obj_req;
    journEntry->trans_state = FDS_TRANS_DEL_OBJ;

    LOGNORMAL << "Deleting blob " << blobReq->getBlobName() << " in trans "
              << transId << " and vol 0x" << std::hex << vol_id << std::dec;
    
    // SAN- check the  version of the object. If the object version NULL ( object deleted) return

    del_obj_req->data_obj_id.digest = std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
    del_obj_req->data_obj_len = blobReq->getDataLen();
  
    journEntry->op = FDS_DELETE_BLOB;
    journEntry->data_obj_id.digest = std::string((const char *)oid.GetId(), (size_t)oid.GetLen());
    journEntry->trans_state = FDS_TRANS_DEL_OBJ;

    // Invalidate the local cache entry for the blob.
    // We can do this here, even before sending the messages to DM
    // since even if the delete fails, it'll just produce a later
    // cache miss that will go to DM anyways.
    err = shVol->vol_catalog_cache->clearEntry(blobReq->getBlobName());
    fds_verify(err == ERR_OK);
  
    // RPC Call DeleteCatalogObject to DataMgr
    FDS_ProtocolInterface::FDSP_DeleteCatalogTypePtr del_cat_obj_req(new FDSP_DeleteCatalogType);
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm(new FDSP_MsgHdrType);
    storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
    fdsp_msg_hdr_dm->msg_code = FDSP_MSG_DELETE_BLOB_REQ;
    fdsp_msg_hdr_dm->glob_volume_id = vol_id;
    fdsp_msg_hdr_dm->req_cookie = transId;
    fdsp_msg_hdr_dm->src_ip_lo_addr = SRC_IP;
    //  fdsp_msg_hdr_dm->src_node_name = storHvisor->my_node_name;
    fdsp_msg_hdr_dm->src_node_name = storHvisor->myIp;
    fdsp_msg_hdr_dm->src_port = 0;
    fdsp_msg_hdr_dm->dst_port = node_port;
    journEntry->dm_msg = fdsp_msg_hdr_dm;
    num_nodes = MAX_DM_NODES;
    fds_uint64_t  node_ids[num_nodes];
    memset(node_ids, 0x00, sizeof(fds_int64_t) * num_nodes);
    storHvisor->dataPlacementTbl->getDMTNodesForVolume(vol_id, node_ids, &num_nodes);
    uint errcount = 0;
    for (i = 0; i < num_nodes; i++) {
        node_ip = 0;
        node_port = 0;
        node_state = -1;
        storHvisor->dataPlacementTbl->getNodeInfo(node_ids[i],
                                                  &node_ip,
                                                  &node_port,
                                                  &node_state);
    
        journEntry->dm_ack[i].ipAddr = node_ip;
        journEntry->dm_ack[i].port = node_port;
        fdsp_msg_hdr_dm->dst_ip_lo_addr = node_ip;
        fdsp_msg_hdr_dm->dst_port = node_port;
        journEntry->dm_ack[i].ack_status = FDS_CLS_ACK;
        journEntry->dm_ack[i].commit_status = FDS_CLS_ACK;
        journEntry->num_dm_nodes = num_nodes;
        del_cat_obj_req->blob_name = blobReq->getBlobName();
        // TODO(Andrew): Set to a specific version rather than
        // always just the current
        del_cat_obj_req->blob_version = blob_version_invalid;

        // Call Update Catalog RPC call to DM
        try {
            netMetaDataPathClientSession *sessionCtx =
                    storHvisor->rpcSessionTbl->\
                    getClientSession<netMetaDataPathClientSession>(node_ip, node_port);
            fds_verify(sessionCtx != NULL);
            boost::shared_ptr<FDSP_MetaDataPathReqClient> client =
                    sessionCtx->getClient();

            fdsp_msg_hdr_dm->session_uuid = sessionCtx->getSessionId();
            journEntry->session_uuid = fdsp_msg_hdr_dm->session_uuid;
            client->DeleteCatalogObject(fdsp_msg_hdr_dm, del_cat_obj_req);
            LOGNORMAL << " txnid:" << transId
                      << " volID:" << std::hex << vol_id << std::dec
                      << " - sent DELETE_BLOB_REQ request to DM @ "
                      <<  node_ip << ":" << node_port;
        } catch (att::TTransportException& e) {
            errcount ++;
            LOGERROR << "error during network call: ["<< errcount << "] : " << e.what() ;
        }
    }

    shVol->readUnlock();

    return ERR_OK; // je_lock destructor will unlock the journal entry
}

fds::Error StorHvCtrl::listBucket(fds::AmQosReq *qosReq) {
    fds::Error err(ERR_OK);
    netSession *endPoint = NULL;
    unsigned int node_ip = 0;
    fds_uint32_t node_port = 0;
    int node_state = -1;

    LOGNORMAL << "Doing a list bucket operation!";

    /*
     * Pull out the blob request
     */
    FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq->magicInUse() == true);

    fds_volid_t   volId = blobReq->getVolId();
    StorHvVolume *shVol = vol_table->getVolume(volId);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        LOGCRITICAL << "listBucket failed to get volume for vol "
                    << volId;    
        blobReq->cbWithResult(-1);
        err = ERR_NOT_FOUND;
        delete qosReq;
        return err;
    }

    /*
     * Track how long the request was queued before get() dispatch
     * TODO: Consider moving to the QoS request
     */
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    bool trans_in_progress = false;
    fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                     blobReq->getBlobOffset(),
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
        blobReq->cbWithResult(-2);
        err = ERR_INVALID_ARG;
        delete qosReq;
        return err;
    }

    journEntry->setActive();

    LOGNORMAL << " StorHvisorTx:" << "IO-XID:" << transId << " volID: 0x"
              << std::hex << volId << std::dec << " - Activated txn for req :" << transId;

    /*
     * Setup msg header
     */

    FDSP_MsgHdrTypePtr msgHdr(new FDSP_MsgHdrType);
    InitDmMsgHdr(msgHdr);
    msgHdr->msg_code       = FDSP_MSG_GET_VOL_BLOB_LIST_REQ;
    msgHdr->msg_id         =  1;
    msgHdr->req_cookie     = transId;
    msgHdr->glob_volume_id = volId;
    msgHdr->src_ip_lo_addr = SRC_IP;
    msgHdr->src_port       = 0;
    //  msgHdr->src_node_name  = my_node_name;
    msgHdr->src_node_name = storHvisor->myIp;
    msgHdr->bucket_name    = blobReq->getBlobName(); /* ListBucketReq stores bucket name in blob name */
 
    /*
     * Setup journal entry
     */
    journEntry->trans_state = FDS_TRANS_GET_BUCKET;
    journEntry->sm_msg = NULL; 
    journEntry->dm_msg = msgHdr;
    journEntry->sm_ack_cnt = 0;
    journEntry->dm_ack_cnt = 0;
    journEntry->op = FDS_LIST_BUCKET;
    journEntry->data_obj_id.digest.clear(); 
    journEntry->data_obj_len = 0;
    journEntry->io = qosReq;
    fds_int32_t num_nodes = MAX_DM_NODES;  // TODO: Why 8? Look up vol/blob repl factor
    fds_uint64_t node_ids[num_nodes];  // TODO: Doesn't need to be signed
    memset(node_ids, 0x00, sizeof(fds_int64_t) * num_nodes);
    dataPlacementTbl->getDMTNodesForVolume(volId, node_ids, &num_nodes);

    if(num_nodes == 0) {
        LOGERROR <<" StorHvisorTx:" << "IO-XID:" << transId << " volID: 0x" << std::hex << volId << std::dec 
                 << " -  DMT Nodes  NOT  configured. Check on OM Manager. Completing request with ERROR(-1)";
        blobReq->cbWithResult(-1);
        err = ERR_GET_DMT_FAILED;
        delete qosReq;
        return err;
    }

    /* getting from first DM in the list */
    FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqTypePtr get_bucket_list_req
            (new FDS_ProtocolInterface::FDSP_GetVolumeBlobListReqType);

    node_ip = 0;
    node_port = 0;
    node_state = -1;
    dataPlacementTbl->getNodeInfo(node_ids[0],
                                  &node_ip,
                                  &node_port,
                                  &node_state);
    
    msgHdr->dst_ip_lo_addr = node_ip;
    msgHdr->dst_port = node_port;

    get_bucket_list_req->max_blobs_to_return = static_cast<ListBucketReq*>(blobReq)->maxkeys;
    get_bucket_list_req->iterator_cookie = static_cast<ListBucketReq*>(blobReq)->iter_cookie;

    // Call Get Volume Blob List to DM
    try {
        netMetaDataPathClientSession *sessionCtx =
                storHvisor->rpcSessionTbl->\
                getClientSession<netMetaDataPathClientSession>(node_ip, node_port);
        fds_verify(sessionCtx != NULL);
        boost::shared_ptr<FDSP_MetaDataPathReqClient> client = sessionCtx->getClient();

        msgHdr->session_uuid = sessionCtx->getSessionId();
        journEntry->session_uuid = msgHdr->session_uuid;
        client->GetVolumeBlobList(msgHdr, get_bucket_list_req);
        LOGNORMAL << " StorHvisorTx:" << "IO-XID:"
                  << transId << " volID:" << std::hex << volId << std::dec
                  << " - Sent async GET_VOL_BLOB_LIST_REQ request to DM at "
                  <<  node_ip << " port " << node_port;
        // Schedule a timer here to track the responses and the original request
        shVol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
    } catch (att::TTransportException& e) {
        LOGERROR << "error during network call : " << e.what() ;
    }
    return err; // je_lock destructor will unlock the journal entry
}

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
    fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
    fds_verify(qosReq != NULL);
    fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_LIST_BUCKET);
    LOGNOTIFY << "Responding to getBucket trans " << transId
              <<" for bucket " << blobReq->getBlobName()
              << " num of blobs " << blobListResp->num_blobs_in_resp
              << " end_of_list? " << blobListResp->end_of_list
              << " with result " << rxMsg->result;
    /*
     * Mark the IO complete, clean up txn, and callback
     */
    qos_ctrl->markIODone(txn->io);
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
        ListBucketReq* list_buck = static_cast<ListBucketReq*>(blobReq);
        list_buck->iter_cookie = blobListResp->iterator_cookie;

        /* call ListBucketReq's callback directly */
        list_buck->DoCallback( (blobListResp->end_of_list == true) ? 0 : 1, //isTrancated == 0 if no more blobs to return?
                               "", // next_marker ?
                               blobListResp->num_blobs_in_resp,
                               contents,
                               FDSN_StatusOK,
                               NULL);

    } else {
        /*
         * We received an error from SM
         */
        blobReq->cbWithResult(-1);
    }

    txn->reset();
    vol->journal_tbl->releaseTransId(transId);

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

fds::Error StorHvCtrl::getBucketStats(fds::AmQosReq *qosReq) {
    fds::Error err(ERR_OK);
    int om_err = 0;
  
    LOGNORMAL << "Doing a get bucket stats operation!";
  
    /*
     * Pull out the blob request
     */
    FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq->magicInUse() == true);

    fds_volid_t   volId = blobReq->getVolId();
    fds_verify(volId == admin_vol_id); /* must be in admin queue */
  
    StorHvVolume *shVol = vol_table->getVolume(volId);
    if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
        LOGCRITICAL << "getBucketStats failed to get admin volume";
        blobReq->cbWithResult(-1);
        err = ERR_NOT_FOUND;
        delete qosReq;
        return err;
    }

    /*
     * Track how long the request was queued before get() dispatch
     * TODO: Consider moving to the QoS request
     */
    blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
        boost::posix_time::microsec_clock::universal_time()));

    bool trans_in_progress = false;
    fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                     blobReq->getBlobOffset(),
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
        blobReq->cbWithResult(-2);
        err = ERR_NOT_IMPLEMENTED;
        delete qosReq;
        return err;
    }

    journEntry->setActive();

    LOGNORMAL <<" StorHvisorTx:" << "IO-XID:" << transId << " volID: 0x"
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
    journEntry->io = qosReq;
  
    /* send request to OM */
    om_err = om_client->pushGetBucketStatsToOM(transId);

    if(om_err != 0) {
        LOGERROR <<" StorHvisorTx:" << "IO-XID:" << transId << " volID: 0x"
                 << std::hex << admin_vol_id << std::dec
                 << " -  Couldn't send get bucket stats to OM. Completing request with ERROR(-1)";
        blobReq->cbWithResult(-1);
        err = ERR_NOT_FOUND;
        delete qosReq;
        return err;
    }

    LOGNORMAL << " StorHvisorTx:" << "IO-XID:"
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
    fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
    fds_verify(qosReq != NULL);
    fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getIoType() == FDS_BUCKET_STATS);
    LOGNOTIFY << "Responding to getBucketStats trans " << transId
              << " number of buckets " << (buck_stats->bucket_stats_list).size()
              << " with result " << rx_msg->result;

    /*
     * Mark the IO complete, clean up txn, and callback
     */
    qos_ctrl->markIODone(txn->io);
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
        BucketStatsReq *stats_req = static_cast<BucketStatsReq*>(blobReq);
        stats_req->DoCallback(buck_stats->timestamp, count, contents, FDSN_StatusOK, NULL);

    } else {    
        /*
         * We received an error from OM
         */
        blobReq->cbWithResult(-1);
    }
  
    txn->reset();
    vol->journal_tbl->releaseTransId(transId);
    /*
     * TODO: We're deleting the request structure. This assumes
     * that the caller got everything they needed when the callback
     * was invoked.
     */
    delete blobReq;
}

