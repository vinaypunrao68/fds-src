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
#include "fds.h"
#include "../include/fds_commons.h"
#include "../include/fdsp.h"
#include "../include/data_mgr.h"
#include "fbd.h"


FDS_JOURN	rwlog_tbl[FDS_READ_WRITE_LOG_ENTRIES];


/*
	- we may have to  revisit this logic.
*/
#if 0
static  int fds_get_dmack_status( uint32_t ipAddr[0], uint32_t trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr,ipAddr,4))
				return (rwlog_tbl[trans_id].dm_ack[node].ack_status);
	}
	return 0;
}
#endif

static  int fds_set_dmack_status( uint32_t ipAddr[0], uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
			if (rwlog_tbl[trans_id].dm_ack[node].ipAddr == *ipAddr)
			if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr, ipAddr,4))
				return (rwlog_tbl[trans_id].dm_ack[node].ack_status = FDS_SET_ACK);
	}

	return 0;

}

#if 0

static  int fds_cls_dmack_status( uint32_t ipAddr[0], uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].dm_ack[node].ipAddr,ipAddr,4)
				return (rwlog_tbl[trans_id].dm_ack[node].ack_status = FDS_CLS_ACK);
	}
	return 0;
}


static  int fds_get_smack_status( uint32_t ipAddr[0], uint32_t trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].sm_ack[node].ipAddr,ipAddr,4))
				return (rwlog_tbl[trans_id].sm_ack[node].ack_status);
	}
	return 0;
}
#endif

static  int fds_set_smack_status( uint32_t ipAddr[0], uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
			if (memcmp(&rwlog_tbl[trans_id].sm_ack[node].ipAddr,ipAddr,4))
				return (rwlog_tbl[trans_id].sm_ack[node].ack_status = FDS_SET_ACK);
	}

	return 0;
}

#if 0
static  int fds_cls_smack_status( uint32_t ipAddr[0], uint32_t  trans_id)
{
	int node =0;

	for (node = 0; node < FDS_MAX_SM_NODES_PER_CLST; node++)
	{
			if (memcpy(&rwlog_tbl[trans_id].sm_ack[node].ipAddr, ipAddr,4))
				return (rwlog_tbl[trans_id].sm_ack[node].ack_status = FDS_CLS_ACK);
	}
	return 0;
}

#endif

int fds_trans_cleanup( uint32_t  trans_id)
{

	if( rwlog_tbl[trans_id].sm_msg)
		kfree(rwlog_tbl[trans_id].sm_msg);

	
	if( rwlog_tbl[trans_id].sm_msg)
		kfree(rwlog_tbl[trans_id].sm_msg);

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

static int fds_process_read( uint8_t  *rx_buf)
{
	fdsp_msg_t  *rd_msg;
	struct request *req; 
	struct bio_vec *bv;
	struct req_iterator iter;
	int dir;
	uint32_t trans_id; 
	void *kaddr = NULL;
	uint8_t  *buf;
	int  i = 0;
	

	rd_msg = (fdsp_msg_t *)rx_buf;
	trans_id = rd_msg->req_cookie; 
	req = (struct request *)rwlog_tbl[trans_id].write_ctx;
   	dir = rq_data_dir(req);

printk(" inside  the read function \n");
	rq_for_each_segment(bv, req, iter)	
	{
		printk(" length: %d   page: %p \n ", bv->bv_len, bv->bv_page);
		kaddr = kmap(bv->bv_page);
		memcpy((void *)kaddr, (void *)(rx_buf +sizeof(fdsp_msg_t)), bv->bv_len); 
		/*
		  - how to handle the  length miss-match ( requested  length and  recived legth from SM
		  - we will have to handle sending more data due to length difference
		*/

		buf = ((rx_buf + sizeof(fdsp_msg_t)));
		for(i = 0; i < 200; i++)
			printk(" %c: ", *buf++);

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

static int fds_process_write(fdsp_msg_t  *wr_msg)
{
	fdsp_msg_t   *sm_msg_ptr;
	int rc, result = 0;
	int flag = 0, node=0;
	struct request *req; 
	struct fbd_device *fbd;
	uint32_t trans_id = wr_msg->req_cookie; 
	fds_object_id_t *p_obj_doid;
	//fds_object_id_t **doid_list;
	fds_doid_t *doid_list[1];

		fbd = (struct fbd_device *)(rwlog_tbl[trans_id].fbd_ptr);
		sm_msg_ptr = (fdsp_msg_t *)(rwlog_tbl[trans_id].sm_msg);
	    	
		/* 
		  -get the request  context  from   trans log 
		  - respond to the block device 
		*/
		req = (struct request *)rwlog_tbl[trans_id].write_ctx;
		if (req)
		__blk_end_request_all(req, 0);

		/*
		  -  add the vvc entry
		  -  If we are thinking of adding the cache , we may have to keep a copy on the cache 
	     */
		
		p_obj_doid = &(sm_msg_ptr->payload.put_obj.data_obj_id);
//		p_obj_doid = &(wr_msg->payload.put_obj.data_obj_id);
		doid_list[0] = (fds_doid_t *)p_obj_doid;

		rc = vvc_entry_update(fbd->vhdl, fbd->blk_name, 1, (const doid_t **)doid_list);
		if (rc)
		{
			printk("Error on creating vvc entry. Error code : %d\n", rc);
		}

		/*
		   -  browse through the list of the DM nodes sent the response .
		   -  respond to  DM - commit 
		 */
		
#if 0
		for (node = 0; node < FDS_MAX_DM_NODES_PER_CLST; node++)
		{
			if (rwlog_tbl[trans_id].dm_ack[node].ack_status)
			{
	    		wr_msg->payload.update_catalog.dm_operation = FDS_DMGR_CMD_COMMIT_TXN;
				result = send_data_dm(fbd, 1, wr_msg, sizeof(fdsp_msg_t),flag);
				if ( result < 0)
				{
					printk(" DM:Error %d: sending the data \n ",result);
				}
			}
		}
#endif

	return 0;
}


int   fds_process_rx_message(uint8_t  *rx_buf)
{
	fdsp_msg_t	*rx_msg;

	/*  get the fds  header */
	if(rx_buf == NULL)
	{
		printk(" Error: Null Buffer returned from kernel_recvmsg\n");
		return -1;
	}
	rx_msg  = (fdsp_msg_t *)rx_buf;

	printk("cookie: %d  src_id:%d msg_code:%d\n",rx_msg->req_cookie,rx_msg->src_id,rx_msg->msg_code );

	/* check for the bogus message */
	if (rwlog_tbl[rx_msg->req_cookie].trans_state != FDS_TRANS_OPEN) 
	{
		printk(" Error: %d Incorrect message received \n",rx_msg->req_cookie);
		/* free  the buffer  and return */
		return -1;
	}

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
					rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt++; 
					fds_set_smack_status(rx_msg->src_ip_addr, rx_msg->req_cookie);
					break;				
			case  FDSP_MSG_GET_OBJ_RSP:
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
				rwlog_tbl[rx_msg->req_cookie].dm_ack_cnt++; 
				fds_set_dmack_status(rx_msg->src_ip_addr, rx_msg->req_cookie);
				printk(" recived the DM ack response \n");
		}
		else 
		{
				printk(" Error:%d Wrong mesage code received from  SM \n",rx_msg->msg_code);
				return -1;
		}
	}

	printk("  cookie: %d sm ack: %d  ctx:%p\n",rx_msg->req_cookie,rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt, \
							rwlog_tbl[rx_msg->req_cookie].write_ctx);

	/* if we  get two( min)  response  from  DM and SM, start processing   the read and write  */ 

	if ((rx_msg->msg_code == FDSP_MSG_PUT_OBJ_RSP ) || (rx_msg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_RSP ))
	{
		if((rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt ==  FDS_MIN_ACK) &&
	   	   (rwlog_tbl[rx_msg->req_cookie].dm_ack_cnt ==  FDS_MIN_ACK) &&
	   	   (!rwlog_tbl[rx_msg->req_cookie].st_flag))
		{
			fds_process_write(rx_msg);
		}
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

