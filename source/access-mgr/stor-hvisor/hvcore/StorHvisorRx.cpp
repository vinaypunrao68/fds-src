#include "./StorHvisorNet.h"
#include "./list.h"
#include <arpa/inet.h>
#include <string>
#include <NetSession.h>

#include "requests/GetBlobReq.h"
#include "requests/PutBlobReq.h"

using namespace std;
using namespace FDS_ProtocolInterface;
#define SRC_IP  0x0a010a65

extern StorHvCtrl *storHvisor;

// Warning: Assumes that caller holds the lock to the transaction
// TODO(Andrew): Why don't we just pass the journal entry...?
int StorHvCtrl::fds_move_wr_req_state_machine(const FDSP_MsgHdrTypePtr& rxMsg) {

    fds_int32_t  result  = 0;
    fds_uint32_t transId = rxMsg->req_cookie; 
    fds_volid_t  volId   = rxMsg->glob_volume_id;

    StorHvVolume* vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);
    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);
  
    FDSP_MsgHdrTypePtr dmMsg = txn->dm_msg;
  
    LOGDEBUG << "Moving state machine txnid:" << transId
             << " state:"  << txn->trans_state
             << " sm_ack:" << (fds_uint32_t)txn->sm_ack_cnt
             << " dm_ack:" << (fds_uint32_t)txn->dm_ack_cnt
             << " dm_commits:" << (fds_uint32_t)txn->dm_commit_cnt
             << " num_dm_nodes:" << (fds_uint32_t)txn->num_dm_nodes
             << " num_sm_nodes:" << (fds_uint32_t)txn->num_sm_nodes;

    // TODO(Andrew): Handle start blob trans separate from
    // the normal put/get transactions
    if (txn->trans_state == FDS_TRANS_BLOB_START ||
        txn->trans_state == FDS_TRANS_MULTIDM) {
        if (txn->dm_ack_cnt == txn->num_dm_nodes) {
            // We return 1 to indicate all of the acks
            // have been received. This allows the caller
            // to handle what needs to be done instead
            // of this generic helper function.
            return 1;
        }
        return 0;
    }
            
    switch(txn->trans_state)  {
        case FDS_TRANS_OPEN:
            if ((txn->sm_ack_cnt < FDS_MIN_ACK) || (txn->dm_ack_cnt < FDS_MIN_ACK)) {
                break;
            }
            LOGDEBUG << "Move trans " << transId
                      << " to FDS_TRANS_OPENED:"
                      << " received min DM/SM acks";
            txn->trans_state = FDS_TRANS_OPENED;
            break;
     
        case FDS_TRANS_OPENED:
            if (txn->dm_commit_cnt >= FDS_MIN_ACK) {
                LOGDEBUG << "Move trans " << transId
                          << " to FDS_TRANS_COMMITTED:"
                          << " received min DM commits";
                txn->trans_state = FDS_TRANS_COMMITTED;
            }      
            if (txn->dm_commit_cnt < txn->num_dm_nodes)
                break;
            // else fall through to next case.
      
        case FDS_TRANS_COMMITTED:
            if((txn->sm_ack_cnt >= txn->num_sm_nodes) &&
               (txn->dm_commit_cnt == txn->num_dm_nodes)) {
                LOGDEBUG << "Move trans " << transId
                          << " to FDS_TRANS_SYNCED:"
                          << " received all DM/SM acks and commits.";
                txn->trans_state = FDS_TRANS_SYNCED;
        
                // Add the vvc entry
                // If we are thinking of adding the cache , we may have to keep a copy on the cache 
                fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
                fds_verify(qosReq != NULL);
                fds::PutBlobReq *blobReq = static_cast<fds::PutBlobReq *>(qosReq->getBlobReqPtr());
                fds_verify(blobReq != NULL);
                Error err = vol->vol_catalog_cache->Update(
                    blobReq->getBlobName(),
                    blobReq->blob_offset,
                    (fds_uint32_t)blobReq->data_len,
                    blobReq->obj_id,
                    blobReq->isLastBuf());
                fds_verify(err == ERR_OK);

                // Mark the IO complete
                qos_ctrl->markIODone(qosReq);

                // Resume any pending operations waiting for this txn
                // TODO(Andrew): This performs the op with the
                // response thread. Also, this performs the op before
                // cleaning up the journal entry and notifying the
                // app about the other I/O
                vol->journal_tbl->resumePendingTrans(txn->trans_id);

                // clean up the journal entry
                txn->trans_state = FDS_TRANS_EMPTY;
                txn->reset();
                vol->journal_tbl->releaseTransId(transId);

                // Notify the application the operation is done
                blobReq->cbWithResult(ERR_OK);
                
                delete blobReq;
                return result;
            }
            break;

        default:
            fds_panic("Unknown SH/AM Journal Trans state for trans %u, state %d",
                      transId, txn->trans_state);
    }
  
    /*
     * Handle processing for open transactions
     */
    if (txn->trans_state > FDS_TRANS_OPEN) {
        /*
         * Browse through the list of the DM nodes that sent the open txn response.
         * Send  DM - commit request 
         */
        FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
        upd_obj_info.offset = txn->blobOffset;  // May want to revert to 0 for blocks?
        upd_obj_info.data_obj_id.digest = txn->data_obj_id.digest;
        upd_obj_info.size = 0; // TODO: fix this.

        FDSP_UpdateCatalogTypePtr upd_obj_req(new FDSP_UpdateCatalogType);
        upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_COMMITED;
        upd_obj_req->dm_transaction_id = 1;
        upd_obj_req->obj_list.clear();
        upd_obj_req->obj_list.push_back(upd_obj_info);
        upd_obj_req->meta_list.clear();
        // TODO(Andrew): Actually set this...
        upd_obj_req->txDesc.txId = 0;

        /*
         * Set the blob name from the blob request
         */
        fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
        fds_verify(qosReq != NULL);
        fds::AmRequest *blobReq = qosReq->getBlobReqPtr();
        fds_verify(blobReq != NULL);
        upd_obj_req->blob_name = blobReq->getBlobName();
        upd_obj_req->dmt_version = txn->dmt_version;
 
    
        for (fds_int32_t node = 0; node < txn->num_dm_nodes; node++) {
            if (txn->dm_ack[node].ack_status != 0) {
                if ((txn->dm_ack[node].commit_status) == FDS_CLS_ACK) {
                    dmMsg->dst_ip_lo_addr = txn->dm_ack[node].ipAddr;
                    dmMsg->dst_port = txn->dm_ack[node].port;
          
                    netMetaDataPathClientSession *sessionCtx;
                    sessionCtx = storHvisor->rpcSessionTbl->\
                            getClientSession<netMetaDataPathClientSession>(txn->dm_ack[node].ipAddr,
                                                                           txn->dm_ack[node].port);
                    boost::shared_ptr<FDSP_MetaDataPathReqClient> client = sessionCtx->getClient();
                    dmMsg->session_uuid = sessionCtx->getSessionId();
                    client->UpdateCatalogObject(dmMsg, upd_obj_req);
                    txn->dm_ack[node].commit_status = FDS_COMMIT_MSG_SENT;
                    LOGDEBUG << "For trans " << transId
                              << " sent UpdCatObjCommit req to DM ip "
                              << txn->dm_ack[node].ipAddr
                              << " port " << txn->dm_ack[node].port;
                }  // if FDS_CLS_ACK
            }  // if ack_status
        } // for (node)
    } // If TRANS_OPEN

    return result;
}


void FDSP_DataPathRespCbackI::GetObjectResp(FDSP_MsgHdrTypePtr& msghdr,
                                            FDSP_GetObjTypePtr& get_obj) {
    LOGDEBUG << " StorHvisorRx:" << "IO-XID:" << msghdr->req_cookie << " - Received get obj response for txn  " <<  msghdr->req_cookie; 
    fds_panic("You shouldn't be here");
}

void FDSP_DataPathRespCbackI::PutObjectResp(FDSP_MsgHdrTypePtr& msghdr,
                                            FDSP_PutObjTypePtr& put_obj) {
    LOGDEBUG << "Received putObjResp for txn "
             << msghdr->req_cookie; 
    fds::Error err = storHvisor->putObjResp(msghdr, put_obj);
    fds_verify(err == ERR_OK);
}

void
FDSP_MetaDataPathRespCbackI::StartBlobTxResp(boost::shared_ptr<FDSP_MsgHdrType> &msgHdr) {
    LOGDEBUG << "Received StartBlobTx response for journal txn "
             << msgHdr->req_cookie;
    storHvisor->startBlobTxResp(msgHdr);
}

void FDSP_MetaDataPathRespCbackI::UpdateCatalogObjectResp(FDSP_MsgHdrTypePtr& fdsp_msg,
                                                          FDSP_UpdateCatalogTypePtr& update_cat) {
    LOGDEBUG << "Received updCatObjResp for txn "
             <<  fdsp_msg->req_cookie; 
    fds::Error err = storHvisor->upCatResp(fdsp_msg, update_cat);
    fds_verify(err == ERR_OK);
}

void
FDSP_MetaDataPathRespCbackI::StatBlobResp(boost::shared_ptr<FDSP_MsgHdrType> &msgHdr,
                                          boost::shared_ptr<FDS_ProtocolInterface::
                                          BlobDescriptor> &blobDesc) {
    LOGDEBUG << "Received StatBlob response for txn "
             << msgHdr->req_cookie;
    storHvisor->statBlobResp(msgHdr, blobDesc);
}

void FDSP_MetaDataPathRespCbackI::SetBlobMetaDataResp(boost::shared_ptr<FDSP_MsgHdrType>& header,
                                                      boost::shared_ptr<std::string>& blobName) {
    fds_panic("You shouldn't be here");
}

void FDSP_MetaDataPathRespCbackI::GetBlobMetaDataResp(boost::shared_ptr<FDSP_MsgHdrType>& header,
                                                      boost::shared_ptr<std::string>& blobName,
                                                      boost::shared_ptr<FDSP_MetaDataList>& metaDataList) {
}

int StorHvCtrl::fds_move_del_req_state_machine(const FDSP_MsgHdrTypePtr& rxMsg) {
    fds_int32_t  result  = 0;
    fds_uint32_t transId = rxMsg->req_cookie; 
    fds_volid_t  volId   = rxMsg->glob_volume_id;

    StorHvVolume* vol = vol_table->getVolume(volId);
    fds_verify(vol != NULL);
    StorHvVolumeLock vol_lock(vol);
    fds_verify(vol->isValidLocked() == true);

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
    fds_verify(txn != NULL);
    switch(txn->trans_state)  {
        case FDS_TRANS_DEL_OBJ:
 
            LOGDEBUG << "DM Ack: " << txn->dm_ack_cnt;
  
            if (txn->dm_ack_cnt < FDS_MIN_ACK) {
                break;
            }
            LOGDEBUG << "Move trans " << transId
                      << " to FDS_TRANS_OPENED:"
                      << " received min DM/SM acks";
    
            /* we have received min acks, fall through the next state */
            txn->trans_state = FDS_TRANS_COMMITTED;

        case FDS_TRANS_COMMITTED: 
            {
            LOGDEBUG << "Move trans " << transId
                      << " to FDS_TRANS_COMMITTED:"
                      << " received min DM acks";
            fds::AmQosReq *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
            fds_verify(qosReq != NULL);
            fds::AmRequest *blobReq = qosReq->getBlobReqPtr();
            fds_verify(blobReq != NULL);
            fds_verify(blobReq->getIoType() == FDS_DELETE_BLOB);
            LOGDEBUG << "Responding to deleteBlob trans " << transId
                      <<" for blob " << blobReq->getBlobName()
                      << " with result " << rxMsg->result;

            /*
             * Mark the IO complete, clean up txn, and callback
             */
            qos_ctrl->markIODone(txn->io);
            txn->reset();
            vol->journal_tbl->releaseTransId(transId);
            if (rxMsg->result == FDSP_ERR_OK) {
                LOGDEBUG << "Invoking the callback";
                blobReq->cbWithResult(ERR_OK);
            } else {
                /*
                 * We received an error from SM
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
            break;

            // unhandled cases
        case FDS_TRANS_EMPTY:
        case FDS_TRANS_OPEN:
        case FDS_TRANS_OPENED:
        case FDS_TRANS_SYNCED:
        case FDS_TRANS_DONE:
        case FDS_TRANS_VCAT_QUERY_PENDING:
        case FDS_TRANS_GET_OBJ:
        case FDS_TRANS_GET_BUCKET:
        case FDS_TRANS_BUCKET_STATS:
        case FDS_TRANS_PENDING_DLT:
        default:
            LOGWARN << "unexpected state " << txn->trans_state ;
            break;
    }
    return 0;
}

void FDSP_MetaDataPathRespCbackI::DeleteCatalogObjectResp(
    FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    FDSP_DeleteCatalogTypePtr& del_obj_req) {
    LOGDEBUG << "Received deleteCatObjResp for txn "
             <<  fdsp_msg_hdr->req_cookie; 

    fds::Error err = storHvisor->deleteCatResp(fdsp_msg_hdr, del_obj_req);
    fds_verify(err == ERR_OK);
}

void FDSP_DataPathRespCbackI::DeleteObjectResp(
    FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    FDSP_DeleteObjTypePtr& cat_obj_req) {
}

void FDSP_MetaDataPathRespCbackI::QueryCatalogObjectResp(
    FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    FDSP_QueryCatalogTypePtr& cat_obj_req) {
    Error err(ERR_OK);
    int num_nodes=MAX_DM_NODES;
    fds_uint64_t node_ids[MAX_DM_NODES];
    int node_state = -1;
    uint32_t node_ip = 0;
    fds_uint32_t node_port = 0;
    ObjectID obj_id;
    int doid_dlt_key;
    netSession *endPoint = NULL;
    int trans_id = fdsp_msg_hdr->req_cookie;
    fds_volid_t vol_id = fdsp_msg_hdr->glob_volume_id;

    LOGDEBUG << " StorHvisorRx: trans " << trans_id << ", volume 0x" << std::hex
              << vol_id << std::dec << " received query catalog response" ;

    // Get the volume specific to the request
    StorHvVolume *shvol = storHvisor->vol_table->getVolume(vol_id);
    StorHvVolumeLock vol_lock(shvol);    
    if (!shvol || !shvol->isValidLocked()) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " - Volume 0x"
                 << std::hex << vol_id << std::dec <<  " not registered";
        return;
    }

    // Get the request journal entry
    StorHvJournalEntry *journEntry = shvol->journal_tbl->get_journal_entry(trans_id);    
    if (journEntry == NULL) {
        LOGWARN << " StorHvisorRx:" << "IO-XID:" << trans_id << " - Journal Entry  " << trans_id <<  "QueryCatalogObjResp not found";
        return;
    }

    // Lock the journal entry and verify it's active
    StorHvJournalEntryLock je_lock(journEntry);
    fds_verify(journEntry->isActive());

    // Get the blob request from the journal
    fds::AmQosReq   *qosReq  = (AmQosReq *)journEntry->io;
    fds_verify(qosReq != NULL);
    fds::GetBlobReq *blobReq = static_cast<fds::GetBlobReq *>(qosReq->getBlobReqPtr());
    fds_verify(blobReq != NULL);
    fds_verify(blobReq->getBlobName() == cat_obj_req->blob_name);

    // Verify the operation type is valid for a query catalog rsp
    fds_verify((journEntry->op ==  FDS_IO_READ) ||
               (journEntry->op == FDS_GET_BLOB));

    // Verify the transaction is expecting a query response
    fds_verify(journEntry->trans_state == FDS_TRANS_VCAT_QUERY_PENDING);

    // Return err to callback if DM query failed
    if (fdsp_msg_hdr->result != FDS_ProtocolInterface::FDSP_ERR_OK) {

        storHvisor->qos_ctrl->markIODone(journEntry->io);
        journEntry->trans_state = FDS_TRANS_EMPTY;

        blobReq->data_len = 0;
        fds_int32_t result = -1;
        if (fdsp_msg_hdr->result == FDS_ProtocolInterface::FDSP_ERR_BLOB_NOT_FOUND) {
            // Set the error code accordingly if the blob wasn't found
            result = FDSN_StatusEntityDoesNotExist;
        } else {
            LOGERROR << "Query response for trans " << trans_id << " for volume 0x"
                     << std::hex << vol_id << std::dec
                     << " journal entry  " << journEntry->trans_id
                     << " returned error " << fdsp_msg_hdr->result;
        }
        journEntry->reset();
        shvol->journal_tbl->releaseTransId(trans_id);
        blobReq->cbWithResult(result);        
        delete blobReq;        
        return;
    }

    fds_bool_t offsetFound = false;

    // Insert the returned entries into the cache
    FDS_ProtocolInterface::FDSP_BlobObjectList blobOffList = cat_obj_req->obj_list;
    fds_verify(blobOffList.empty() == false);
    for (FDS_ProtocolInterface::FDSP_BlobObjectList::const_iterator it =
                 blobOffList.cbegin();
         it != blobOffList.cend();
         it++) {
        ObjectID offsetObjId(((*it).data_obj_id).digest);
        // TODO(Andrew): Need to pass in if this is the last
        // offset in the blob or not...
        err = shvol->vol_catalog_cache->Update(blobReq->getBlobName(),
                                               (fds_uint64_t)(*it).offset,
                                               (fds_uint32_t)(*it).size,
                                               offsetObjId);
        fds_verify(err == ERR_OK);

        // Check if we received the offset we queried about
        // TODO(Andrew): Today DM just gives us the entire
        // blob, so it doesn't tell us if the offset we queried
        // about was in the list or not
        if ((fds_uint64_t)(*it).offset == blobReq->blob_offset) {
            offsetFound = true;
        }
    }

    if (offsetFound == false) {
        // We queried for a specific blob offset and didn't get it
        // in the response
        LOGWARN << "Blob " << blobReq->getBlobName()
                << " query catalog did NOT return requested offset "
                << blobReq->blob_offset;
        
        storHvisor->qos_ctrl->markIODone(journEntry->io);
        journEntry->trans_state = FDS_TRANS_DONE;

        journEntry->reset();
        shvol->journal_tbl->releaseTransId(trans_id);
        blobReq->data_len = 0;        
        blobReq->cbWithResult(FDSN_StatusEntityDoesNotExist);
        
        return;
    }

    // Get the object ID from the cache
    ObjectID offsetObjId;
    err = shvol->vol_catalog_cache->Query(blobReq->getBlobName(),
                                          blobReq->blob_offset,
                                          journEntry->trans_id,
                                          &offsetObjId);
    fds_verify(err == ERR_OK);
    journEntry->data_obj_id.digest = std::string((const char *)offsetObjId.GetId(),
                                                 (size_t)offsetObjId.GetLen());

    bool fSizeZero = (offsetObjId == NullObjectID);
    // Dispatch the SM get request after the
    // DM response is received
    fds_verify(journEntry->getMsg != NULL);
    
    // Update the request in the journal
    journEntry->getMsg->data_obj_id.digest =
            std::string((const char *)offsetObjId.GetId(),
                        (size_t)offsetObjId.GetLen());
    journEntry->getMsg->data_obj_len = blobReq->data_len;
    journEntry->trans_state = FDS_TRANS_GET_OBJ;
    if (fSizeZero) {
        LOGWARN << "zero size object "
                << " name:" << blobReq->getBlobName()
                << " not sending get request to sm";
        
        storHvisor->qos_ctrl->markIODone(journEntry->io);
        journEntry->trans_state = FDS_TRANS_DONE;

        journEntry->reset();
        shvol->journal_tbl->releaseTransId(trans_id);
        blobReq->data_len = 0;
        blobReq->cbWithResult(FDSN_StatusEntityEmpty);
        
    } else {
        LOGDEBUG << "sending get request to sm"
                  << " name:" << blobReq->getBlobName()
                  << " size:" << blobReq->data_len;
        err = storHvisor->dispatchSmGetMsg(journEntry);
    }
    
    fds_verify(err == ERR_OK);

    // Schedule a timer here to track the responses and the original request
    shvol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
    LOGDEBUG << "Done with a update catalog response processing";
}


void FDSP_MetaDataPathRespCbackI::GetVolumeBlobListResp(
    FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    FDSP_GetVolumeBlobListRespTypePtr& blob_list_resp
                                                        ) {
    LOGDEBUG << "Received GetVolumeBlobListResp for txn "
             <<  fdsp_msg_hdr->req_cookie; 
    fds::Error err = storHvisor->getBucketResp(fdsp_msg_hdr, blob_list_resp);
    fds_verify(err == ERR_OK);
}

void FDSP_MetaDataPathRespCbackI::GetVolumeMetaDataResp(fpi::FDSP_MsgHdrTypePtr& header,
                                                        fpi::FDSP_VolumeMetaDataPtr& volumeMeta) {
    STORHANDLER(GetVolumeMetaDataHandler,
                fds::FDS_GET_VOLUME_METADATA)->handleResponse(header, volumeMeta);
}
