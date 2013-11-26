#include <Ice/Ice.h>
#include <fdsp/FDSP.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include "StorHvisorNet.h"
#include "list.h"
#include "StorHvisorCPP.h"
#include <arpa/inet.h>

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;
#define SRC_IP  0x0a010a65

extern StorHvCtrl *storHvisor;

int StorHvCtrl::fds_process_get_obj_resp(const FDSP_MsgHdrTypePtr& rd_msg, const FDSP_GetObjTypePtr& get_obj_rsp )
{
  // struct fbd_device *fbd;
	fbd_request_t *req; 
	int trans_id;
        fds_uint32_t vol_id;
        StorHvVolume* vol;
//	int   data_size;
//	int64_t data_offset;
//	char  *data_buf;
	
        vol_id = rd_msg->glob_volume_id;
	trans_id = rd_msg->req_cookie;
        vol = vol_table->getVolume(vol_id);
        StorHvVolumeLock vol_lock(vol);    
        if (!vol || !vol->isValidLocked()) {  
	   FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: GET_OBJ_RSP for an un-registered volume" ;
           return (0);
        }

        StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);
  	// fbd = (fbd_device *)txn->fbd_ptr;
	StorHvJournalEntryLock je_lock(txn);

	if (!txn->isActive()) {
	   FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: Journal Entry" << rd_msg->req_cookie <<  "  GET_OBJ_RS for an inactive transaction" ;
	   return (0);
	}

	// TODO: check if incarnation number matches

	if (txn->trans_state != FDS_TRANS_GET_OBJ) {
           FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: Journal Entry" << rd_msg->req_cookie <<  "  GET_OBJ_RSP for a transaction not in GetObjResp";
	   return (0);
	}

	FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - GET_OBJ_RSP Processing read response for trans  " <<  trans_id;
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
	
	FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - GET_OBJ_RSP  responding to  the block :  " << req;
	if(req) {
          qos_ctrl->markIODone(txn->io);
          if (rd_msg->result == FDSP_ERR_OK) { 
              memcpy(req->buf, get_obj_rsp->data_obj.c_str(), req->len);
	      txn->fbd_complete_req(req, 0);
          } else {
	      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error Reading the Data,   responding to  the block :  " << req;
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
  fds_uint32_t vol_id = rx_msg->glob_volume_id;
  StorHvVolume *vol =  vol_table->getVolume(vol_id);
  StorHvVolumeLock vol_lock(vol);
  if (!vol || !vol->isValidLocked()) {
     FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: PUT_OBJ_RSP for an un-registered volume";
    return (0);
  }

  StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);   
  StorHvJournalEntryLock je_lock(txn);
 

  // Check sanity here, if this transaction is valid and matches with the cookie we got from the message

  if (!(txn->isActive())) {
     FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: Journal Entry" << rx_msg->req_cookie <<  "  PUT_OBJ_RSP for an inactive transaction";
     return (0);
  }

	if (rx_msg->msg_code == FDSP_MSG_PUT_OBJ_RSP) {
	    fbd_request_t *req = (fbd_request_t*)txn->write_ctx;
	    txn->fds_set_smack_status(rx_msg->src_ip_lo_addr,
                                      rx_msg->src_port);
	    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:"
                                           << trans_id << " volID:" << vol_id
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
  fds_uint32_t vol_id;

  vol_id = rx_msg->glob_volume_id;
  
  trans_id = rx_msg->req_cookie; 
  StorHvVolume *vol = vol_table->getVolume(vol_id);
  StorHvVolumeLock vol_lock(vol);
  if (!vol || !vol->isValidLocked()) {
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: UPDATE_CAT_OBJ_RSP for an un-registered volume";
    return (0);
  }

  StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(trans_id);
  StorHvJournalEntryLock je_lock(txn);
  
  FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id
                                 << " volID:" << vol_id
                                 << " - Recvd DM UPDATE_CAT_OBJ_RSP RSP ";
  // Check sanity here, if this transaction is valid and matches with the cookie we got from the message
  
  if (!(txn->isActive()) || txn->trans_state == FDS_TRANS_EMPTY ) {
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error: Journal Entry" << rx_msg->req_cookie <<  "  UPDATE_CAT_OBJ_RSP for an inactive transaction";
    return (0);
  }
  
  if (cat_obj_rsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
    txn->fds_set_dmack_status(rx_msg->src_ip_lo_addr,
                              rx_msg->src_port);
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id
                                   << " volID:" << vol_id
                                   << " -  Recvd DM TXN_STATUS_OPEN RSP "
                                   << " ip " << rx_msg->src_ip_lo_addr
                                   << " port " << rx_msg->src_port;
  } else {
    txn->fds_set_dm_commit_status(rx_msg->src_ip_lo_addr,
                                  rx_msg->src_port);
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id
                                   << " volID:" << vol_id
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
  
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "State transition attemp for trans "
                                                   << transId  << " cur state "  << txn->trans_state
                                                   << " sm_ack: " << txn->sm_ack_cnt << " dm_ack: "
                                                   << txn->dm_ack_cnt << " dm_commits: "
                                                   << txn->dm_commit_cnt;    
  switch(txn->trans_state)  {
    case FDS_TRANS_OPEN:
      if ((txn->sm_ack_cnt < FDS_MIN_ACK) || (txn->dm_ack_cnt < FDS_MIN_ACK)) {
        break;
      }
      FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Move trans " << transId
                                                       << " to FDS_TRANS_OPENED:"
                                                       << " received min DM/SM acks";
      txn->trans_state = FDS_TRANS_OPENED;
      break;
     
    case FDS_TRANS_OPENED:
      if (txn->dm_commit_cnt >= FDS_MIN_ACK) {
        FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Move trans " << transId
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
        FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Move trans " << transId
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
            ObjectID(txn->data_obj_id.hash_high, txn->data_obj_id.hash_low));
        
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
    upd_obj_info.data_obj_id.hash_high = txn->data_obj_id.hash_high;
    upd_obj_info.data_obj_id.hash_low =  txn->data_obj_id.hash_low;
    upd_obj_info.size = 0; // TODO: fix this.

    FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;
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
    
    for (fds_uint32_t node = 0; node < txn->num_dm_nodes; node++) {
      if (txn->dm_ack[node].ack_status != 0) {
        if ((txn->dm_ack[node].commit_status) == FDS_CLS_ACK) {
	  dmMsg->dst_ip_lo_addr = txn->dm_ack[node].ipAddr;
          dmMsg->dst_port = txn->dm_ack[node].port;
          
          FDS_RPC_EndPoint *endPoint = NULL;
          storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(txn->dm_ack[node].ipAddr,
                                                     txn->dm_ack[node].port,
                                                     FDSP_DATA_MGR,
                                                     &endPoint);
          fds_verify(endPoint != NULL);
          endPoint->fdspDPAPI->begin_UpdateCatalogObject(dmMsg, upd_obj_req);
          txn->dm_ack[node].commit_status = FDS_COMMIT_MSG_SENT;
          FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "For trans " << transId
                                                           << " sent UpdCatObjCommit req to DM ip "
                                                           << txn->dm_ack[node].ipAddr
                                                           << " port " << txn->dm_ack[node].port;
        }  // if FDS_CLS_ACK
      }  // if ack_status
    } // for (node)
  } // If TRANS_OPEN

  return result;
}


void FDSP_DataPathRespCbackI::GetObjectResp(const FDSP_MsgHdrTypePtr& msghdr,
                                            const FDSP_GetObjTypePtr& get_obj,
                                            const Ice::Current&) {
   FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << msghdr->req_cookie << " - Received get obj response for txn  " <<  msghdr->req_cookie; 
   // storHvisor->fds_process_get_obj_resp(msghdr, get_obj);
   fds::Error err = storHvisor->getObjResp(msghdr, get_obj);
   fds_verify(err == ERR_OK);
}

void FDSP_DataPathRespCbackI::PutObjectResp(const FDSP_MsgHdrTypePtr& msghdr,
                                            const FDSP_PutObjTypePtr& put_obj,
                                            const Ice::Current&) {
  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::debug) << "Received putObjResp for txn "
                                                          << msghdr->req_cookie; 
   // storHvisor->fds_process_put_obj_resp(msghdr, put_obj);
   fds::Error err = storHvisor->putObjResp(msghdr, put_obj);
   fds_verify(err == ERR_OK);
}

void FDSP_DataPathRespCbackI::UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                                                      const FDSP_UpdateCatalogTypePtr& update_cat,
                                                      const Ice::Current &) {
  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::debug) << "Received updCatObjResp for txn "
                                                          <<  fdsp_msg->req_cookie; 
  // storHvisor->fds_process_update_catalog_resp(fdsp_msg, update_cat);
  fds::Error err = storHvisor->upCatResp(fdsp_msg, update_cat);
  fds_verify(err == ERR_OK);
}

void FDSP_DataPathRespCbackI::DeleteCatalogObjectResp(
    const FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    const FDSP_DeleteCatalogTypePtr& cat_obj_req,
    const Ice::Current &) {
  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::debug) << "Received deleteCatObjResp for txn "
                                                          <<  fdsp_msg_hdr->req_cookie; 
}

void FDSP_DataPathRespCbackI::DeleteObjectResp(
    const FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    const FDSP_DeleteObjTypePtr& cat_obj_req,
    const Ice::Current &) {
}

void FDSP_DataPathRespCbackI::QueryCatalogObjectResp(
    const FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    const FDSP_QueryCatalogTypePtr& cat_obj_req,
    const Ice::Current &) {
    int num_nodes=8;
    int node_ids[8];
    int node_state = -1;
    uint32_t node_ip = 0;
    fds_uint32_t node_port = 0;
    ObjectID obj_id;
    int doid_dlt_key;
    FDS_RPC_EndPoint *endPoint = NULL;
    uint64_t doid_high;
    int trans_id = fdsp_msg_hdr->req_cookie;
    fbd_request_t *req;
    fds_uint32_t vol_id = fdsp_msg_hdr->glob_volume_id;
    StorHvVolume *shvol = storHvisor->vol_table->getVolume(vol_id);
    StorHvVolumeLock vol_lock(shvol);    
    if (!shvol || !shvol->isValidLocked()) {
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " - Volume " << vol_id <<  " not registered";
      return;
    }

    StorHvJournalEntry *journEntry = shvol->journal_tbl->get_journal_entry(trans_id);
    
    if (journEntry == NULL) {
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " - Journal Entry  " << trans_id <<  "QueryCatalogObjResp not found";
      return;
    }
    
    StorHvJournalEntryLock je_lock(journEntry);
    if (!journEntry->isActive()) {
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Journal Entry is In-Active";
      return;
    }
    
    if (journEntry->op !=  FDS_IO_READ && journEntry->op != FDS_GET_BLOB && journEntry->op != FDS_DELETE_BLOB) { 
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Journal Entry  " << fdsp_msg_hdr->req_cookie <<  "  QueryCatalogObjResp for a non IO_READ transaction" ;
      req = (fbd_request_t *)journEntry->write_ctx;
      journEntry->trans_state = FDS_TRANS_EMPTY;
      journEntry->write_ctx = 0;
      if(req) {
        FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Responding to the block device with Error" ;
        journEntry->fbd_complete_req(req, -1);
      }
      journEntry->reset();
      shvol->journal_tbl->releaseTransId(trans_id);
      return;
    }
    
    if (journEntry->trans_state != FDS_TRANS_VCAT_QUERY_PENDING) {
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Journal Entry  " << fdsp_msg_hdr->req_cookie <<  " QueryCatalogObjResp for a transaction node in Query Pending " ;
      req = (fbd_request_t *)journEntry->write_ctx;
      journEntry->trans_state = FDS_TRANS_EMPTY;
      journEntry->write_ctx = 0;
      if(req) {
        FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Responding to the block device with Error" ;
        journEntry->fbd_complete_req(req, -1);
      }
      journEntry->reset();
      shvol->journal_tbl->releaseTransId(trans_id);
      return;
    }
    
    // If Data Mgr does not have an entry, simply return 0s.
    if (fdsp_msg_hdr->result != FDS_ProtocolInterface::FDSP_ERR_OK) {
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Journal Entry  " << fdsp_msg_hdr->req_cookie <<  ":  QueryCatalogObjResp returned error ";
      req = (fbd_request_t *)journEntry->write_ctx;
      storHvisor->qos_ctrl->markIODone(journEntry->io);
      journEntry->trans_state = FDS_TRANS_EMPTY;
      journEntry->write_ctx = 0;
      if(req) {
        memset(req->buf, 0, req->len);
        journEntry->fbd_complete_req(req, 0);
      }
      journEntry->reset();
      shvol->journal_tbl->releaseTransId(trans_id);
      return;
    }

    fds_verify(cat_obj_req->obj_list.size() > 0);
    FDS_ProtocolInterface::FDSP_BlobObjectInfo& cat_obj_info = cat_obj_req->obj_list[0];
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - GOT A QUERY RESPONSE! Object ID :- " 
				   << cat_obj_info.data_obj_id.hash_high << ":" << cat_obj_info.data_obj_id.hash_low ;
    AmQosReq *qosReq = (AmQosReq *)journEntry->io;
    FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
    

    
    doid_high = cat_obj_info.data_obj_id.hash_high;
    doid_dlt_key = doid_high >> 56;
    
    // Lookup the Primary SM node-id/ip-address to send the GetObject to
    storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
    if(num_nodes == 0) {
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - DataPlace Error : no nodes in DLT :Jrnl Entry" << fdsp_msg_hdr->req_cookie <<  "QueryCatalogObjResp ";
      req = (fbd_request_t *)journEntry->write_ctx;
      storHvisor->qos_ctrl->markIODone(journEntry->io);
      journEntry->trans_state = FDS_TRANS_EMPTY;
      journEntry->write_ctx = 0;
      if(req) {
        journEntry->fbd_complete_req(req, 0);
      }
      journEntry->reset();
      shvol->journal_tbl->releaseTransId(trans_id);
      return;
    }
    storHvisor->dataPlacementTbl->getNodeInfo(node_ids[0],
                                              &node_ip,
                                              &node_port,
                                              &node_state);
       //
       // *****CAVEAT: Modification reqd
       // ******  Need to find out which is the primary SM and send this out to that SM. ********
       storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                                  node_port,
                                                  FDSP_STOR_MGR,
                                                  &endPoint);
    
       // RPC Call GetObject to StorMgr
       fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
       fdsp_msg_hdr->msg_id =  1;
          // fdsp_msg_hdr->src_ip_lo_addr = SRC_IP;
       fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
       fdsp_msg_hdr->dst_port = node_port;
       fdsp_msg_hdr->src_node_name = storHvisor->my_node_name;
       if ( journEntry->io->io_type == FDS_IO_READ || journEntry->io->io_type == FDS_GET_BLOB) { 
         FDS_ProtocolInterface::FDSP_GetObjTypePtr get_obj_req = new FDSP_GetObjType;
         get_obj_req->data_obj_id.hash_high = cat_obj_info.data_obj_id.hash_high;
         get_obj_req->data_obj_id.hash_low = cat_obj_info.data_obj_id.hash_low;
         get_obj_req->data_obj_len = journEntry->data_obj_len;
         if (endPoint) {
           endPoint->fdspDPAPI->begin_GetObject(fdsp_msg_hdr, get_obj_req);
           FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Sent Async getObj req to SM at " << node_ip ;
           journEntry->trans_state = FDS_TRANS_GET_OBJ;
         }
         obj_id.SetId( cat_obj_info.data_obj_id.hash_high,cat_obj_info.data_obj_id.hash_low);
         /*
          * TODO: Don't just grab the hard coded first catalog object in the list.
          * Actually loop here.
          */
         FDS_PLOG(storHvisor->GetLog()) << "Doing a update catalog request after resp received";
         shvol->vol_catalog_cache->Update(cat_obj_req->blob_name,
                                     cat_obj_info.offset,
                                     obj_id);
       } else if (journEntry->io->io_type == FDS_DELETE_BLOB) { 
         FDS_ProtocolInterface::FDSP_DeleteObjTypePtr del_obj_req = new FDSP_DeleteObjType;
         del_obj_req->data_obj_id.hash_high = cat_obj_info.data_obj_id.hash_high;
         del_obj_req->data_obj_id.hash_low = cat_obj_info.data_obj_id.hash_low;
         del_obj_req->data_obj_len = journEntry->data_obj_len;
         if (endPoint) {
           endPoint->fdspDPAPI->begin_DeleteObject(fdsp_msg_hdr, del_obj_req);
           FDS_PLOG(storHvisor->GetLog()) << " StorHvisorRx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Sent Async deleteObj req to SM at " << node_ip ;
           journEntry->trans_state = FDS_TRANS_DEL_OBJ;
         }
         // RPC Call DeleteCatalogObject to DataMgr
         FDS_ProtocolInterface::FDSP_DeleteCatalogTypePtr del_cat_obj_req = new FDSP_DeleteCatalogType;
         num_nodes = 8;
         FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm = new FDSP_MsgHdrType;
         storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
         fdsp_msg_hdr_dm->msg_code = FDSP_MSG_DELETE_CAT_OBJ_REQ;
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
           storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                               node_port,
                                               FDSP_DATA_MGR,
                                               &endPoint);
           if (endPoint){
             endPoint->fdspDPAPI->begin_DeleteCatalogObject(fdsp_msg_hdr_dm, del_cat_obj_req);
             FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:"
                                     << trans_id << " volID:" << vol_id
                                     << " - Sent async DELETE_CAT_OBJ_REQ request to DM at "
                                     <<  node_ip << " port " << node_port;
           }
         }
       }

    FDS_PLOG(storHvisor->GetLog()) << "Done with a update catalog request after resp received";
}


void FDSP_DataPathRespCbackI::GetVolumeBlobListResp(
    const FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    const FDSP_GetVolumeBlobListRespTypePtr& blob_list_resp,
    const Ice::Current &) {
  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::debug) << "Received GetVolumeBlobListResp for txn "
                                                          <<  fdsp_msg_hdr->req_cookie; 
  fds::Error err = storHvisor->getBucketResp(fdsp_msg_hdr, blob_list_resp);
  fds_verify(err == ERR_OK);
}
