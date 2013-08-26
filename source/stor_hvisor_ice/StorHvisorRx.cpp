#include <Ice/Ice.h>
#include "FDSP.h"
#include <ObjectFactory.h>
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

extern StorHvCtrl *storHvisor;
int vvc_entry_update(vvc_vhdl_t vhdl, const char *blk_name, int num_segments, const doid_t **doid_list);

int StorHvCtrl::fds_set_dmack_status( int ipAddr, int  trans_id)
{
	int node =0;
        StorHvJournalEntry *journEntry = journalTbl->get_journal_entry(trans_id);
	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&journEntry->dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (journEntry->dm_ack[node].ack_status < FDS_SET_ACK) {
	      journEntry->dm_ack_cnt++; 
	      journEntry->dm_ack[node].ack_status = FDS_SET_ACK;
	    }
	    return (0);
	  }
	}

	return 0;

}

int StorHvCtrl::fds_set_dm_commit_status( int ipAddr, int  trans_id)
{
	int node =0;

        StorHvJournalEntry *journEntry = journalTbl->get_journal_entry(trans_id);
	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&journEntry->dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (journEntry->dm_ack[node].commit_status == FDS_COMMIT_MSG_SENT) {
	      journEntry->dm_commit_cnt++; 
	      journEntry->dm_ack[node].commit_status = FDS_COMMIT_MSG_ACKED;
	    }
	    return (0);
	  }
	}

	return 0;

}

int StorHvCtrl::fds_set_smack_status( int ipAddr, int  trans_id)
{
	int node =0;

        StorHvJournalEntry *journEntry = journalTbl->get_journal_entry(trans_id);
	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
	  if (memcmp((void *)&journEntry->sm_ack[node].ipAddr, (void *)&ipAddr, 4) == 0) {
	    if (journEntry->sm_ack[node].ack_status != FDS_SET_ACK) {
	      journEntry->sm_ack[node].ack_status = FDS_SET_ACK;
	      journEntry->sm_ack_cnt++;
	    }
	    return (0);
	  }
	}

	return 0;
}


void StorHvCtrl::fbd_process_req_timeout(unsigned long arg)
{
  int trans_id;
  StorHvJournalEntry *txn;
  struct request *req;

  trans_id = (int)arg;
  txn = journalTbl->get_journal_entry(trans_id);
  if (txn->trans_state != FDS_TRANS_EMPTY) {
    txn->trans_state = FDS_TRANS_EMPTY;
    req = (struct request *)txn->write_ctx;
    if (req) { 
      txn->write_ctx = 0;
      printf(" Timing out, responding to  the block : %p \n ",req);
      // __blk_end_request_all(req, 0); 
    }
  }
  
}

void StorHvCtrl::fbd_complete_req(int trans_id, fbd_request_t *req, int status)
{

  StorHvJournalEntry *txn = journalTbl->get_journal_entry(trans_id);
  txn->comp_req(txn->comp_arg1, txn->comp_arg2, req, status);

}


int StorHvCtrl::fds_process_get_obj_resp(const FDSP_MsgHdrTypePtr& rd_msg, const FDSP_GetObjTypePtr& get_obj_rsp )
{
	fbd_request_t *req; 
	int trans_id;
//	int   data_size;
//	int64_t data_offset;
//	char  *data_buf;
	

	trans_id = rd_msg->req_cookie;
        StorHvJournalEntry *txn = journalTbl->get_journal_entry(trans_id);
	if (txn->trans_state != FDS_TRANS_GET_OBJ) {
          cout << "Error: Journal Entry" << rd_msg->req_cookie <<  "  GetObjectResp for a transaction not in GetObjResp" << std::endl;
	  return (0);
	}

	printf("Processing read response for trans %d\n", trans_id);
	req = (fbd_request_t *)txn->write_ctx;
   	txn->trans_state = FDS_TRANS_EMPTY;
	txn->write_ctx = 0;
	/*
	  - how to handle the  length miss-match ( requested  length and  recived legth from SM
	  - we will have to handle sending more data due to length difference
	*/

	/*
	 - respond to the block device- data ready 
	*/
	
	printf(" responding to  the block : %p \n ",req);
	if(req) {
	  // __blk_end_request_all(req, 0); 
	  fbd_complete_req(trans_id, req, 0);
	}

	return 0;
}


int StorHvCtrl::fds_process_put_obj_resp(const FDSP_MsgHdrTypePtr& rx_msg, const FDSP_PutObjTypePtr& put_obj_rsp )
{

	if (rx_msg->msg_code == FDSP_MSG_PUT_OBJ_RSP) {
	    fds_set_smack_status(rx_msg->src_ip_lo_addr, rx_msg->req_cookie);
        }
	return 0;
}

int StorHvCtrl::fds_process_update_catalog_resp(const FDSP_MsgHdrTypePtr& rx_msg, 
                                                const FDSP_UpdateCatalogTypePtr& cat_obj_rsp )
{
	FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;
	FDSP_MsgHdrTypePtr   sm_msg_ptr;
    ObjectID obj_id;
	int node=0;
	fbd_request_t *req; 
	int trans_id;
    FDS_RPC_EndPoint *endPoint = NULL;
    FDSP_MsgHdrTypePtr     wr_msg;

	trans_id = rx_msg->req_cookie; 
        StorHvJournalEntry *txn = journalTbl->get_journal_entry(trans_id);
// Check sanity here, if this transaction is valid and matches with the cookie we got from the message


	if (txn->trans_state == FDS_TRANS_EMPTY) {
	  return (0);
	}

	req = (fbd_request_t *)txn->write_ctx;
	wr_msg = txn->dm_msg;
	sm_msg_ptr = txn->sm_msg;

	if (cat_obj_rsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
		fds_set_dmack_status(rx_msg->src_ip_lo_addr, rx_msg->req_cookie);
		printf(" DM OpenTrans RSP ;");
	} else {
		fds_set_dm_commit_status(rx_msg->src_ip_lo_addr, rx_msg->req_cookie);
		printf(" DM CommitTrans RSP ; ");
	}


       switch(txn->trans_state)  {
	 case FDS_TRANS_OPEN :
         {
	  		if ((txn->sm_ack_cnt < FDS_MIN_ACK) || (txn->dm_ack_cnt < FDS_MIN_ACK)) 
          	{
	    		 break;
	  		}
	  		printf(" Rx: State Transition to OPENED : Received min ack from  DM and SM. \n\n");
	  		txn->trans_state = FDS_TRANS_OPENED;
	 	 }	 
         break;

         case  FDS_TRANS_OPENED :
         {
	    	if (txn->dm_commit_cnt >= FDS_MIN_ACK) 
			{
	        	printf(" Rx: State Transition to COMMITTED  : Received min commits from  DM \n\n ");
	    		txn->trans_state = FDS_TRANS_COMMITTED;
	    	}
         }
         break;

	 case FDS_TRANS_COMMITTED :
         {
	   		 if((txn->sm_ack_cnt == txn->num_sm_nodes) && (txn->dm_commit_cnt == txn->num_dm_nodes))
		 	 {
	   			printf(" Rx: State Transition to SYCNED  : Received all acks and commits from  DM and SM. Ending req %p \n\n ", req);
	    		txn->trans_state = FDS_TRANS_SYNCED;

	    	/*
	      	-  add the vvc entry
	      	-  If we are thinking of adding the cache , we may have to keep a copy on the cache 
	    	*/
	    

        	obj_id.SetId( cat_obj_rsp->data_obj_id.hash_high,cat_obj_rsp->data_obj_id.hash_low);
        	storHvisor->volCatalogCache->Update((fds_uint64_t)rx_msg->glob_volume_id, (fds_uint64_t)cat_obj_rsp->volume_offset, obj_id);


	    	// destroy the txn, reclaim the space and return from here	    
	    	txn->trans_state = FDS_TRANS_EMPTY;
	    	txn->write_ctx = 0;
	    	// del_timer(txn->p_ti);
	    	if (req) 
			{
	      		fbd_complete_req(trans_id, req, 0);
	    	}
             return(0);
	    	}
          }
          break;

          default : 
             break;
       }
       if (txn->trans_state > FDS_TRANS_OPEN) 
	   {
	/*
	  -  browse through the list of the DM nodes that sent the open txn response .
          -  send  DM - commit request 
	 */
		
	for (node = 0; node < txn->num_dm_nodes; node++)
	{
	   if (txn->dm_ack[node].ack_status) 
		{
	     if ((txn->dm_ack[node].commit_status) == FDS_CLS_ACK)
		 {
		   upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_COMMITED;
		   upd_obj_req->dm_transaction_id = 1;
           upd_obj_req->data_obj_id.hash_high = txn->data_obj_id.hash_high;
           upd_obj_req->data_obj_id.hash_low =  txn->data_obj_id.hash_low;

      
           storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(txn->dm_ack[node].ipAddr, FDSP_DATA_MGR, &endPoint);
		   txn->dm_ack[node].commit_status = FDS_COMMIT_MSG_SENT;
		  if (endPoint)
		 	endPoint->fdspDPAPI->UpdateCatalogObject(wr_msg, upd_obj_req);
	     }
	   }
	}
    }
    return 0;
}


void FDSP_DataPathRespCbackI::GetObjectResp(const FDSP_MsgHdrTypePtr& msghdr, const FDSP_GetObjTypePtr& get_obj, const Ice::Current&) {
  storHvisor->fds_process_get_obj_resp(msghdr, get_obj);
}

void FDSP_DataPathRespCbackI::PutObjectResp(const FDSP_MsgHdrTypePtr& msghdr, const FDSP_PutObjTypePtr& put_obj, const Ice::Current&) {
  storHvisor->fds_process_put_obj_resp(msghdr, put_obj);
}

void FDSP_DataPathRespCbackI::UpdateCatalogObjectResp(const FDSP_MsgHdrTypePtr& fdsp_msg, const FDSP_UpdateCatalogTypePtr& update_cat, const Ice::Current &) {
        storHvisor->fds_process_update_catalog_resp(fdsp_msg, update_cat);
}

void FDSP_DataPathRespCbackI::QueryCatalogObjectResp(
    const FDSP_MsgHdrTypePtr& fdsp_msg_hdr,
    const FDSP_QueryCatalogTypePtr& cat_obj_req,
    const Ice::Current &) {
    int num_nodes;
    int node_ids[64];
    int node_state = -1;
    uint32_t node_ip = 0;
    ObjectID obj_id;
    int doid_dlt_key;
    FDS_RPC_EndPoint *endPoint = NULL;
    uint64_t doid_high;
    int trans_id = fdsp_msg_hdr->req_cookie;
    fbd_request_t *req;

    
    std::cout << "GOT A QUERY RESPONSE!" << std::endl;

        StorHvJournalEntry *journEntry = storHvisor->journalTbl->get_journal_entry(trans_id);

        if (journEntry == NULL) {
          cout << "Journal Entry  " << trans_id <<  "QueryCatalogObjResp not found" << std::endl;
           return;
        }
   
        if (journEntry->op !=  FDS_IO_READ) { 
          cout << "Journal Entry  " << fdsp_msg_hdr->req_cookie <<  "  QueryCatalogObjResp for a non IO_READ transaction" << std::endl;
           req = (fbd_request_t *)journEntry->write_ctx;
           journEntry->trans_state = FDS_TRANS_EMPTY;
           journEntry->write_ctx = 0;
           if(req) {
             storHvisor->fbd_complete_req(trans_id, req, 0);
           }
          return;
        }

        if (journEntry->trans_state != FDS_TRANS_VCAT_QUERY_PENDING) {
            cout << "Journal Entry  " << fdsp_msg_hdr->req_cookie <<  "  QueryCatalogObjResp for a transaction node in Query Pending " << std::endl;
           req = (fbd_request_t *)journEntry->write_ctx;
           journEntry->trans_state = FDS_TRANS_EMPTY;
           journEntry->write_ctx = 0;
           if(req) {
             storHvisor->fbd_complete_req(trans_id, req, 0);
           }
           return;
        }

        doid_high = cat_obj_req->data_obj_id.hash_high;
        doid_dlt_key = doid_high >> 56;

         FDS_ProtocolInterface::FDSP_GetObjTypePtr get_obj_req = new FDSP_GetObjType;
         // Lookup the Primary SM node-id/ip-address to send the GetObject to
         storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
         if(num_nodes == 0) {
            cout << "DataPlace Error : no nodes in DLT :Jrnl Entry" << fdsp_msg_hdr->req_cookie <<  "QueryCatalogObjResp " << std::endl;
           req = (fbd_request_t *)journEntry->write_ctx;
           journEntry->trans_state = FDS_TRANS_EMPTY;
           journEntry->write_ctx = 0;
           if(req) {
             storHvisor->fbd_complete_req(trans_id, req, 0);
           }
           return;
         }
         storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[0], &node_ip, &node_state);
         //
         // *****CAVEAT: Modification reqd
         // ******  Need to find out which is the primary SM and send this out to that SM. ********
         storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip, FDSP_STOR_MGR, &endPoint);
         
        // RPC Call GetObject to StorMgr
        fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
        fdsp_msg_hdr->msg_id =  1;
        get_obj_req->data_obj_id.hash_high = cat_obj_req->data_obj_id.hash_high;
        get_obj_req->data_obj_id.hash_low = cat_obj_req->data_obj_id.hash_low;
        get_obj_req->data_obj_len = journEntry->data_obj_len;
        if (endPoint) {
            endPoint->fdspDPAPI->GetObject(fdsp_msg_hdr, get_obj_req);
            journEntry->trans_state = FDS_TRANS_GET_OBJ;
        }
   
        obj_id.SetId( cat_obj_req->data_obj_id.hash_high,cat_obj_req->data_obj_id.hash_low);
        storHvisor->volCatalogCache->Update((fds_uint64_t)fdsp_msg_hdr->glob_volume_id, (fds_uint64_t)cat_obj_req->volume_offset, obj_id);
}
