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


using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;

extern FDSP_NetworkCon  NETPtr;
int vvc_entry_update(vvc_vhdl_t vhdl, const char *blk_name, int num_segments, const doid_t **doid_list);

int FDSP_ProcRx::fds_set_dmack_status( int ipAddr, int  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[node].ack_status < FDS_SET_ACK) {
	      NETPtr.procIo.rwlog_tbl[trans_id].dm_ack_cnt++; 
	      NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[node].ack_status = FDS_SET_ACK;
	    }
	    return (0);
	  }
	}

	return 0;

}

int FDSP_ProcRx::fds_set_dm_commit_status( int ipAddr, int  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[node].commit_status == FDS_COMMIT_MSG_SENT) {
	      NETPtr.procIo.rwlog_tbl[trans_id].dm_commit_cnt++; 
	      NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[node].commit_status = FDS_COMMIT_MSG_ACKED;
	    }
	    return (0);
	  }
	}

	return 0;

}

int FDSP_ProcRx::fds_set_smack_status( int ipAddr, int  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&NETPtr.procIo.rwlog_tbl[trans_id].sm_ack[node].ipAddr, &ipAddr, 4) == 0) {
	    if (NETPtr.procIo.rwlog_tbl[trans_id].sm_ack[node].ack_status != FDS_SET_ACK) {
	      NETPtr.procIo.rwlog_tbl[trans_id].sm_ack[node].ack_status = FDS_SET_ACK;
	      NETPtr.procIo.rwlog_tbl[trans_id].sm_ack_cnt++;
	    }
	    return (0);
	  }
	}

	return 0;
}


void FDSP_ProcRx::fbd_process_req_timeout(unsigned long arg)
{
  int trans_id;
  FDSP_fdsJourn *txn;
  struct request *req;

  trans_id = (int)arg;
  txn = &NETPtr.procIo.rwlog_tbl[trans_id];
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

void FDSP_ProcRx::fbd_complete_req(int trans_id, td_request_t *req, int status)
{

  FDSP_fdsJourn  *txn;

  txn = &NETPtr.procIo.rwlog_tbl[trans_id];
  txn->comp_req(txn->comp_arg1, txn->comp_arg2, req, status);

}


int FDSP_ProcRx::fds_process_get_obj_resp(FDSP_MsgHdrType *rd_msg, FDSP_GetObjType *get_obj_rsp )
{
	td_request_t *req; 
	int trans_id;
//	int   data_size;
//	int64_t data_offset;
//	char  *data_buf;
	

	trans_id = rd_msg->req_cookie;
	if (NETPtr.procIo.rwlog_tbl[trans_id].trans_state == FDS_TRANS_EMPTY) {
	  return (0);
	}

	printf("Processing read response for trans %d\n", trans_id);
	req = (td_request_t *)NETPtr.procIo.rwlog_tbl[trans_id].write_ctx;
//	data_size    = req->secs * HVISOR_SECTOR_SIZE;
//	data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
//	data_buf = req->buf;
   	NETPtr.procIo.rwlog_tbl[trans_id].trans_state = FDS_TRANS_EMPTY;
	NETPtr.procIo.rwlog_tbl[trans_id].write_ctx = 0;
	// del_timer(NETPtr.procIo.rwlog_tbl[trans_id].p_ti);
	
//	printf(" length: %d   buf: %p \n ", data_size, data_buf);

//	memcpy((void *)data_buf, (void *)&(get_obj_rsp->data_obj[0]), data_size); 
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


int FDSP_ProcRx::fds_process_put_obj_resp(FDSP_MsgHdrTypePtr rx_msg, FDSP_PutObjTypePtr put_obj_rsp )
{

	if (rx_msg->msg_code == FDSP_MSG_PUT_OBJ_RSP) 
			fds_set_smack_status(rx_msg->src_ip_lo_addr, rx_msg->req_cookie);

	return 0;
}

int FDSP_ProcRx::fds_process_update_catalog_resp(FDSP_MsgHdrTypePtr rx_msg, FDSP_UpdateCatalogTypePtr cat_obj_rsp )
{
	FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;
	FDSP_MsgHdrTypePtr   sm_msg_ptr;
	int rc;
	int node=0;
	td_request_t *req; 
	struct fbd_device *fbd;

	int trans_id;
	fds_doid_t *doid_list[1];

    FDSP_MsgHdrTypePtr     wr_msg;
	FDSP_fdsJourn  *txn;

	trans_id = rx_msg->req_cookie; 
	txn = &NETPtr.procIo.rwlog_tbl[trans_id];
	// Check sanity here, if this transaction is valid and matches with the cookie we got from the message
	if (txn->trans_state == FDS_TRANS_EMPTY) {
	  return (0);
	}

	fbd = (struct fbd_device *)(txn->fbd_ptr);
	req = (td_request_t *)txn->write_ctx;
	wr_msg = txn->dm_msg;
	sm_msg_ptr = NETPtr.procIo.rwlog_tbl[trans_id].sm_msg;

	if (cat_obj_rsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
		fds_set_dmack_status(rx_msg->src_ip_lo_addr, rx_msg->req_cookie);
		printf(" DM OpenTrans RSP ;");
	} else {
		fds_set_dm_commit_status(rx_msg->src_ip_lo_addr, rx_msg->req_cookie);
		printf(" DM CommitTrans RSP ; ");
	}


			if (txn->trans_state == FDS_TRANS_OPEN) {
	  			if ((txn->sm_ack_cnt < FDS_MIN_ACK) || (txn->dm_ack_cnt < FDS_MIN_ACK)) {
	    			return (0);
	  			}
	  			printf(" **** State Transition to OPENED *** : Received min ack from  DM and SM. \n\n");
	  			txn->trans_state = FDS_TRANS_OPENED;
	
			} else if (txn->trans_state == FDS_TRANS_OPENED) {
	  			if (txn->dm_commit_cnt >= FDS_MIN_ACK) {
	    		printf(" **** State Transition to COMMITTED *** : Received min commits from  DM \n\n ");
	    		txn->trans_state = FDS_TRANS_COMMITTED;
	  			}
			} else if (txn->trans_state == FDS_TRANS_COMMITTED) {
	  			if ((txn->sm_ack_cnt == txn->num_sm_nodes) && (txn->dm_commit_cnt == txn->num_dm_nodes)) {
	    		printf(" **** State Transition to SYCNED *** : Received all acks and commits from  DM and SM. Ending req %p \n\n ", req);
	    		txn->trans_state = FDS_TRANS_SYNCED;
	    	/*
	      	-  add the vvc entry
	      	-  If we are thinking of adding the cache , we may have to keep a copy on the cache 
	    	*/
	    
	    		doid_list[0] = (fds_doid_t *)&(cat_obj_rsp->data_obj_id);
	    
	    		rc = vvc_entry_update(fbd->vhdl, "BlockName", 1, (const doid_t **)doid_list);

	    		if (rc)
	      		{
					printf("Error on creating vvc entry. Error code : %d\n", rc);
	      		}

	    		// destroy the txn, reclaim the space and return from here	    
	    		txn->trans_state = FDS_TRANS_EMPTY;
	    		txn->write_ctx = 0;
	    		// del_timer(txn->p_ti);
	    		if (req) {
	      			//__blk_end_request_all(req, 0);
	      			fbd_complete_req(trans_id, req, 0);
	    		}
	    		return (0);
	  		}
		}
	
		if (txn->trans_state > FDS_TRANS_OPEN) {

		/*
		   -  browse through the list of the DM nodes that sent the open txn response .
		   -  send  DM - commit request 
		 */
		
			for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
			{
		  		if (txn->dm_ack[node].ack_status) {
			  		if ((txn->dm_ack[node].commit_status) == FDS_CLS_ACK)
			    	{
			      		upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_COMMITED;
				  		upd_obj_req->dm_transaction_id = 1;

          		  		NETPtr.fdspDPAPI->UpdateCatalogObject(wr_msg, upd_obj_req);
				  		txn->dm_ack[node].commit_status = FDS_COMMIT_MSG_SENT;
			    	}
			  	}
		  	}
		}

	return 0;
}

