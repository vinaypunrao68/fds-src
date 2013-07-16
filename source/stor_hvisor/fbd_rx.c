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
#include "vvclib.h"
#include "fds.h"
#include "../include/fds_commons.h"
#include "../include/fdsp.h"


FDS_JOURN	rwlog_tbl[FDS_READ_WRITE_LOG_ENTRIES];

int fds_trans_cleanup( uint32_t  trans_id)
{

	if( rwlog_tbl[trans_id].sm_msg)
		kfree(rwlog_tbl[trans_id].sm_msg);

	
	if( rwlog_tbl[trans_id].sm_msg)
		kfree(rwlog_tbl[trans_id].sm_msg);

	rwlog_tbl[trans_id].trans_state = FDS_TRANS_EMPTY;

	return 0;
}

int   fds_process_rx_message(uint8_t  *rx_buf)
{
	fdsp_msg_t	*rx_msg;
	struct request *req; 

	/*  get the fds  header */
	rx_msg  = (fdsp_msg_t *)rx_buf;

//printk("  cookie: %d \n",rx_msg->req_cookie);
//printk(" src_id : %d \n ", rx_msg->src_id );
//printk(" msg code : %d \n ", rx_msg->msg_code );

	if (rx_msg->src_id == FDSP_DATA_MGR ) 
	{
		/* check for the bogus message */
		if (rwlog_tbl[rx_msg->req_cookie].trans_state != FDS_TRANS_OPEN) 
		{
			printk(" Error: %d Incorrect message received \n",rx_msg->req_cookie);
			/* free  the buffer  and return */

		}
		
		switch (rx_msg->msg_code)
		{
			case  FDSP_MSG_PUT_OBJ_RSP:
					rwlog_tbl[rx_msg->req_cookie].dm_ack_cnt++; 
				    	
					break;				
			case  FDSP_MSG_GET_OBJ_RSP:
					break;				
			case  FDSP_MSG_VERIFY_OBJ_RSP:
					break;				
			case  FDSP_MSG_UPDATE_CAT_OBJ_RSP:
					break;				
			case  FDSP_MSG_OFFSET_WRITE_OBJ_RSP:
					break;				
			case  FDSP_MSG_REDIR_WRITE_OBJ_RSP:
					break;				
			default :
				printk(" Error:%d Wrong mesage code received from  DM \n",rx_msg->msg_code);
				return -1;
		}
	}
	else if (rx_msg->src_id == FDSP_STOR_MGR ) 
	{

		switch (rx_msg->msg_code)
		{
			case  FDSP_MSG_PUT_OBJ_RSP:
					rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt++; 
					printk(" recived the SM ack response \n");
					break;				
			case  FDSP_MSG_GET_OBJ_RSP:
					break;				
			case  FDSP_MSG_VERIFY_OBJ_RSP:
					break;				
			case  FDSP_MSG_UPDATE_CAT_OBJ_RSP:
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

printk("  cookie: %d sm ack: %d  ctx:%p\n",rx_msg->req_cookie,rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt,rwlog_tbl[rx_msg->req_cookie].write_ctx);

#if 0
	if((rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt ==  FDS_MIN_ACK) &&
	   (rwlog_tbl[rx_msg->req_cookie].dm_ack_cnt ==  FDS_MIN_ACK) &&
	   (!rwlog_tbl[rx_msg->req_cookie].st_flag))
#endif
	if(rwlog_tbl[rx_msg->req_cookie].sm_ack_cnt ==  FDS_MIN_ACK)
	{
	    	
		/* get the request  context  from   trans log */
		req = (struct request *)rwlog_tbl[rx_msg->req_cookie].write_ctx;
		__blk_end_request_all(req, 0);
		/* clear the  trans log */
		fds_trans_cleanup(rx_msg->req_cookie);
printk(" responding to the block devivce \n");
	}
	else
	{
#if 0
		/* get the request  context  from   trans log */
		req = (struct request *)rwlog_tbl[rx_msg->req_cookie].write_ctx;
		__blk_end_request_all(req, -EIO);
#endif
	}

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
	}

  return 0;
}


int get_trans_id(void)
{
	static int t_id = 1;

	return t_id++;

}

