/* Copyright (c) 2013 - 2014 All Right Reserved
 *   Company= Formation  Data Systems. 
 * 
 *  This source is subject to the Formation Data systems Permissive License.
 *  All other rights reserved.
 * 
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *  
 * 
 * This file contains the  data  structures and code  corresponds to 
 *                - Receive path processing  
 */

#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <net/sock.h>
#include <linux/net.h>
#include "vvclib.h"
#include "../include/fds_commons.h"
#include "../include/fdsp.h"
#include "../include/data_mgr.h"
#include "blktap.h"
#include "fbd.h"
#include "fds.h"


FDS_JOURN	rwlog_tbl[FDS_READ_WRITE_LOG_ENTRIES];


/*
	- we may have to  revisit this logic.
*/
#if 0
static  int fds_get_dmack_status( uint32_t ipAddr, uint32_t trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr,&ipAddr,4) == 0)
				return (rwlog_tbl[trans_id].dm_ack[node].ack_status);
	}
	return 0;
}
#endif

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

#if 0

static  int fds_cls_dmack_status( uint32_t ipAddr, uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
	  if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr,&ipAddr,4) == 0)
				return (rwlog_tbl[trans_id].dm_ack[node].ack_status = FDS_CLS_ACK);
	}
	return 0;
}


static  int fds_get_smack_status( uint32_t ipAddr, uint32_t trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].sm_ack[node].ipAddr,&ipAddr,4))
				return (rwlog_tbl[trans_id].sm_ack[node].ack_status);
	}
	return 0;
}
#endif

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

#if 0
static  int fds_cls_smack_status( uint32_t ipAddr, uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].sm_ack[node].ipAddr, &ipAddr,4) == 0)
				return (rwlog_tbl[trans_id].sm_ack[node].ack_status = FDS_CLS_ACK);
	}
	return 0;
}

#endif

int fds_trans_cleanup( uint32_t  trans_id)
{

	if( rwlog_tbl[trans_id].sm_msg)
		kfree(rwlog_tbl[trans_id].sm_msg);

	
	if( rwlog_tbl[trans_id].dm_msg)
		kfree(rwlog_tbl[trans_id].dm_msg);

	rwlog_tbl[trans_id].trans_state = FDS_TRANS_EMPTY;

	return 0;
}


void  fds_lt_cb_proc( uint32_t  trans_id)
{
	if (rwlog_tbl[trans_id].trans_state == FDS_TRANS_EMPTY)
	{
		/* 		
		  -tranaction could have completed successfully  OR 
		  -tranaction id is  wrong. How to diffferntiate ?? 
           	 */ 
		printk("Transaction is completed- nothing to process \n");
	}
	else
	{
		/*
		   - pretty much same as ST, will have to identify what else need to be done 
		*/
		/* mark the  timeout flag  */
		rwlog_tbl[trans_id].lt_flag  = FDS_TIMER_TIMEOUT;

	}
	return;
}




void  fds_st_cb_proc(uint32_t trans_id)
{
	struct request *req; 
	fdsp_msg_t	*sm_msg = (fdsp_msg_t *)rwlog_tbl[trans_id].sm_msg;
	fdsp_msg_t	*dm_msg = (fdsp_msg_t *)rwlog_tbl[trans_id].dm_msg;
	req = (struct request *)rwlog_tbl[trans_id].write_ctx;

	if (rwlog_tbl[trans_id].trans_state == FDS_TRANS_EMPTY)
	{
		/* 		
	  		-tranaction could have completed successfully  OR 
	  		-tranaction id is  wrong. How to diffferntiate ?? 
			-free the resources 
       	 */ 
		printk("Transaction is completed- nothing to process \n");
		
	}

	if ((sm_msg->msg_code == FDSP_MSG_PUT_OBJ_REQ ) || (dm_msg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_REQ ))
	{
		/*
	  		- transaction  is  open  and timer timed out.
	  		- Re-try the transaction- will take this up later . 
	  		- For now   release the resources and   ack the block device  with Error 
	  		- Send a message  to the SM and DM  to clear the tranaction 
	 		- send a message to OM - please check on this node ??
		*/ 

printk(" inside the  st call backl \n");
		/* mark the  timeout flag  */
		rwlog_tbl[trans_id].st_flag  = FDS_TIMER_TIMEOUT;
		fds_trans_cleanup(trans_id);
		if(req)
			__blk_end_request_all(req, -EIO); /* Err to  block driver */
	}
	else if (sm_msg->msg_code == FDSP_MSG_GET_OBJ_REQ)
	{
		/* 
			- read request timed out 
			- could  be primary node does not have the copy 
			- send  a request to seconday  node 
			- should  we enable the timer  again ??
		 */

	}
	return;
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
      printk(" Timing out, responding to  the block : %p \n ",req);
      __blk_end_request_all(req, 0); 
    }
  }
  
}

static int fds_process_read( uint8_t  *rx_buf)
{
	fdsp_msg_t  *rd_msg;
	fdsp_get_object_t *get_obj_rsp;
	struct request *req; 
	struct bio_vec *bv;
	struct req_iterator iter;
	int dir;
	uint32_t trans_id; 
	void *kaddr = NULL;
	uint8_t  *buf;
	

	rd_msg = (fdsp_msg_t *)rx_buf;
	trans_id = rd_msg->req_cookie;
	if (rwlog_tbl[trans_id].trans_state == FDS_TRANS_EMPTY) {
	  return (0);
	}

	printk("Processing read response for trans %d\n", trans_id);
	req = (struct request *)rwlog_tbl[trans_id].write_ctx;
   	dir = rq_data_dir(req);
	rwlog_tbl[trans_id].trans_state = FDS_TRANS_EMPTY;
	rwlog_tbl[trans_id].write_ctx = 0;
	del_timer(rwlog_tbl[trans_id].p_ti);
	get_obj_rsp = (fdsp_get_object_t *)&(rd_msg->payload);
	
printk(" inside  the read function \n");
	rq_for_each_segment(bv, req, iter)	
	{
		printk(" length: %d   page: %p \n ", bv->bv_len, bv->bv_page);
		kaddr = kmap(bv->bv_page);
		memcpy((void *)kaddr, (void *)&(get_obj_rsp->data_obj[0]), bv->bv_len); 
		/*
		  - how to handle the  length miss-match ( requested  length and  recived legth from SM
		  - we will have to handle sending more data due to length difference
		*/

		buf = &get_obj_rsp->data_obj[0];
		//		for(i = 0; i < 200; i++)
		//	printk(" %c: ", *buf++);

//		bv->bv_page = bv->bv_page + bv->bv_len;
	}

	/*
	 - respond to the block device- data ready 
	*/
	
	printk(" responding to  the block : %p \n ",req);
	if(req)
	__blk_end_request_all(req, 0); 

	return 0;
}

static int fds_process_write(fdsp_msg_t  *rx_msg)
{
	fdsp_msg_t   *sm_msg_ptr;
	int rc, result = 0;
	int flag = 0, node=0;
	struct request *req; 
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
	req = (struct request *)txn->write_ctx;
	wr_msg = txn->dm_msg;
	sm_msg_ptr = (fdsp_msg_t *)(rwlog_tbl[trans_id].sm_msg);


	if (txn->trans_state == FDS_TRANS_OPEN) {
	  if ((txn->sm_ack_cnt < FDS_MIN_ACK) || (txn->dm_ack_cnt < FDS_MIN_ACK)) {
	    return (0);
	  }
	  printk(" **** State Transition to OPENED *** : Received min ack from  DM and SM. \n\n");
	  txn->trans_state = FDS_TRANS_OPENED;
	
	} else if (txn->trans_state == FDS_TRANS_OPENED) {
	  if (txn->dm_commit_cnt >= FDS_MIN_ACK) {
	    printk(" **** State Transition to COMMITTED *** : Received min commits from  DM \n\n ");
	    txn->trans_state = FDS_TRANS_COMMITTED;
	  }
	} else if (txn->trans_state == FDS_TRANS_COMMITTED) {
	  if ((txn->sm_ack_cnt == txn->num_sm_nodes) && (txn->dm_commit_cnt == txn->num_dm_nodes)) {
	    printk(" **** State Transition to SYCNED *** : Received all acks and commits from  DM and SM. Ending req %p \n\n ", req);
	    txn->trans_state = FDS_TRANS_SYNCED;
	    /*
	      -  add the vvc entry
	      -  If we are thinking of adding the cache , we may have to keep a copy on the cache 
	    */
	    
	    doid_list[0] = (fds_doid_t *)&(sm_msg_ptr->payload.put_obj.data_obj_id);
	    
	    rc = vvc_entry_update(fbd->vhdl, fbd->blk_name, 1, (const doid_t **)doid_list);
	    if (rc)
	      {
		printk("Error on creating vvc entry. Error code : %d\n", rc);
	      }

	    // destroy the txn, reclaim the space and return from here	    
	    txn->trans_state = FDS_TRANS_EMPTY;
	    txn->write_ctx = 0;
	    del_timer(txn->p_ti);
	    if (req)
	      __blk_end_request_all(req, 0);

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
			      result = send_data_dm(fbd, 1, wr_msg, sizeof(fdsp_msg_t),flag,
						    rwlog_tbl[trans_id].dm_ack[node].ipAddr);
			      if ( result < 0)
				{
				  printk(" DM:Error %d: sending the data \n ",result);
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
		printk(" Error: Null Buffer returned from kernel_recvmsg\n");
		return -1;
	}
	rx_msg  = (fdsp_msg_t *)rx_buf;

	printk("Rx Info: cookie: %d ; src_id:%d ; src_ip: 0x%x ; msg_code:%d \n",rx_msg->req_cookie,rx_msg->src_id,src_ip, rx_msg->msg_code );

#if 0
	/* check for the bogus message */
	if (rwlog_tbl[rx_msg->req_cookie].trans_state != FDS_TRANS_OPEN) 
	{
		printk(" Error: %d Incorrect message received \n",rx_msg->req_cookie);
		/* free  the buffer  and return */
		return -1;
	}
#endif

	if (rx_msg->src_id == FDSP_STOR_MGR ) 
	{
		/* check the response status from  SM */
		if(rx_msg->result == FDSP_ERR_FAILED) 
		{
			printk(" Error:  response  from  SM  code: %d \n ",rx_msg->err_code); 
			return -1;
		}
		
		switch (rx_msg->msg_code)
		{
			case  FDSP_MSG_PUT_OBJ_RSP:
			  printk(" SM PUT OBJ RSP ; ");
					fds_set_smack_status(src_ip, rx_msg->req_cookie);
					break;				
			case  FDSP_MSG_GET_OBJ_RSP:
			  printk(" SM GET OBJ RSP ; ");
					break;				
			case  FDSP_MSG_VERIFY_OBJ_RSP:
					break;				
			case  FDSP_MSG_OFFSET_WRITE_OBJ_RSP:
					break;				
			case  FDSP_MSG_REDIR_WRITE_OBJ_RSP:
					break;				
			default :
				printk(" Error:%d Wrong mesage code received from  SM \n",rx_msg->msg_code);
				return -1;
		}
	}
	else if (rx_msg->src_id == FDSP_DATA_MGR ) 
	{
		/* check the response status from  SM */
		if (rx_msg->result == FDSP_ERR_FAILED ) 
		{
			printk(" Error:  response  from  DM  code: %d \n ",rx_msg->err_code); 
			return -1;
		}

		if (rx_msg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_RSP)
		{
		  if (rx_msg->payload.update_catalog.dm_operation == FDS_DMGR_CMD_OPEN_TXN) {
		    fds_set_dmack_status(src_ip, rx_msg->req_cookie);
		    printk(" DM OpenTrans RSP ;");
		  } else {
		    fds_set_dm_commit_status(src_ip, rx_msg->req_cookie);
		    printk(" DM CommitTrans RSP ; ");
		  }
		}
		else 
		{
		  printk(" Error:%d Wrong mesage code received from  SM \n",rx_msg->msg_code);
		  return -1;
		}
	}

	printk(" Txn State: cookie: %d ; sm-acks : %d ; dm-acks : %d ; dm-commits : %d ; st_flag %d ; ctx:%p \n",
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




/* initialise the read/write transactional catalog  */

int fds_init_trans_log(void)
{
  int i =0;
	
	for(i=0; i<= FDS_READ_WRITE_LOG_ENTRIES; i++)
	{
		rwlog_tbl[i].trans_state = FDS_TRANS_EMPTY;
		rwlog_tbl[i].replc_cnt = 0;
		rwlog_tbl[i].sm_ack_cnt = 0;
		rwlog_tbl[i].dm_ack_cnt = 0;
		rwlog_tbl[i].st_flag = 0;
		rwlog_tbl[i].lt_flag = 0;
	}

  return 0;
}


int get_trans_id(void)
{
	static int t_id = 1;

	return t_id++;

}

