/* Copyright (c) 2013 - 2014 All Right Reserved
 x   Company= Formation  Data Systems. 
 o* 
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
 *        - Formation  block driver 
 *        - IOCTL interface  to the blokc driver 
 *        - sysfs interface to  block driver 
 *        - Murmur hash-3 support 
 *        - IO thread  scheduling  ( basic one ) 
 */

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

// #include "tapdisk.h"
#include "ubd.h"
#include "hvisor_lib.h"

#include "vvclib.h"
#include "fds_commons.h"
#include "fdsp.h"
#include "data_mgr.h"
#include "fbd.h"
#include "fds.h"
#include "fbd_hash.h"



struct fbd_device *fbd_dev;
struct task_struct *fbd_thread_id;
struct task_struct *fbd_thread_id_rx;

struct  fbd_contbl *fbd_con;
extern  FDS_JOURN rwlog_tbl[];


/*
  Initialise the connection  management table 
*/
static int init_fbdtbl(void)
{

  fbd_con = (struct fbd_contbl *)malloc(sizeof(*fbd_con));
  if (fbd_con == NULL)
    return -ENOMEM;
  memset(fbd_con, 0, sizeof(*fbd_con));

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


uint8_t  rx_buf[8192]; /* 4k block size +  some more mesage header, needs to be refined */

static void *fds_rx_io_proc(void *data)
{
  int sock = fbd_con->sock;
  struct msghdr msg;
  struct iovec iov;
  struct sockaddr_in cAddr;
  int  length;
  
  if (!sock)
    {
      printf("Trying to send  on closed  DM socket \n");
    }

  if ( fbd_dev->proto_type != FBD_PROTO_TCP)
    {
      cAddr.sin_family = AF_INET;
      cAddr.sin_addr.s_addr = htonl(fbd_dev->udp_destAddr);
      cAddr.sin_port = htons(FBD_CLUSTER_UDP_PORT_DM);
      
      msg.msg_name = &cAddr;
      msg.msg_namelen = sizeof(cAddr);
    }
  else
    {
      msg.msg_name = NULL;
      msg.msg_namelen = 0;
    }
  
  msg.msg_control = NULL; 
  msg.msg_controllen = 0;
  msg.msg_iov    = (void *)&iov; 
  msg.msg_iovlen = 1;

  // set_user_nice(current, 19);
  while (1) 
    {
      
      // wait_event_interruptible(thread_wait, atomic_read(&fds_data_ready) || atomic_read(&fds_exit));
      
      // if (kthread_should_stop() || atomic_read(&fds_exit))
      //  break;
      
      iov.iov_base = rx_buf;
      iov.iov_len  = sizeof(rx_buf);
      sock = fbd_con->sock;
      
      while ((length = recvmsg(sock, &msg, 0)) > 0)
	fds_process_rx_message(rx_buf, ntohl(cAddr.sin_addr.s_addr));
      
      //atomic_set(&fds_data_ready, 0);
      
    }
  return 0;
}


/*
 open the connection with  SM and DM  
*/

int fds_cluster_conn(struct fbd_contbl *pCon) {

  int sock;
  struct sockaddr_in dAddr;
  int ret = 0;

  if ( fbd_dev->proto_type == FBD_PROTO_TCP)
    {
      printf(" TCP - socket create  \n");
        	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        	if (sock < 0)
		{
			perror(" Error: creating the TCP socket  \n");
                	return sock;
		}

		memset(&dAddr, 0, sizeof(struct sockaddr_in));
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(fbd_dev->tcp_destAddr);
		dAddr.sin_port = htons(FBD_CLUSTER_TCP_PORT_DM);


        	ret = connect(sock, (struct sockaddr *)&dAddr, sizeof(dAddr));


        	if (ret < 0)
		{
                	perror("Error  : TCP connect  error\n");
			close(sock);
			fbd_dev->sock = -1;
			return ret;
		}
		// set_sock_callbacks(sock,pCon);
	}
	else if (fbd_dev->proto_type == FBD_PROTO_UDP)
	{
       		sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
       		if (sock < 0)
		{
			perror("Error : creating the UDP socket  \n");
                	return (sock);
		}

		/* setup the call back for the socket  */
		// set_sock_callbacks(sock,pCon);
		printf(" Successfully created UDP  socket : %d\n", sock);
	}

       	pCon->sock = sock;
	fbd_dev->sock = sock;

        return sock;
}




/* socket dis connect */
static int close_cluster_con(struct fbd_contbl *con)
{
	int rc;
	rc = close(con->sock);
	con->sock = -1;
	return rc;
}


static int fbd_set_tgt_blksize(struct fbd_device *fbd, int data)
{
		fbd->blocksize = data;
		fbd->bytesize &= ~(fbd->blocksize-1);
		return 0;
}

static int fbd_set_tgt_size(struct fbd_device *fbd, int data)
{
		fbd->bytesize = data & ~(fbd->blocksize-1);
		return 0;
}

static int fbd_set_base_port(struct fbd_device *fbd, int data) {
  fbd->stor_mgr_port = data;
  fbd->data_mgr_port = data+1;
  return (0);
}

static int fbd_process_cluster_conn(struct fbd_device *fbd, int data)
{

printf("fbd_process_cluster_conn:  received:%x fbd:%pfbd_dev:%p proto:%d fbd_con:%p\n ",data,fbd,fbd_dev,fbd->proto_type,fbd_con);
	if(fbd->proto_type  == FBD_PROTO_TCP)
		fbd_dev->tcp_destAddr =  data;
	else if (fbd->proto_type == FBD_PROTO_UDP)
		fbd_dev->udp_destAddr =  data;
	else
	{	
		printf(" Error: Wrong proto type received \n");
		return -1;
	}

	/* init the sockets */

	fbd_dev->sock = fds_cluster_conn(fbd_con); 
	if (fbd_dev->sock <= 0){
		printf(" Error DM: socket is not created  yet .. \n");
		return -1;
	}

	return 0;

}

static int fbd_process_cluster_dconn(struct fbd_device *fbd, int data)
{
	printf("fbd_process_cluster_dconn:  received \n ");

	/* close  the sockets */
	close_cluster_con(fbd_con);

	return 0;

}

static void sock_shutdown(struct fbd_device *fbd, int lock)
{
printf(" inside sock_shutdown \n");
	/* Forcibly shutdown the socket causing all listeners
	 * to error
	 *
	 * FIXME: This code is duplicated from sys_shutdown, but
	 * there should be a more generic interface rather than
	 * calling socket ops directly here */
 if (lock)
   pthread_mutex_lock(&fbd->tx_lock);

 if (fbd->sock){
   printf("DM shutting down socket\n");
   close(fbd->sock);
   fbd->sock = -1;
 }
 if (lock)
   pthread_mutex_unlock(&fbd->tx_lock);
}

static int send_msg_sm(struct fbd_device *fbd, int send, void *buf, int size,
		       int msg_flags, long int node_ipaddr)
{
	int result;
	//struct timer_list ti;
	struct sockaddr_in dAddr;
	int sock = fbd_con->sock;
	struct msghdr msg;
	struct iovec iov;
	sigset_t blocked, oldset;
	fbd->xmit_timeout = 1000;


	if (sock <= 0)
	{
		printf("Trying to send  on closed  SM socket \n");
		return -EINVAL;
	}

	if ( fbd_dev->proto_type != FBD_PROTO_TCP)
	{
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(node_ipaddr);
		dAddr.sin_port = htons(fbd->stor_mgr_port);

		msg.msg_name = &dAddr;
		msg.msg_namelen = sizeof(dAddr);
	}
	else
	{
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}
	
	printf(" SM ip addr - 0x%x, port: %d \n",(unsigned int) node_ipaddr, fbd->stor_mgr_port);
	/* block the signals interrupting  the  transmission */
	// siginitsetinv(&blocked, sigmask(SIGKILL));
	// sigprocmask(SIG_SETMASK, &blocked, &oldset);

	// current->flags |= PF_MEMALLOC;
	do {
		iov.iov_base = (void *)(buf); /* sm  message */
		iov.iov_len = sizeof(fdsp_msg_t);
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		result = sendmsg(sock, &msg, 0);

		if (result < size) {
		  printf("Send failed or truncated, result = %d\n", result);
		}

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}

		size -= result;
		buf += result;

	} while (size > 0);

	//	sigprocmask(SIG_SETMASK, &oldset, NULL);
	//	tsk_restore_flags(current, pflags, PF_MEMALLOC);
	// current->flags &= ~PF_MEMALLOC;
	// current->flags |=  pflags & PF_MEMALLOC;

	return result;
}

int send_data_dm(struct fbd_device *fbd, int send, void *buf, int size,
		 int msg_flags, long int node_ipaddr)
{
	int result;
	// struct timer_list ti;
	struct sockaddr_in dAddr;
	int sock = fbd_con->sock;
	struct msghdr msg;
	struct iovec iov;
	sigset_t blocked, oldset;
	fbd->xmit_timeout = 1000;



	if (sock <= 0)
	{
		printf("Trying to send  on closed  DM socket \n");
		return -EINVAL;
	}

	if ( fbd_dev->proto_type != FBD_PROTO_TCP)
	{
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(node_ipaddr);
		dAddr.sin_port = htons(fbd->data_mgr_port);

		msg.msg_name = &dAddr;
		msg.msg_namelen = sizeof(dAddr);
	}
	else
	{
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}
	
	printf(" DM ip addr - 0x%x, port: %d  Heade size: %d\n", (unsigned int) node_ipaddr, fbd->data_mgr_port,size);
	/* block the signals interrupting  the  transmission */
	//	siginitsetinv(&blocked, sigmask(SIGKILL));
	//    sigprocmask(SIG_SETMASK, &blocked, &oldset);

	//	current->flags |= PF_MEMALLOC;
	do {
		/* init the  message headr to vec[0] */
//		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
	  //	sock->sk->sk_allocation = GFP_NOIO;
		
		iov.iov_base = (void *)(buf);
		iov.iov_len = size;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		result = sendmsg(sock, &msg, 0);

#if 0
		if (signal_pending(current)) {
			siginfo_t info;
			printf(KERN_WARNING "fbd (pid %d: %s) got signal %d\n",
				task_pid_nr(current), current->comm,
				dequeue_signal_lock(current, &current->blocked, &info));
			result = -EINTR;
			sock_shutdown(fbd, !send);
			break;
		}
#endif

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	//	sigprocmask(SIG_SETMASK, &oldset, NULL);
//	tsk_restore_flags(current, pflags, PF_MEMALLOC);
//	current->flags &= ~PF_MEMALLOC;
//	current->flags |=  pflags & PF_MEMALLOC;

	return result;
}



static int send_data_sm(struct fbd_device *fbd, int send, void *buf, int size,
			int msg_flags, fdsp_msg_t *sm_msg, long int node_ipaddr)
{
  //	struct timer_list ti;
	struct sockaddr_in dAddr;
	int sock = fbd_con->sock;
	int result;
	struct msghdr msg;
	struct iovec iov[2];
	sigset_t blocked, oldset;
	fbd->xmit_timeout = 1000;

	if (sock <= 0)
	{
		printf("Trying to send  the  I/O's on closed SM socket \n");
		return -EINVAL;
	}

	if ( fbd_dev->proto_type != FBD_PROTO_TCP)
	{
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(node_ipaddr);
		dAddr.sin_port = htons(fbd->stor_mgr_port);

		msg.msg_name = &dAddr;
		msg.msg_namelen = sizeof(dAddr);
	}
	else
	{
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}

	printf(" SM ipaddr - 0x%x, port: %d \n", (unsigned int) node_ipaddr, fbd->stor_mgr_port);

	/* block the signals interrupting  the  I/O transmission */
	//siginitsetinv(&blocked, sigmask(SIGKILL));
	//sigprocmask(SIG_SETMASK, &blocked, &oldset);

	// current->flags |= PF_MEMALLOC;
	do {
		/* init the  message headr to vec[0] */
//		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
	  // sock->sk->sk_allocation = GFP_NOIO;
		
		iov[0].iov_base = (void *)sm_msg;
		iov[0].iov_len = sizeof(fdsp_msg_t) - sizeof(fdsp_payload_t) + sizeof(fdsp_put_object_t); // Data starts right at the end of fdsp_put_object
		iov[1].iov_base = buf;
		iov[1].iov_len = size;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;

		result = sendmsg(sock, &msg, 0);
		if (result < size) {
			printf("Send failed or truncated. result - %d\n", result);
		}

#if 0
		if (signal_pending(current)) {
			siginfo_t info;
			printf(KERN_WARNING "fbd (pid %d: %s) got signal %d\n",
				task_pid_nr(current), current->comm,
				dequeue_signal_lock(current, &current->blocked, &info));
			result = -EINTR;
			sock_shutdown(fbd, !send);
			break;
		}
#endif 
		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	//	sigprocmask(SIG_SETMASK, &oldset, NULL);
//	tsk_restore_flags(current, pflags, PF_MEMALLOC);
	//current->flags &= ~PF_MEMALLOC;
	//current->flags |=  pflags & PF_MEMALLOC;

	return result;
}

static  int fds_init_sm_hdr(fdsp_msg_t *psm_msg)
{
	psm_msg->msg_code = FDSP_MSG_PUT_OBJ_REQ;
	psm_msg->msg_id =  1;

	psm_msg->major_ver = 0xa5;
	psm_msg->minor_ver = 0x5a;

	psm_msg->num_objects = 1;
	psm_msg->frag_len = 0;
	psm_msg->frag_num = 0;

	psm_msg->tennant_id = 0;
	psm_msg->local_domain_id = 0;
	psm_msg->glob_volume_id = 0;

	psm_msg->src_id = FDSP_STOR_HVISOR;
	psm_msg->dst_id = FDSP_STOR_MGR;

	return 0;
}

static  int fds_init_dm_hdr(fdsp_msg_t *pdm_msg)
{
	pdm_msg->msg_code = FDSP_MSG_UPDATE_CAT_OBJ_REQ;
	pdm_msg->msg_id =  1;

	pdm_msg->major_ver = 0xa5;
	pdm_msg->minor_ver = 0x5a;

	pdm_msg->num_objects = 1;
	pdm_msg->frag_len = 0;
	pdm_msg->frag_num = 0;

	pdm_msg->tennant_id = 0;
	pdm_msg->local_domain_id = 0;
	pdm_msg->glob_volume_id = 0;

	pdm_msg->src_id = FDSP_STOR_HVISOR;
	pdm_msg->dst_id = FDSP_DATA_MGR;

	pdm_msg->payload.update_catalog.dm_transaction_id = 0;
	pdm_msg->payload.update_catalog.dm_operation = FDS_DMGR_CMD_OPEN_TXN;

	return 0;
}


int hvisor_process_read_request(void *dev_hdl, td_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)
{
	int rc, result = 0;
	struct fbd_device *fbd;

  	int n_segments = 0;
	int flag;
	fdsp_msg_t   *sm_msg;
	uint32_t     trans_id = 0;
	//	struct timer_list *p_ti;
	

	struct  DOID {
	  uint64_t	doid;
	  uint64_t	doid1;
	}doid;

	fds_doid_t	*doid_list;

	int      data_size    = req->secs * HVISOR_SECTOR_SIZE;
	uint64_t data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
	char *data_buf = req->buf;
	SM_NODES *sm_nodes;
        SM_NODES *tmp_sm_node;
	long int sm_ipAddr;
	unsigned int doid_dlt_key;
	
	// fbd = req->rq_disk->private_data;
	fbd = fbd_dev;

	rc = vvc_entry_get(fbd->vhdl, "BlockName", &n_segments,(doid_t **)&doid_list);
	if (rc)
	  {
	    printf("Error reading the VVC catalog  Error code : %d req:%p\n", rc,req);
	    /* for now just return the  and ack the read. we will have to come up with logic to handle these cases */
	    // __blk_end_request_all(req, 0);
	    result=rc;
	    return result;
	  }

	sm_msg = malloc(sizeof(*sm_msg));
	if (!sm_msg)
	  {
	    printf("Error Read: Allocating the memory for sm message header %p\n",req);
	    //if(req)
	    //  __blk_end_request_all(req, -EIO);
	    return -ENOMEM;
	  }
	memset(sm_msg, 0, sizeof(*sm_msg));


	fds_init_sm_hdr(sm_msg);
	sm_msg->msg_code = FDSP_MSG_GET_OBJ_REQ;
	sm_msg->msg_id =  1;
	memcpy((void *)&(sm_msg->payload.get_obj.data_obj_id), (void *)&(doid_list[0].bytes[0]), sizeof(doid));
	doid_dlt_key = doid_list[0].bytes[0];
	sm_msg->payload.get_obj.data_obj_len = data_size;
	sm_msg->payload_len = data_size;
	printf(" read buf addr: %p \n", data_buf);
	printf("Read Req len: %d  offset: %d sm_msg:%p \
				doid:%llx:%llx \n",
	       data_size, (int32_t) data_offset, sm_msg, doid_list[0].obj_id.hash_high, doid_list[0].obj_id.hash_low);
			/* open transaction  */
	trans_id = get_trans_id();

	rwlog_tbl[trans_id].trans_state = FDS_TRANS_OPEN;
	rwlog_tbl[trans_id].fbd_ptr = (void *)fbd;
	rwlog_tbl[trans_id].write_ctx = (void *)req;
	rwlog_tbl[trans_id].comp_req = comp_req;
	rwlog_tbl[trans_id].comp_arg1 = arg1;
	rwlog_tbl[trans_id].comp_arg2 = arg2;
	rwlog_tbl[trans_id].sm_msg = (void *)sm_msg; 
	rwlog_tbl[trans_id].dm_msg = NULL;
	rwlog_tbl[trans_id].sm_ack_cnt = 0;
	rwlog_tbl[trans_id].dm_ack_cnt = 0;

	sm_msg->req_cookie = trans_id;

	sm_nodes = get_sm_nodes_for_doid_key(doid_dlt_key);
        list_for_each_entry(tmp_sm_node,& sm_nodes->list, list) {
		sm_ipAddr = tmp_sm_node->node_ipaddr;
		break;
	}

	pthread_mutex_lock(&fbd->tx_lock);

	result = send_msg_sm(fbd, 1, sm_msg, sizeof(fdsp_msg_t), 0, sm_ipAddr);
	if ( result < 0)
	  {
	    printf("  READ-SM:Error %d: Error  sending the data %p \n ",result,req);
	    pthread_mutex_unlock(&fbd->tx_lock);
	    // if(req)
	    //  __blk_end_request_all(req, -EIO);
	    return result;
	  }
	pthread_mutex_unlock(&fbd->tx_lock);
#if 0
	p_ti = (struct timer_list *)kzalloc(sizeof(struct timer_list), GFP_KERNEL);
	init_timer(p_ti);
	p_ti->function = fbd_process_req_timeout;
	p_ti->data = (unsigned long)trans_id;
	p_ti->expires = jiffies + HZ*5;
	add_timer(p_ti);
	rwlog_tbl[trans_id].p_ti = p_ti;
#endif

End:
	return 0;
}

int hvisor_process_write_request(void *dev_hdl, td_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)
{
	int result = 0;
	int flag;
	struct fbd_device *fbd;
	void *kaddr = NULL;
                 
	fdsp_msg_t   *sm_msg;
	fdsp_msg_t   *dm_msg;
	uint32_t     trans_id;
                           
	int      data_size    = req->secs * HVISOR_SECTOR_SIZE;
	uint64_t data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
	char *data_buf = req->buf;

	struct  DOID {
	uint64_t	doid;
	uint64_t	doid1;
	}doid;
	unsigned int doid_dlt_key;
	DM_NODES *dm_nodes;
	SM_NODES *sm_nodes;
	DM_NODES *tmp_dm_node;
	SM_NODES *tmp_sm_node;
	int num_nodes;
	//	struct timer_list *p_ti;

	fbd = fbd_dev;
	
	flag = 0;

	
	/* get the DOID */

	MurmurHash3_x64_128 (data_buf, data_size, 0, &doid );

	sm_msg = malloc(sizeof(*sm_msg));
	if (!sm_msg)
	  {
	    printf(" Error Allocating the memory for sm message header  %p\n",req);
	    //if(req)
	    //  __blk_end_request_all(req, -EIO);
	    return -ENOMEM;
	  }
	memset(sm_msg, 0, sizeof(*sm_msg));

	dm_msg = malloc(sizeof(*sm_msg));
	if (!dm_msg)
	  {
	    printf(" Error Allocating the memory for dm message header  %p \n",req);
	    // if(req)
	    //			__blk_end_request_all(req, -EIO);
	    return -ENOMEM;
	  }
			
	fds_init_sm_hdr(sm_msg);
	memcpy(sm_msg->src_ip_addr, &fbd->src_ip_addr, 4); 
	memcpy(sm_msg->src_ip_addr, &fbd->udp_destAddr, 4);
	memcpy((void *)&(sm_msg->payload.put_obj.data_obj_id), (void *)&doid, sizeof(doid));
	sm_msg->payload.put_obj.data_obj_len = data_size;
	sm_msg->payload.put_obj.volume_offset = data_offset;

	memcpy(sm_msg->src_ip_addr, &fbd->src_ip_addr, 4); 
	memcpy(sm_msg->src_ip_addr, &fbd->udp_destAddr, 4);
	fds_init_dm_hdr(dm_msg);
	dm_msg->payload_len = data_size;
	dm_msg->payload.update_catalog.volume_offset = data_offset;
	memcpy((void *)&(dm_msg->payload.update_catalog.data_obj_id), (void *)&doid, sizeof(doid));

	doid_dlt_key = doid.doid >> 56;

	trans_id = get_trans_id();

	printf("Write Req len: %d  offset: %d  buf:%p t_id: %d doid:%llx:%llx \n",
	       data_size, (uint32_t) data_offset, data_buf, trans_id, doid.doid, doid.doid1);
	/* open transaction  */

	rwlog_tbl[trans_id].fbd_ptr = fbd;
	rwlog_tbl[trans_id].trans_state = FDS_TRANS_OPEN;
	rwlog_tbl[trans_id].write_ctx = (void *)req;
	rwlog_tbl[trans_id].comp_req = comp_req;
	rwlog_tbl[trans_id].comp_arg1 = arg1;
	rwlog_tbl[trans_id].comp_arg2 = arg2;
	rwlog_tbl[trans_id].sm_msg = (void *)sm_msg; 
	rwlog_tbl[trans_id].dm_msg = (void *)dm_msg;
	rwlog_tbl[trans_id].fbd_ptr = (void *)fbd;
	rwlog_tbl[trans_id].sm_ack_cnt = 0;
	rwlog_tbl[trans_id].dm_ack_cnt = 0;
	rwlog_tbl[trans_id].dm_commit_cnt = 0;

	sm_msg->req_cookie = trans_id;
	dm_msg->req_cookie = trans_id;

	pthread_mutex_lock(&fbd->tx_lock);

	sm_nodes = get_sm_nodes_for_doid_key(doid_dlt_key);
	num_nodes = 0;
	list_for_each_entry(tmp_sm_node,& sm_nodes->list, list) {
	  
	  rwlog_tbl[trans_id].sm_ack[num_nodes].ipAddr = tmp_sm_node->node_ipaddr;
	  rwlog_tbl[trans_id].sm_ack[num_nodes].ack_status = FDS_CLS_ACK;
	  num_nodes++;
	  result = send_data_sm(fbd, 1, data_buf, data_size, 0, sm_msg, tmp_sm_node->node_ipaddr);
			  
	  if ( result < 0)
	    {
	      printf(" WRITE-SM:Error: Error  sending the data  %p\n",req);
	      // if(req)
	      // __blk_end_request_all(req, -EIO);
	      pthread_mutex_unlock(&fbd->tx_lock);
	      return result;
	    }
	}
	rwlog_tbl[trans_id].num_sm_nodes = num_nodes;


	dm_nodes = get_dm_nodes_for_volume(fbd->vol_id);
	num_nodes = 0;

	list_for_each_entry(tmp_dm_node, & dm_nodes->list, list) {
	  
	  rwlog_tbl[trans_id].dm_ack[num_nodes].ipAddr = tmp_dm_node->node_ipaddr;
	  rwlog_tbl[trans_id].dm_ack[num_nodes].ack_status = FDS_CLS_ACK;
	  rwlog_tbl[trans_id].dm_ack[num_nodes].commit_status = FDS_CLS_ACK;
	  num_nodes++;
	  result = send_data_dm(fbd, 1, dm_msg, sizeof(fdsp_msg_t), 0, tmp_dm_node->node_ipaddr);
	  if ( result < 0)
	    {
	      printf("WRITE-DM:Error: Error  sending the data  %p\n ",req);
	      pthread_mutex_unlock(&fbd->tx_lock);
	      return result;
	    }
	}
	rwlog_tbl[trans_id].num_dm_nodes = num_nodes;
	
	pthread_mutex_unlock(&fbd->tx_lock);
	// Schedule timer here to track the responses
#if 0
	p_ti = (struct timer_list *)kzalloc(sizeof(struct timer_list), GFP_KERNEL);
	init_timer(p_ti);
	p_ti->function = fbd_process_req_timeout;
	p_ti->data = (unsigned long)trans_id;
	p_ti->expires = jiffies + HZ*5;
	add_timer(p_ti);
	rwlog_tbl[trans_id].p_ti = p_ti;
#endif

End:
	return 0;
}



void *hvisor_lib_init(void)
{
	int err = -ENOMEM;
	int rc;

	init_fbdtbl(); /* socket transport structures */
	fbd_dev = malloc(sizeof(*fbd_dev));
	if (!fbd_dev)
	  return (void *)-ENOMEM;
	memset(fbd_dev, 0, sizeof(*fbd_dev));
	
	pthread_mutex_init(&fbd_dev->tx_lock, NULL);

	fbd_dev->blocksize = 4096; /* default value  */
	fbd_dev->bytesize = 0;
	/* we will have to  generate the dev id for multiple device support */
	fbd_dev->dev_id = 0;

	fbd_dev->vol_id = 1; /*  this should be comming from OM */
	if((fbd_dev->vhdl = vvc_vol_create(fbd_dev->vol_id, NULL,8192)) == 0 )
	{
		printf(" Error: creating the  vvc  volume \n");
		goto out;	
	}

	fbd_dev->stor_mgr_port = FBD_CLUSTER_UDP_PORT_SM;
	fbd_dev->data_mgr_port = FBD_CLUSTER_UDP_PORT_DM;

	/* init  DMT and DLT tables */
	fds_init_dmt();
	fds_init_dlt();
	/* sample  code to populate the DMT and DLT tables  */
	populate_dmt_dlt_tbl();
	/* the read write trans  table */
	fds_init_trans_log();

	fbd_dev->proto_type = FBD_PROTO_UDP;
	fbd_process_cluster_conn(fbd_dev, 0);

	rc = pthread_create(&fbd_dev->rx_thread, NULL, fds_rx_io_proc, fbd_dev);
	if (rc) {
	  printf(" Error: creating the   RX IO thread : %d \n", rc);
		goto out;	
	}

	return (fbd_dev);
out:
	free(fbd_dev);
	free(fbd_con);
	return (void *)err;
}
