#include "list.h"
#include "StorHvisorNet.h"
#include "StorHvisorCPP.h"
#include "hvisor_lib.h"
#include "fds_qos.h"
#include "qos_ctrl.h"
#include <hash/MurmurHash3.h>
#include <arpa/inet.h>

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace fds;

extern StorHvCtrl *storHvisor;

#define SRC_IP  0x0a010a65
#define FDS_IO_LONG_TIME  60 // seconds

BEGIN_C_DECLS
/*----------------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
int StorHvisorProcIoRd(void *_io)
{
 FDS_IOType *io = (FDS_IOType *)_io;
 fbd_request_t *req = (fbd_request_t *)io->fbd_req;
 blkdev_complete_req_cb_t comp_req = io->comp_req; 
 void *arg1 = ((fbd_request *)io->fbd_req)->vbd; 
 void *arg2 = ((fbd_request *)io->fbd_req)->vReq;
  netSession *endPoint = NULL;
  unsigned int node_ip = 0;
  fds_uint32_t node_port = 0;
  unsigned int doid_dlt_key=0;
  int num_nodes = 8;
  int node_ids[8];
  int node_state = -1;
  fds::Error err(ERR_OK);
  ObjectID oid;
  fds_uint32_t vol_id;
  StorHvVolume *shvol;

  unsigned int      trans_id = 0;
  fds_uint64_t data_offset  = req->sec * HVISOR_SECTOR_SIZE;
  vol_id = req->volUUID;  /* TODO: Derive vol_id from somewhere better. NOT fbd! */

#ifdef FDS_TEST_SH_NOOP_DISPATCH 
  FDS_PLOG(storHvisor->GetLog()) << "StorHvisorTx: FDS_TEST_SH_NOOP_DISPATCH defined, returning IO as soon as its dequeued";
  storHvisor->qos_ctrl->markIODone(io);
  memset(req->buf, 0, req->len);
  (*req->cb_request)(arg1, arg2, req, 0);
  return 0;
#endif /* FDS_TEST_SH_NOOP_DISPATCH */


  shvol = storHvisor->vol_table->getVolume(vol_id);
  if (!shvol || !shvol->isValidLocked()) {
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << " volID:" << vol_id << "- volume not registered, completing request "
				   << io->io_req_id  << " with ERROR(-1)";
    (*req->cb_request)(arg1, arg2, req, -1);  
    return -1;
  }

  /* Check if there is an outstanding transaction for this block offset  */
  trans_id = shvol->journal_tbl->get_trans_id_for_block(data_offset);
  StorHvJournalEntry *journEntry = shvol->journal_tbl->get_journal_entry(trans_id);
  
  StorHvJournalEntryLock je_lock(journEntry);
  
  if (journEntry->isActive()) {
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " - Transaction  is already in ACTIVE state, completing request "
				   << io->io_req_id << " with ERROR(-2) ";
    // There is an ongoing transaciton for this offset.
    // We should queue this up for later processing once that completes.
    
    // For now, return an error.
    (*req->cb_request)(arg1, arg2, req, -2);
    return (-1); // je_lock destructor will unlock the journal entry
  }

  /* record time request is queued -- temp before we add actual queues */
  req->sh_queued_usec = shvol->journal_tbl->microsecSinceCtime(boost::posix_time::microsec_clock::universal_time());
  
  journEntry->setActive();

  FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Activated txn for req :" << io->io_req_id;
  
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr(new FDSP_MsgHdrType);
  FDS_ProtocolInterface::FDSP_GetObjTypePtr get_obj_req(new FDSP_GetObjType);
  
  storHvisor->InitSmMsgHdr(fdsp_msg_hdr);
  fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
  fdsp_msg_hdr->msg_id =  1;
  
  
  journEntry->trans_state = FDS_TRANS_OPEN;
  journEntry->write_ctx = (void *)req;
  journEntry->comp_req = comp_req;
  journEntry->comp_arg1 = arg1; // vbd
  journEntry->comp_arg2 = arg2; //vreq
  journEntry->sm_msg = fdsp_msg_hdr; 
  journEntry->dm_msg = NULL;
  journEntry->sm_ack_cnt = 0;
  journEntry->dm_ack_cnt = 0;
  journEntry->op = FDS_IO_READ;
  journEntry->data_obj_id.hash_high = 0;
  journEntry->data_obj_id.hash_low = 0;
  journEntry->data_obj_len = req->len;
  journEntry->io = io;
  
  fdsp_msg_hdr->req_cookie = trans_id;
  
  err  = shvol->vol_catalog_cache->Query(std::to_string(data_offset), data_offset, trans_id, &oid);
  if (err.GetErrno() == ERR_PENDING_RESP) {
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Vol catalog Cache Query pending :" << err.GetErrno() << req;
    journEntry->trans_state = FDS_TRANS_VCAT_QUERY_PENDING;
    return 0;
  }
  
  if (err.GetErrno() == ERR_CAT_QUERY_FAILED)
  {
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Error reading the Vol catalog  Error code : " <<  err.GetErrno() << req;
    (*req->cb_request)(arg1, arg2, req, err.GetErrno());
    return err.GetErrno();
  }
  
  FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - object ID: " << oid.GetHigh() <<  ":" << oid.GetLow()									 << "  ObjLen:" << journEntry->data_obj_len;
  
  // We have a Cache HIT *$###
  //
  uint64_t doid_dlt = oid.GetHigh();
  doid_dlt_key = (doid_dlt >> 56);
  
  fdsp_msg_hdr->glob_volume_id = vol_id;;
  fdsp_msg_hdr->req_cookie = trans_id;
  fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
  fdsp_msg_hdr->msg_id =  1;
  fdsp_msg_hdr->src_ip_lo_addr = SRC_IP;
  fdsp_msg_hdr->src_port = 0;
  fdsp_msg_hdr->src_node_name = storHvisor->my_node_name;
  get_obj_req->data_obj_id.hash_high = oid.GetHigh();
  get_obj_req->data_obj_id.hash_low = oid.GetLow();
  get_obj_req->data_obj_len = req->len;
  
  journEntry->op = FDS_IO_READ;
  journEntry->data_obj_id.hash_high = oid.GetHigh();;
  journEntry->data_obj_id.hash_low = oid.GetLow();;
  
  // Lookup the Primary SM node-id/ip-address to send the GetObject to
  boost::shared_ptr<DltTokenGroup> dltPtr;
  dltPtr = storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(&oid);
  fds_verify(dltPtr != NULL);

  num_nodes = dltPtr->getLength();
//  storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
  if(num_nodes == 0) {
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " -  DLT Nodes  NOT  confiigured. Check on OM Manager. Completing request with ERROR(-1)";
    (*req->cb_request)(arg1, arg2, req, -1);
    return -1;
  }
  storHvisor->dataPlacementTbl->getNodeInfo(dltPtr->get(0).uuid_get_val(),
                                            &node_ip,
                                            &node_port,
                                            &node_state);
  
  fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
  fdsp_msg_hdr->dst_port       = node_port;
  
  // *****CAVEAT: Modification reqd
  // ******  Need to find out which is the primary SM and send this out to that SM. ********
  endPoint = storHvisor->rpcSessionTbl->getSession
             (node_ip, FDS_ProtocolInterface::FDSP_STOR_MGR);
  
  // RPC Call GetObject to StorMgr
  journEntry->trans_state = FDS_TRANS_GET_OBJ;
  if (endPoint)
  { 
    boost::shared_ptr<FDSP_DataPathReqClient> client =
            dynamic_cast<netDataPathClientSession *>(endPoint)->getClient();
    netDataPathClientSession *sessionCtx =  static_cast<netDataPathClientSession *>(endPoint);
    fdsp_msg_hdr->session_uuid = sessionCtx->getSessionId();
    client->GetObject(fdsp_msg_hdr, get_obj_req);
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Sent async GetObj req to SM";
  }
  
  // Schedule a timer here to track the responses and the original request
  shvol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
  return 0; // je_lock destructor will unlock the journal entry
}

/*----------------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
int StorHvisorProcIoWr(void *_io)
{
  FDS_IOType *io = (FDS_IOType *)_io;
  fbd_request_t *req = (fbd_request_t *)io->fbd_req;
  blkdev_complete_req_cb_t comp_req = io->comp_req; 
 void *arg1 = ((fbd_request *)io->fbd_req)->vbd; 
 void *arg2 = ((fbd_request *)io->fbd_req)->vReq;
  int   trans_id, i=0;
  // int   data_size    = req->secs * HVISOR_SECTOR_SIZE;
  int   data_size    = req->len;
  double data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
  char *tmpbuf = (char*)req->buf;
  ObjectID  objID;
  unsigned char doid_dlt_key;
  int num_nodes=8;
  netSession *endPoint = NULL;
  int node_ids[8];
  fds_uint32_t node_ip = 0;
  fds_uint32_t node_port = 0;
  int node_state = -1;
  fds_uint32_t vol_id;
  StorHvVolume *shvol;

#ifdef FDS_TEST_SH_NOOP_DISPATCH 
  FDS_PLOG(storHvisor->GetLog()) << "StorHvisorTx: FDS_TEST_SH_NOOP_DISPATCH defined, returning IO as soon as its dequeued";
  storHvisor->qos_ctrl->markIODone(io);
  (*req->cb_request)(arg1, arg2, req, 0);
  return 0;
#endif /* FDS_TEST_SH_NOOP_DISPATCH */
  
  /*
   * TODO: Currently don't derive the vol ID from the block
   * device. It's not safe since we may not always have
   * a block device. Let's take it from the request 
   * and clean it up when we introduce multi-volume.
   */
  vol_id = req->volUUID;
  shvol = storHvisor->vol_table->getLockedVolume(vol_id);
  if (!shvol || !shvol->isValidLocked()) {
    shvol->readUnlock();

    FDS_PLOG(storHvisor->GetLog()) << "StorHvisorTx:" << " volID:" << vol_id << " - volume not registered; completing request with ERROR(-1)";
    (*req->cb_request)(arg1, arg2, req, -1);
    return -1;
  }
  
  //  *** Get a new Journal Entry in xaction-log journalTbl
  trans_id = shvol->journal_tbl->get_trans_id_for_block(data_offset);
  StorHvJournalEntry *journEntry = shvol->journal_tbl->get_journal_entry(trans_id);
  FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - TXN_STATUS_OPEN ";
  
  StorHvJournalEntryLock je_lock(journEntry);
  
  
  if (journEntry->isActive()) {
    // There is an on-going transaction for this offset
    // Queue this up for later processing.

    // For now, return an error
    shvol->readUnlock();
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Transaction  is already in ACTIVE state, completing request with ERROR(-2) ";
    (*req->cb_request)(arg1, arg2, req, -2); 
    return (-1);
  }

  req->sh_queued_usec = shvol->journal_tbl->microsecSinceCtime(boost::posix_time::microsec_clock::universal_time());
  
  journEntry->setActive();
  
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr(new FDSP_MsgHdrType);
  FDS_ProtocolInterface::FDSP_PutObjTypePtr put_obj_req(new FDSP_PutObjType);
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm(new FDSP_MsgHdrType);
  FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr upd_obj_req(new FDSP_UpdateCatalogType);
  
  // Obtain MurmurHash on the data object
  MurmurHash3_x64_128(tmpbuf, data_size, 0, &objID );
  
  put_obj_req->data_obj = std::string((const char *)tmpbuf, (size_t )data_size);
  put_obj_req->data_obj_len = data_size;

  upd_obj_req->obj_list.clear();

  FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
  upd_obj_info.offset = 0;
  upd_obj_info.size = data_size;
  
  put_obj_req->data_obj_id.hash_high = upd_obj_info.data_obj_id.hash_high = objID.GetHigh();
  put_obj_req->data_obj_id.hash_low = upd_obj_info.data_obj_id.hash_low = objID.GetLow();
  
  upd_obj_req->obj_list.push_back(upd_obj_info);
  upd_obj_req->meta_list.clear();
  upd_obj_req->blob_size = data_size;


  fdsp_msg_hdr->glob_volume_id    = vol_id;
  fdsp_msg_hdr_dm->glob_volume_id = vol_id;
  
  doid_dlt_key = objID.GetHigh() >> 56;
  
  FDS_PLOG(storHvisor->GetLog())<< " StorHvisorTx:" << "IO-XID:" << trans_id << " volID:" << vol_id << " - Object ID:"<< objID.GetHigh()<< ":" << objID.GetLow() \
                                 << "  ObjLen:" << put_obj_req->data_obj_len << "  volID:" << fdsp_msg_hdr->glob_volume_id;
  
  // *** Initialize the journEntry with a open txn

  journEntry->trans_state = FDS_TRANS_OPEN;
  journEntry->write_ctx = (void *)req;
  journEntry->comp_req = comp_req;
  journEntry->comp_arg1 = arg1;
  journEntry->comp_arg2 = arg2;
  journEntry->io= io;
  journEntry->sm_msg = fdsp_msg_hdr;
  journEntry->dm_msg = fdsp_msg_hdr_dm;
  journEntry->sm_ack_cnt = 0;
  journEntry->dm_ack_cnt = 0;
  journEntry->dm_commit_cnt = 0;
  journEntry->op = FDS_IO_WRITE;
  journEntry->data_obj_id.hash_high = objID.GetHigh();
  journEntry->data_obj_id.hash_low = objID.GetLow();
  journEntry->data_obj_len= data_size;;
  
  fdsp_msg_hdr->src_ip_lo_addr = SRC_IP;
  fdsp_msg_hdr->src_node_name = storHvisor->my_node_name;
  fdsp_msg_hdr->req_cookie = trans_id;
  storHvisor->InitSmMsgHdr(fdsp_msg_hdr);
  
  
  // DLT lookup from the dataplacement object
//  num_nodes = 8;
  boost::shared_ptr<DltTokenGroup> dltPtr;
  dltPtr = storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(&objID);
  fds_verify(dltPtr != NULL);

  fds_int32_t numNodes = dltPtr->getLength();
//  storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
  for (i = 0; i < numNodes; i++) {
    node_ip = 0;
    node_port = 0;
    node_state = -1;
    // storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[i], &node_ip, &node_state);
    storHvisor->dataPlacementTbl->getNodeInfo(node_ids[i],
                                              &node_ip,
                                              &node_port,
                                              &node_state);
    journEntry->sm_ack[i].ipAddr = node_ip;
    journEntry->sm_ack[i].port = node_port;
    fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
    fdsp_msg_hdr->dst_port = node_port;
    journEntry->sm_ack[i].ack_status = FDS_CLS_ACK;
    journEntry->num_sm_nodes = numNodes;

    // Call Put object RPC to SM
    endPoint = storHvisor->rpcSessionTbl->getSession
             (node_ip, FDS_ProtocolInterface::FDSP_STOR_MGR);
    if (endPoint) { 
        boost::shared_ptr<FDSP_DataPathReqClient> client =
                dynamic_cast<netDataPathClientSession *>(endPoint)->getClient();
      netDataPathClientSession *sessionCtx =  static_cast<netDataPathClientSession *>(endPoint);
      fdsp_msg_hdr->session_uuid = sessionCtx->getSessionId();
      client->PutObject(fdsp_msg_hdr, put_obj_req);
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:"
                                     << trans_id << " volID:" << vol_id
                                     << " -  Sent async PUT_OBJ_REQ request to SM at "
                                     << node_ip << " port " << node_port;
    }
  }
  
  // DMT lookup from the data placement object
  num_nodes = 8;
  storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
  upd_obj_req->blob_name = std::to_string(data_offset);
  upd_obj_req->dm_transaction_id = 1;
  upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_OPEN;
  fdsp_msg_hdr_dm->req_cookie = trans_id;
  fdsp_msg_hdr_dm->src_ip_lo_addr = SRC_IP;
  fdsp_msg_hdr_dm->src_node_name = storHvisor->my_node_name;
  fdsp_msg_hdr_dm->src_port = 0;
  fdsp_msg_hdr_dm->dst_port = node_port;
  
  storHvisor->dataPlacementTbl->getDMTNodesForVolume(vol_id, node_ids, &num_nodes);
  
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
    
    // Call Update Catalog RPC call to DM
    endPoint = storHvisor->rpcSessionTbl->getSession
             (node_ip, FDS_ProtocolInterface::FDSP_DATA_MGR);
    if (endPoint){
        boost::shared_ptr<FDSP_MetaDataPathReqClient> client =
               dynamic_cast<netMetaDataPathClientSession *>(endPoint)->getClient();
      netMetaDataPathClientSession *sessionCtx =  static_cast<netMetaDataPathClientSession *>(endPoint);
      fdsp_msg_hdr_dm->session_uuid = sessionCtx->getSessionId();
      client->UpdateCatalogObject(fdsp_msg_hdr_dm, upd_obj_req);
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:"
                                     << trans_id << " volID:" << vol_id
                                     << " - Sent async UPDATE_CAT_OBJ_REQ request to DM at "
                                     <<  node_ip << " port " << node_port;
    }
  }
  
  // Schedule a timer here to track the responses and the original request
  shvol->journal_tbl->schedule(journEntry->ioTimerTask, std::chrono::seconds(FDS_IO_LONG_TIME));
    shvol->readUnlock(); 
  return 0;
}

END_C_DECLS


void StorHvCtrl::InitSmMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
	msg_hdr->minor_ver = 0;
	msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
        msg_hdr->msg_id =  1;

        msg_hdr->major_ver = 0xa5;
        msg_hdr->minor_ver = 0x5a;

        msg_hdr->num_objects = 1;
        msg_hdr->frag_len = 0;
        msg_hdr->frag_num = 0;

        msg_hdr->tennant_id = 0;
        msg_hdr->local_domain_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_STOR_MGR;

	msg_hdr->src_node_name = this->my_node_name;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


}

void StorHvCtrl::InitDmMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
 	msg_hdr->msg_code = FDSP_MSG_UPDATE_CAT_OBJ_REQ;
        msg_hdr->msg_id =  1;

        msg_hdr->major_ver = 0xa5;
        msg_hdr->minor_ver = 0x5a;

        msg_hdr->num_objects = 1;
        msg_hdr->frag_len = 0;
        msg_hdr->frag_num = 0;

        msg_hdr->tennant_id = 0;
        msg_hdr->local_domain_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_DATA_MGR;

	msg_hdr->src_node_name = this->my_node_name;

	msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;

}
