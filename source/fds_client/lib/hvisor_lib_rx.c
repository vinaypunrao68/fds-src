#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "list.h"
#include <pthread.h>
#include <linux/errno.h>

#include "tapdisk.h"
#include "hvisor_lib.h"

#include "vvclib.h"
#include "fds_commons.h"
#include "fdsp.h"
#include "data_mgr.h"
#include "fbd.h"
#include "fds.h"
#include "fbd_hash.h"


FDS_JOURN	rwlog_tbl[FDS_READ_WRITE_LOG_ENTRIES];

static  int fds_set_dmack_status( uint32_t ipAddr, uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (rwlog_tbl[trans_id].dm_ack[node].ack_status < FDS_SET_ACK) {
	      rwlog_tbl[trans_id].dm_ack_cnt++; 
	      rwlog_tbl[trans_id].dm_ack[node].ack_status = FDS_SET_ACK;
	    }
	    return (0);
	  }
	}

	return 0;

}

static  int fds_set_dm_commit_status( uint32_t ipAddr, uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr, &ipAddr,4) == 0) {
	    if (rwlog_tbl[trans_id].dm_ack[node].commit_status == FDS_COMMIT_MSG_SENT) {
	      rwlog_tbl[trans_id].dm_commit_cnt++; 
	      rwlog_tbl[trans_id].dm_ack[node].commit_status = FDS_COMMIT_MSG_ACKED;
	    }
	    return (0);
	  }
	}

	return 0;

}

static  int fds_set_smack_status( uint32_t ipAddr, uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&rwlog_tbl[trans_id].sm_ack[node].ipAddr, &ipAddr, 4) == 0) {
	    if (rwlog_tbl[trans_id].sm_ack[node].ack_status != FDS_SET_ACK) {
	      rwlog_tbl[trans_id].sm_ack[node].ack_status = FDS_SET_ACK;
	      rwlog_tbl[trans_id].sm_ack_cnt++;
	    }
	    return (0);
	  }
	}

	return 0;
}


void fbd_process_req_timeout(unsigned long arg)
{
  uint32_t trans_id;
  FDS_JOURN *txn;
  struct request *req;

  trans_id = (uint32_t)arg;
  txn = &rwlog_tbl[trans_id];
  if (txn->trans_state != FDS_TRANS_EMPTY) {
    txn->trans_state = FDS_TRANS_EMPTY;
    req = txn->write_ctx;
    if (req) { 
      txn->write_ctx = 0;
      printf(" Timing out, responding to  the block : %p \n ",req);
      // __blk_end_request_all(req, 0); 
    }
  }
  
}

void fbd_complete_req(uint32_t trans_id, td_request_t *req, int status) {

  FDS_JOURN *txn;

  txn = &rwlog_tbl[trans_id];
  txn->comp_req(txn->comp_arg1, txn->comp_arg2, req,  status);

}


static int fds_process_read( uint8_t  *rx_buf)
{
	fdsp_msg_t  *rd_msg;
	fdsp_get_object_t *get_obj_rsp;
	td_request_t *req; 
	uint32_t trans_id;
	int   data_size;
	uint64_t data_offset;
	char  *data_buf;
	

	rd_msg = (fdsp_msg_t *)rx_buf;
	trans_id = rd_msg->req_cookie;
	if (rwlog_tbl[trans_id].trans_state == FDS_TRANS_EMPTY) {
	  return (0);
	}

	printf("Processing read response for trans %d\n", trans_id);
	req = (td_request_t *)rwlog_tbl[trans_id].write_ctx;
	data_size    = req->secs * HVISOR_SECTOR_SIZE;
	data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
	data_buf = req->buf;
   	rwlog_tbl[trans_id].trans_state = FDS_TRANS_EMPTY;
	rwlog_tbl[trans_id].write_ctx = 0;
	// del_timer(rwlog_tbl[trans_id].p_ti);
	get_obj_rsp = (fdsp_get_object_t *)&(rd_msg->payload);
	
	printf(" inside  the read function \n");
	printf(" length: %d   buf: %p \n ", data_size, data_buf);

	memcpy((void *)data_buf, (void *)&(get_obj_rsp->data_obj[0]), data_size); 
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

static int fds_process_write(fdsp_msg_t  *rx_msg)
{
	fdsp_msg_t   *sm_msg_ptr;
	int rc, result = 0;
	int flag = 0, node=0;
	td_request_t *req; 
	struct fbd_device *fbd;

	uint32_t trans_id;
	fds_doid_t *doid_list[1];

	fdsp_msg_t *wr_msg;
	FDS_JOURN  *txn;

	trans_id = rx_msg->req_cookie; 
	txn = &rwlog_tbl[trans_id];
	// Check sanity here, if this transaction is valid and matches with the cookie we got from the message
	if (txn->trans_state == FDS_TRANS_EMPTY) {
	  return (0);
	}



	fbd = (struct fbd_device *)(txn->fbd_ptr);
	req = (td_request_t *)txn->write_ctx;
	wr_msg = txn->dm_msg;
	sm_msg_ptr = (fdsp_msg_t *)(rwlog_tbl[trans_id].sm_msg);


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
	    
	    doid_list[0] = (fds_doid_t *)&(sm_msg_ptr->payload.put_obj.data_obj_id);
	    
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
			      wr_msg->payload.update_catalog.dm_operation = FDS_DMGR_CMD_COMMIT_TXN;
			      result = send_data_dm(fbd, 1, wr_msg, sizeof(fdsp_msg_t), 0,
						    rwlog_tbl[trans_id].dm_ack[node].ipAddr);
			      if ( result < 0)
				{
				  printf(" DM:Error %d: sending the data \n ",result);
				}
			      else {
				txn->dm_ack[node].commit_status = FDS_COMMIT_MSG_SENT;
			      }
			    }
		  }
		}

	}

	return 0;
}



int   fds_process_rx_message(uint8_t  *rx_buf, uint32_t src_ip)
{
	fdsp_msg_t	*rx_msg;

	/*  get the fds  header */
	if(rx_buf == NULL)
	{
		printf(" Error: Null Buffer returned from kernel_recvmsg\n");
		return -1;
	}
	rx_msg  = (fdsp_msg_t *)rx_buf;

	printf("Rx Info: cookie: %d ; src_id:%d ; src_ip: 0x%x ; msg_code:%d \n",rx_msg->req_cookie,rx_msg->src_id,src_ip, rx_msg->msg_code );


	if (rx_msg->src_id == FDSP_STOR_MGR ) 
	{
		/* check the response status from  SM */
		if(rx_msg->result == FDSP_ERR_FAILED) 
		{
			printf(" Error:  response  from  SM  code: %d \n ",rx_msg->err_code); 
			return -1;
		}
		
		switch (rx_msg->msg_code)
		{
			case  FDSP_MSG_PUT_OBJ_RSP:
			  printf(" SM PUT OBJ RSP ; ");
					fds_set_smack_status(src_ip, rx_msg->req_cookie);
					break;				
			case  FDSP_MSG_GET_OBJ_RSP:
			  printf(" SM GET OBJ RSP ; ");
					break;				
			case  FDSP_MSG_VERIFY_OBJ_RSP:
					break;				
			case  FDSP_MSG_OFFSET_WRITE_OBJ_RSP:
					break;				
			case  FDSP_MSG_REDIR_WRITE_OBJ_RSP:
					break;				
			default :
				printf(" Error:%d Wrong mesage code received from  SM \n",rx_msg->msg_code);
				return -1;
		}
	}
	else if (rx_msg->src_id == FDSP_DATA_MGR ) 
	{
		/* check the response status from  SM */
		if (rx_msg->result == FDSP_ERR_FAILED ) 
		{
			printf(" Error:  response  from  DM  code: %d \n ",rx_msg->err_code); 
			return -1;
		}

		if (rx_msg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_RSP)
		{
		  if (rx_msg->payload.update_catalog.dm_operation == FDS_DMGR_CMD_OPEN_TXN) {
		    fds_set_dmack_status(src_ip, rx_msg->req_cookie);
		    printf(" DM OpenTrans RSP ;");
		  } else {
		    fds_set_dm_commit_status(src_ip, rx_msg->req_cookie);
		    printf(" DM CommitTrans RSP ; ");
		  }
		}
		else 
		{
		  printf(" Error:%d Wrong mesage code received from  SM \n",rx_msg->msg_code);
		  return -1;
		}
	}

	printf(" Txn State: cookie: %d ; sm-acks : %d ; dm-acks : %d ; dm-commits : %d ; st_flag %d ; ctx:%p \n",
	       rx_msg->req_cookie,rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt,
	       rwlog_tbl[rx_msg->req_cookie].dm_ack_cnt,
	       rwlog_tbl[rx_msg->req_cookie].dm_commit_cnt,
	       rwlog_tbl[rx_msg->req_cookie].st_flag,
	       rwlog_tbl[rx_msg->req_cookie].write_ctx);

	/* if we  get two( min)  response  from  DM and SM, start processing   the read and write  */ 

	if ((rx_msg->msg_code == FDSP_MSG_PUT_OBJ_RSP ) || (rx_msg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_RSP ))
	{
	  fds_process_write(rx_msg);
	}

	if (rx_msg->msg_code == FDSP_MSG_GET_OBJ_RSP)
	  fds_process_read(rx_buf);

	/* for now release  response  to  block, else driver will hang */

	return 0;
}

