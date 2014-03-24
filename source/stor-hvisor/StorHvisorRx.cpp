#include "StorHvisorNet.h"
#include "list.h"
#include "StorHvisorCPP.h"
#include <arpa/inet.h>

using namespace std;
using namespace FDS_ProtocolInterface;
#define SRC_IP  0x0a010a65

extern StorHvCtrl *storHvisor;

int StorHvCtrl::fds_process_get_obj_resp(const FDSP_MsgHdrTypePtr& rd_msg, const FDSP_GetObjTypePtr& get_obj_rsp )
{
    // struct fbd_device *fbd;
    fbd_request_t *req; 
    int trans_id;
    StorHvVolume* vol;
    //	int   data_size;
    //	int64_t data_offset;
    //	char  *data_buf;
	
    fds_volid_t vol_id = rd_msg->glob_volume_id;
    trans_id = rd_msg->req_cookie;
    vol = vol_table->getVolume(vol_id);
    StorHvVolumeLock vol_lock(vol);    
    if (!vol || !vol->isValidLocked()) {  
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Error: GET_OBJ_RSP for an un-registered volume" ;
        return (0);
    }

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);
    // fbd = (fbd_device *)txn->fbd_ptr;
    StorHvJournalEntryLock je_lock(txn);

    if (!txn->isActive()) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Error: Journal Entry" << rd_msg->req_cookie
                 << "  GET_OBJ_RS for an inactive transaction" ;
        return (0);
    }

    // TODO: check if incarnation number matches

    if (txn->trans_state != FDS_TRANS_GET_OBJ) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id <<std::dec
                 << " - Error: Journal Entry" << rd_msg->req_cookie <<  "  GET_OBJ_RSP for a transaction not in GetObjResp";
        return (0);
    }

    LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" <<std::hex << vol_id <<std::dec
              << " - GET_OBJ_RSP Processing read response for trans  " <<  trans_id;
    req = (fbd_request_t *)txn->write_ctx;
    /*
      - how to handle the  length miss-match ( requested  length and  recived legth from SM
      - we will have to handle sending more data due to length difference
    */

    //boost::posix_time::ptime ts = boost::posix_time::microsec_clock::universal_time();
    //long lat = vol->journal_tbl->microsecSinceCtime(ts) - req->sh_queued_usec;

    /*
      - respond to the block device- data ready 
    */
	
    LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
              << " - GET_OBJ_RSP  responding to  the block :  " << req;
    if(req) {
        qos_ctrl->markIODone(txn->io);
        if (rd_msg->result == FDSP_ERR_OK) { 
            memcpy(req->buf, get_obj_rsp->data_obj.c_str(), req->len);
            txn->fbd_complete_req(req, 0);
        } else {
            LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" <<std::hex << vol_id << std::dec
                     << " - Error Reading the Data,   responding to  the block :  " << req;
            txn->fbd_complete_req(req, -1);
        }
    }
    txn->reset();
    vol->journal_tbl->release_trans_id(trans_id);
    return 0;

}


int StorHvCtrl::fds_process_put_obj_resp(const FDSP_MsgHdrTypePtr& rx_msg, const FDSP_PutObjTypePtr& put_obj_rsp )
{
    int trans_id = rx_msg->req_cookie; 
    fds_volid_t vol_id = rx_msg->glob_volume_id;
    StorHvVolume *vol =  vol_table->getVolume(vol_id);
    StorHvVolumeLock vol_lock(vol);
    if (!vol || !vol->isValidLocked()) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Error: PUT_OBJ_RSP for an un-registered volume";
        return (0);
    }

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);   
    StorHvJournalEntryLock je_lock(txn);
 

    // Check sanity here, if this transaction is valid and matches with the cookie we got from the message

    if (!(txn->isActive())) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Error: Journal Entry" << rx_msg->req_cookie <<  "  PUT_OBJ_RSP for an inactive transaction";
        return (0);
    }

    if (rx_msg->msg_code == FDSP_MSG_PUT_OBJ_RSP) {
        fbd_request_t *req = (fbd_request_t*)txn->write_ctx;
        txn->fds_set_smack_status(rx_msg->src_ip_lo_addr,
                                  rx_msg->src_port);
        LOGNORMAL << " StorHvisorRx:" << "IO-XID:"
                  << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                  << " - Recvd SM PUT_OBJ_RSP RSP "
                  << " ip " << rx_msg->src_ip_lo_addr
                  << " port " << rx_msg->src_port;

        fds_move_wr_req_state_machine(rx_msg);
    }

    return 0;
}

int StorHvCtrl::fds_process_update_catalog_resp(const FDSP_MsgHdrTypePtr& rx_msg, 
                                                const FDSP_UpdateCatalogTypePtr& cat_obj_rsp )
{
  
    int trans_id;
    fds_volid_t vol_id = rx_msg->glob_volume_id;
  
    trans_id = rx_msg->req_cookie; 
    StorHvVolume *vol = vol_table->getVolume(vol_id);
    StorHvVolumeLock vol_lock(vol);
    if (!vol || !vol->isValidLocked()) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Error: UPDATE_CAT_OBJ_RSP for an un-registered volume";
        return (0);
    }

    StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);
    StorHvJournalEntryLock je_lock(txn);
  
    LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id
              << " volID: 0x" << std::hex << vol_id << std::dec
              << " - Recvd DM UPDATE_CAT_OBJ_RSP RSP ";
    // Check sanity here, if this transaction is valid and matches with the cookie we got from the message
  
    if (!(txn->isActive()) || txn->trans_state == FDS_TRANS_EMPTY ) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Error: Journal Entry" << rx_msg->req_cookie <<  "  UPDATE_CAT_OBJ_RSP for an inactive transaction";
        return (0);
    }
  
    if (cat_obj_rsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
        txn->fds_set_dmack_status(rx_msg->src_ip_lo_addr,
                                  rx_msg->src_port);
        LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id
                  << " volID: 0x" << std::hex << vol_id << std::dec
                  << " -  Recvd DM TXN_STATUS_OPEN RSP "
                  << " ip " << rx_msg->src_ip_lo_addr
                  << " port " << rx_msg->src_port;
    } else {
        txn->fds_set_dm_commit_status(rx_msg->src_ip_lo_addr,
                                      rx_msg->src_port);
        LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id
                  << " volID: 0x" << std::hex << vol_id << std::dec
                  << " -  Recvd DM TXN_STATUS_COMMITED RSP "
                  << " ip " << rx_msg->src_ip_lo_addr
                  << " port " << rx_msg->src_port;
    }
  
    fds_move_wr_req_state_machine(rx_msg);
    return (0); 
}

// Warning: Assumes that caller holds the lock to the transaction
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
  
    LOGNORMAL << "State transition attemp for trans "
              << transId  << " cur state "  << txn->trans_state
              << " sm_ack: " << txn->sm_ack_cnt << " dm_ack: "
              << txn->dm_ack_cnt << " dm_commits: "
              << txn->dm_commit_cnt;    
    switch(txn->trans_state)  {
        case FDS_TRANS_OPEN:
            if ((txn->sm_ack_cnt < FDS_MIN_ACK) || (txn->dm_ack_cnt < FDS_MIN_ACK)) {
                break;
            }
            LOGNORMAL << "Move trans " << transId
                      << " to FDS_TRANS_OPENED:"
                      << " received min DM/SM acks";
            txn->trans_state = FDS_TRANS_OPENED;
            break;
     
        case FDS_TRANS_OPENED:
            if (txn->dm_commit_cnt >= FDS_MIN_ACK) {
                LOGNORMAL << "Move trans " << transId
                          << " to FDS_TRANS_COMMITTED:"
                          << " received min DM commits";
                txn->trans_state = FDS_TRANS_COMMITTED;
            }      
            if (txn->dm_commit_cnt < txn->num_dm_nodes)
                break;
            // else fall through to next case.
      
        case FDS_TRANS_COMMITTED:
            if((txn->sm_ack_cnt == txn->num_sm_nodes) &&
               (txn->dm_commit_cnt == txn->num_dm_nodes)) {
                LOGNORMAL << "Move trans " << transId
                          << " to FDS_TRANS_SYNCED:"
                          << " received all DM/SM acks and commits.";
                txn->trans_state = FDS_TRANS_SYNCED;
        
                /*
                 * Add the vvc entry
                 * If we are thinking of adding the cache , we may have to keep a copy on the cache 
                 */        
                fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
                fds_verify(qosReq != NULL);
                fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
                fds_verify(blobReq != NULL);
                vol->vol_catalog_cache->Update(
                    blobReq->getBlobName(),
                    blobReq->getBlobOffset(),
                    ObjectID(txn->data_obj_id.digest));
        
                /*
                 * Mark the IO complete, clean up txn, and callback
                 */
                qos_ctrl->markIODone(qosReq);
                txn->trans_state = FDS_TRANS_EMPTY;
                txn->write_ctx   = 0;
                // del_timer(txn->p_ti);
                blobReq->cbWithResult(0);

                txn->reset();
                vol->journal_tbl->releaseTransId(transId);

                /*
                 * TODO: We're deleting the request structure. This assumes
                 * that the caller got everything they needed when the callback
                 * was invoked.
                 */
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

        /*
         * Set the blob name from the blob request
         */
        fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
        fds_verify(qosReq != NULL);
        fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
        fds_verify(blobReq != NULL);
        upd_obj_req->blob_name = blobReq->getBlobName();
    
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
                    LOGNORMAL << "For trans " << transId
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
    LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << msghdr->req_cookie << " - Received get obj response for txn  " <<  msghdr->req_cookie; 
    // storHvisor->fds_process_get_obj_resp(msghdr, get_obj);
    fds::Error err = storHvisor->getObjResp(msghdr, get_obj);
    fds_verify(err == ERR_OK);
}

void FDSP_DataPathRespCbackI::PutObjectResp(FDSP_MsgHdrTypePtr& msghdr,
                                            FDSP_PutObjTypePtr& put_obj) {
    LOGDEBUG << "Received putObjResp for txn "
             << msghdr->req_cookie; 
    // storHvisor->fds_process_put_obj_resp(msghdr, put_obj);
    fds::Error err = storHvisor->putObjResp(msghdr, put_obj);
    fds_verify(err == ERR_OK);
}

void FDSP_MetaDataPathRespCbackI::UpdateCatalogObjectResp(FDSP_MsgHdrTypePtr& fdsp_msg,
                                                          FDSP_UpdateCatalogTypePtr& update_cat) {
    LOGDEBUG << "Received updCatObjResp for txn "
             <<  fdsp_msg->req_cookie; 
    fds::Error err = storHvisor->upCatResp(fdsp_msg, update_cat);
    fds_verify(err == ERR_OK);
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
 
            LOGNORMAL << "DM Ack: " << txn->dm_ack_cnt;
  
            if (txn->dm_ack_cnt < FDS_MIN_ACK) {
                break;
            }
            LOGNORMAL << "Move trans " << transId
                      << " to FDS_TRANS_OPENED:"
                      << " received min DM/SM acks";
    
            /* we have received min acks, fall through the next state */
            txn->trans_state = FDS_TRANS_COMMITTED;

        case FDS_TRANS_COMMITTED:
            LOGNORMAL << "Move trans " << transId
                      << " to FDS_TRANS_COMMITTED:"
                      << " received min DM acks";
            fds::AmQosReq *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
            fds_verify(qosReq != NULL);
            fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
            fds_verify(blobReq != NULL);
            fds_verify(blobReq->getIoType() == FDS_DELETE_BLOB);
            LOGNOTIFY << "Responding to deleteBlob trans " << transId
                      <<" for blob " << blobReq->getBlobName()
                      << " with result " << rxMsg->result;

            /*
             * Mark the IO complete, clean up txn, and callback
             */
            qos_ctrl->markIODone(txn->io);
            if (rxMsg->result == FDSP_ERR_OK) {
                LOGNOTIFY << "Invoking the callback";
                blobReq->cbWithResult(0);
            } else {
                /*
                 * We received an error from SM
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
    int num_nodes=8;
    fds_uint64_t node_ids[8];
    int node_state = -1;
    uint32_t node_ip = 0;
    fds_uint32_t node_port = 0;
    ObjectID obj_id;
    int doid_dlt_key;
    netSession *endPoint = NULL;
    int trans_id = fdsp_msg_hdr->req_cookie;
    //fbd_request_t *req;
    fds_volid_t vol_id = fdsp_msg_hdr->glob_volume_id;

    LOGNORMAL << " StorHvisorRx: " << "IO_XID: " << trans_id << " - Volume 0x" << std::hex
              << vol_id << std::dec << " Received query catalog response" ;

    StorHvVolume *shvol = storHvisor->vol_table->getVolume(vol_id);
    StorHvVolumeLock vol_lock(shvol);    
    if (!shvol || !shvol->isValidLocked()) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " - Volume 0x"
                 << std::hex << vol_id << std::dec <<  " not registered";
        return;
    }

    StorHvJournalEntry *journEntry = shvol->journal_tbl->get_journal_entry(trans_id);
    
    if (journEntry == NULL) {
        LOGWARN << " StorHvisorRx:" << "IO-XID:" << trans_id << " - Journal Entry  " << trans_id <<  "QueryCatalogObjResp not found";
        return;
    }
    
    StorHvJournalEntryLock je_lock(journEntry);
    if (!journEntry->isActive()) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x"
                 << std::hex << vol_id << std::dec << " - Journal Entry is In-Active";
        return;
    }

    fds::AmQosReq   *qosReq  = (AmQosReq *)journEntry->io;
    fds_verify(qosReq != NULL);
    fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    fds_verify(blobReq != NULL);
    
    if (journEntry->op !=  FDS_IO_READ && journEntry->op != FDS_GET_BLOB && journEntry->op != FDS_DELETE_BLOB) { 
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Journal Entry  " << fdsp_msg_hdr->req_cookie <<  "  QueryCatalogObjResp for a non IO_READ transaction" ;
        journEntry->trans_state = FDS_TRANS_EMPTY;
        journEntry->write_ctx = 0;
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Responding to AM request with Error" ;
        blobReq->cbWithResult(-1);
        journEntry->reset();
        delete blobReq;
        shvol->journal_tbl->releaseTransId(trans_id);
        return;
    }
    
    if (journEntry->trans_state != FDS_TRANS_VCAT_QUERY_PENDING) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Journal Entry  " << fdsp_msg_hdr->req_cookie
                 <<  " QueryCatalogObjResp for a transaction node not in Query Pending " ;
        journEntry->trans_state = FDS_TRANS_EMPTY;
        journEntry->write_ctx = 0;
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Responding to the block device with Error" ;
        blobReq->cbWithResult(-1);      
        journEntry->reset();
        delete blobReq;
        shvol->journal_tbl->releaseTransId(trans_id);
        return;
    }
    
    // If Data Mgr does not have an entry, simply return 0s.
    if (fdsp_msg_hdr->result != FDS_ProtocolInterface::FDSP_ERR_OK) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - Journal Entry  " << fdsp_msg_hdr->req_cookie <<  ":  QueryCatalogObjResp returned error ";
        storHvisor->qos_ctrl->markIODone(journEntry->io);
        journEntry->trans_state = FDS_TRANS_EMPTY;
        journEntry->write_ctx = 0;
        // memset(req->buf, 0, req->len);
#if 0
        // TODO: We need to do something like below to fill the return buff with zeroes.
        if (blobReq->getIoType() == FDS_GET_BLOB) {
            /* NOTE: we are currently supporting only getting the whole blob
             * so the requester does not know about the blob length, 
             * we get the blob length in response from SM;
             * will need to revisit when we also support (don't ignore) byteCount in native api */
            fds_verify(getObjRsp->data_obj_len <= blobReq->getDataLen());
            blobReq->setDataLen(getObjRsp->data_obj_len);
        }
        fds_verify(getObjRsp->data_obj_len == blobReq->getDataLen());
        blobReq->setDataBuf(getObjRsp->data_obj.c_str());
#endif      
        blobReq->cbWithResult(-1);  
        journEntry->reset();
        delete blobReq;
        shvol->journal_tbl->releaseTransId(trans_id);
        return;
    }

    fds_verify(cat_obj_req->obj_list.size() > 0);
    FDS_ProtocolInterface::FDSP_BlobObjectInfo& cat_obj_info = cat_obj_req->obj_list[0];
    LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID: 0x"
              << std::hex << vol_id << std::dec << " - GOT A QUERY RESPONSE! Object ID :- " 
              << cat_obj_info.data_obj_id.digest;
    //AmQosReq *qosReq = (AmQosReq *)journEntry->io;
    //FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    

    
    // Lookup the Primary SM node-id/ip-address to send the GetObject to
    boost::shared_ptr<DltTokenGroup> dltPtr;
    dltPtr = storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(obj_id);
    fds_verify(dltPtr != NULL);

    num_nodes = dltPtr->getLength();
    // storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
    if(num_nodes == 0) {
        LOGERROR << " StorHvisorRx:" << "IO-XID:" << trans_id 
                 << " volID: 0x" << std::hex << vol_id << std::dec
                 << " - DataPlace Error : no nodes in DLT :Jrnl Entry" << fdsp_msg_hdr->req_cookie <<  "QueryCatalogObjResp ";
        storHvisor->qos_ctrl->markIODone(journEntry->io);
        journEntry->trans_state = FDS_TRANS_EMPTY;
        journEntry->write_ctx = 0;
        blobReq->cbWithResult(0);  
        journEntry->reset();
        delete blobReq;
        shvol->journal_tbl->releaseTransId(trans_id);
        return;
    }

    storHvisor->dataPlacementTbl->getNodeInfo(dltPtr->get(0).uuid_get_val(),
                                              &node_ip,
                                              &node_port,
                                              &node_state);
    // *****CAVEAT: Modification reqd
    // ******  Need to find out which is the primary SM and send this out to that SM. ********
    netDataPathClientSession *sessionCtx =
            storHvisor->rpcSessionTbl->\
            getClientSession<netDataPathClientSession>(node_ip, node_port);
    fds_verify(sessionCtx != NULL);
    
    // RPC Call GetObject to StorMgr
    fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
    fdsp_msg_hdr->msg_id =  1;
    // fdsp_msg_hdr->src_ip_lo_addr = SRC_IP;
    fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
    fdsp_msg_hdr->dst_port = node_port;
    fdsp_msg_hdr->src_node_name = storHvisor->my_node_name;
    if ( journEntry->io->io_type == FDS_IO_READ || journEntry->io->io_type == FDS_GET_BLOB) {
        FDS_ProtocolInterface::FDSP_GetObjTypePtr get_obj_req(new FDSP_GetObjType);
        get_obj_req->data_obj_id.digest = cat_obj_info.data_obj_id.digest;
        //        get_obj_req->data_obj_id.hash_low = cat_obj_info.data_obj_id.hash_low;
        get_obj_req->data_obj_len = journEntry->data_obj_len;


        boost::shared_ptr<FDSP_DataPathReqClient> client = sessionCtx->getClient();
        fdsp_msg_hdr->session_uuid = sessionCtx->getSessionId();
        client->GetObject(fdsp_msg_hdr, get_obj_req);
        LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Sent Async getObj req to SM at " << node_ip ;
        journEntry->trans_state = FDS_TRANS_GET_OBJ;


        obj_id.SetId( (const char *)cat_obj_info.data_obj_id.digest.c_str(), cat_obj_info.data_obj_id.digest.length());
        FDS_PLOG(storHvisor->GetLog()) << " Query Cat StorHvisorRx:" << obj_id ;
        /*
         * TODO: Don't just grab the hard coded first catalog object in the list.
         * Actually loop here.
         */
        LOGNORMAL << "Doing a update catalog request after resp received";
        shvol->vol_catalog_cache->Update(cat_obj_req->blob_name,
                                         cat_obj_info.offset,
                                         obj_id);
    } else if (journEntry->io->io_type == FDS_DELETE_BLOB) {
        FDS_ProtocolInterface::FDSP_DeleteObjTypePtr del_obj_req(new FDSP_DeleteObjType);
        del_obj_req->data_obj_id.digest = cat_obj_info.data_obj_id.digest;
        del_obj_req->data_obj_len = journEntry->data_obj_len;
        if (endPoint) {
            boost::shared_ptr<FDSP_DataPathReqClient> client =
                    dynamic_cast<netDataPathClientSession *>(endPoint)->getClient();
            netDataPathClientSession *sessionCtx =  static_cast<netDataPathClientSession *>(endPoint);
            fdsp_msg_hdr->session_uuid = sessionCtx->getSessionId();
            client->DeleteObject(fdsp_msg_hdr, del_obj_req);
            LOGNORMAL << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << std::hex << vol_id << std::dec << " - Sent Async deleteObj req to SM at " << node_ip ;
            journEntry->trans_state = FDS_TRANS_DEL_OBJ;
        }
        // RPC Call DeleteCatalogObject to DataMgr
        FDS_ProtocolInterface::FDSP_DeleteCatalogTypePtr del_cat_obj_req(new FDSP_DeleteCatalogType);
        num_nodes = FDS_REPLICATION_FACTOR;
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm(new FDSP_MsgHdrType);
        storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
        fdsp_msg_hdr_dm->msg_code = FDSP_MSG_DELETE_BLOB_REQ;
        fdsp_msg_hdr_dm->req_cookie = trans_id;
        fdsp_msg_hdr_dm->src_ip_lo_addr = SRC_IP;
        fdsp_msg_hdr_dm->src_node_name = storHvisor->my_node_name;
        fdsp_msg_hdr_dm->src_port = 0;
        fdsp_msg_hdr_dm->dst_port = node_port;
        storHvisor->dataPlacementTbl->getDMTNodesForVolume(vol_id, node_ids, &num_nodes);

        for (int i = 0; i < num_nodes; i++) {
            node_ip = 0;
            node_port = 0;
            node_state = -1;
            storHvisor->dataPlacementTbl->getNodeInfo(node_ids[i],
                                                      &node_ip,
                                                      &node_port,
                                                      &node_state);

            fdsp_msg_hdr_dm->dst_ip_lo_addr = node_ip;
            fdsp_msg_hdr_dm->dst_port = node_port;
            del_cat_obj_req->blob_name = blobReq->getBlobName();

            // Call Update Catalog RPC call to DM
            netMetaDataPathClientSession *sessionCtx =
                    storHvisor->rpcSessionTbl->\
                    getClientSession<netMetaDataPathClientSession>(node_ip, node_port);
            fds_verify(sessionCtx != NULL);

            boost::shared_ptr<FDSP_MetaDataPathReqClient> client = sessionCtx->getClient();
            fdsp_msg_hdr_dm->session_uuid = sessionCtx->getSessionId();
            client->DeleteCatalogObject(fdsp_msg_hdr_dm, del_cat_obj_req);
            LOGNORMAL << " StorHvisorTx:" << "IO-XID:"
                      << trans_id << " volID:" << std::hex << vol_id << std::dec
                      << " - Sent async DELETE_BLOB_REQ request to DM at "
                      <<  node_ip << " port " << node_port;

        }
    }

    LOGNORMAL << "Done with a update catalog request after resp received";
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
