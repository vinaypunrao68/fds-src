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

#include <linux/types.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/kthread.h>
#include <net/tcp.h>

#include <asm/uaccess.h>
#include <asm/types.h>
#include "vvclib.h"
#include "fds.h"
#include "../include/fds_commons.h"
#include "../include/fdsp.h"
#include "../include/data_mgr.h"
#include "fbd.h"
#include "fbd_hash.h"


static DEFINE_SPINLOCK(fbd_lock);
struct fbd_device *fbd_dev;
struct task_struct *fbd_thread_id;
struct task_struct *fbd_thread_id_rx;
static DEFINE_MUTEX(ctl_mutex);

struct  fbd_contbl *fbd_con;
extern  FDS_JOURN rwlog_tbl[];
atomic_t fds_data_ready;
atomic_t fds_exit;
DECLARE_WAIT_QUEUE_HEAD(thread_wait);


/*
  Initialise the connection  management table 
*/
static int init_fbdtbl(void)
{

        fbd_con = kzalloc(sizeof(*fbd_con), GFP_KERNEL);
        if (fbd_con == NULL)
                return -ENOMEM;

	return 0;
}



uint8_t  rx_buf[5120]; /* 4k block size +  some more mesage header, needs to be refined */
static  int  fds_rx_io_proc(void *data)
{
	struct socket *sock = fbd_con->sock;
	struct msghdr msg;
	struct kvec iov;
	struct sockaddr_in cAddr;
	int  length;

	if (!sock)
	{
		printk("Trying to send  on closed  DM socket \n");
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

    	set_user_nice(current, 19);
	while (!kthread_should_stop() && !atomic_read(&fds_exit)) 
	{

		wait_event_interruptible(thread_wait, atomic_read(&fds_data_ready) || atomic_read(&fds_exit));

		if (kthread_should_stop() || atomic_read(&fds_exit))
			break;

		iov.iov_base = rx_buf;
		iov.iov_len  = sizeof(rx_buf);
		sock = fbd_con->sock;

		while ((length = kernel_recvmsg(sock,&msg, &iov, 1, sizeof(rx_buf),  \
							MSG_NOSIGNAL | MSG_DONTWAIT)) > 0)
		  fds_process_rx_message(rx_buf);

		  atomic_set(&fds_data_ready, 0);
			
	}
	return 0;
}

static  void  fds_sock_write_ready(struct sock *sk)
{


}

static  void  fds_sock_process_state(struct sock *sk)
{
	struct fbd_contbl *con =
		(struct fbd_contbl *)sk->sk_user_data;

	printk("fds_state_change %p  sk_state = %u\n",
	     con, sk->sk_state);


	switch (sk->sk_state) {
	case TCP_CLOSE:
		printk("fds_state_changed TCP_CLOSE\n");
	case TCP_CLOSE_WAIT:
		printk("fds_state_changed TCP_CLOSE_WAIT\n");
		break;
	case TCP_ESTABLISHED:
		printk("fds_state_changed TCP_ESTABLISHED\n");
		break;
	}

}


static  void  fds_rx_data_ready(struct sock *sk)
{

	atomic_set(&fds_data_ready, 1);
	wake_up_interruptible(&thread_wait);

}

static void set_sock_callbacks(struct socket *sock, struct fbd_contbl *con)
{
	struct sock *sk = sock->sk;
	sk->sk_user_data = (void *)con;
	sk->sk_data_ready = fds_rx_data_ready;
	sk->sk_write_space = fds_sock_write_ready;
	sk->sk_state_change = fds_sock_process_state;
}

/*
 open the connection with  SM and DM  
*/

static struct socket *fds_cluster_conn(struct fbd_contbl *pCon)
{
        struct socket *sock;
	struct sockaddr_in dAddr;
        int ret = 0;


	if ( fbd_dev->proto_type == FBD_PROTO_TCP)
	{
printk(" TCP - socket create  \n");
        	ret = sock_create_kern(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
        	if (ret)
		{
			printk(" Error: creating the TCP socket  \n");
                	return ERR_PTR(ret);
		}


		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(fbd_dev->tcp_destAddr);
		dAddr.sin_port = htons(FBD_CLUSTER_TCP_PORT_DM);


        	ret = sock->ops->connect(sock, (struct sockaddr *)&dAddr, sizeof(dAddr),
                                 O_NONBLOCK);
	
		if (ret == -EINPROGRESS) 
		{
			printk(" connection  in progress  sk_state = %u\n", sock->sk->sk_state);
			return NULL;
		}

        	if (ret < 0)
		{
                	printk("Error  : TCP connect  error %d\n", ret);
			sock_release(sock);
			pCon->sock = NULL;
			fbd_dev->sock = NULL;
			return NULL;
		}
		set_sock_callbacks(sock,pCon);
	}
	else if (fbd_dev->proto_type == FBD_PROTO_UDP)
	{
       		ret = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
       		if (ret < 0)
		{
			printk("Error : creating the UDP socket  \n");
                	return ERR_PTR(ret);
		}

		/* setup the call back for the socket  */
		set_sock_callbacks(sock,pCon);
		printk(" Successfully created UDP  socket \n");
	}

       	pCon->sock = sock;
		fbd_dev->sock = sock;


        if (ret < 0)
	{
		printk(" Error DM : creating the cluster connection : %d \n", ret);
                return ERR_PTR(ret);
	}
        return sock;
}




/* socket dis connect */
static int close_cluster_con(struct fbd_contbl *con)
{
	int rc;

//	if (!con->sock)
//		return 0;
	rc = con->sock->ops->shutdown(con->sock, SHUT_RDWR);
	sock_release(con->sock);
	con->sock = NULL;
	return rc;
}

/*
   sys fs  functions for  supporting the  fbd device  block
*/

static struct fbd_device *dev_to_fbd(struct device *dev)
{
	return container_of(dev, struct fbd_device, dev);
}
static ssize_t fbd_size_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct fbd_device *fbd_dev = dev_to_fbd(dev);

	return sprintf(buf, "%llu\n", (unsigned long long)fbd_dev->bytesize);
}

static ssize_t fbd_major_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct fbd_device *fbd_dev = dev_to_fbd(dev);

	return sprintf(buf, "%d\n", fbd_dev->dev_major);
}


static DEVICE_ATTR(size, S_IRUGO, fbd_size_show, NULL);
static DEVICE_ATTR(major, S_IRUGO, fbd_major_show, NULL);

static struct attribute *fbd_attrs[] = {
	&dev_attr_size.attr,
	&dev_attr_major.attr,
	NULL
};

static struct attribute_group fbd_attr_group = {
	.attrs = fbd_attrs,
};

static const struct attribute_group *fbd_attr_groups[] = {
	&fbd_attr_group,
	NULL
};

static void fbd_sysfs_dev_release(struct device *dev)
{
}

static struct device_type fbd_device_type = {
	.name		= "fbd",
	.groups		= fbd_attr_groups,
	.release	= fbd_sysfs_dev_release,
};


static int fbd_read_dmt_tbl(struct block_device *bdev, struct fbd_device *fbd, int data)
{
	show_dmt_entry(data);
  return 0;
}

static int fbd_read_dlt_tbl(struct block_device *bdev, struct fbd_device *fbd, int data)
{
	show_dlt_entry(data);

  return 0;
}

static int fbd_read_vvc_catalog(struct block_device *bdev, struct fbd_device *fbd, int data)
{
	int n_segments = 0;
	doid_t *doid_list = 0;
	int i, rc;

	rc = vvc_entry_get(fbd->vhdl, fbd->blk_name, &n_segments, &doid_list);

	if (rc)
	{
		printk("Error on retrieving vvc entry. Error code : %d\n", rc);
	}
	else
	{
		printk("block name : %s \n",fbd->blk_name);
		printk("Num segments: %d \n", n_segments);
		for (i = 0; i < n_segments; i++) {
			fds_object_id_t *p_obj_id;
			p_obj_id = (fds_object_id_t *)&doid_list[i][0];
			printk("doid: %llx-%llx", p_obj_id->hash_high, p_obj_id->hash_low);
		}
		printk("\n");
	}
	return 0;
}



static int fbd_set_tgt_blksize(struct block_device *bdev, struct fbd_device *fbd, int data)
{
		fbd->blocksize = data;
		fbd->bytesize &= ~(fbd->blocksize-1);
		bdev->bd_inode->i_size = fbd->bytesize;
		set_blocksize(bdev, fbd->blocksize);
//		set_capacity(fbd->disk, fbd->bytesize >> 9);
//printk(" Current blksize:%x  new blksize:%d  \n",bdev->bd_block_size,data);
		/* we are not able to set the block size. below code  seem  ot be helping. 
	           still need more evaluvation  on why ? 
  		 */
		if (data < bdev_logical_block_size(bdev))
			return -EINVAL;
		if (bdev->bd_block_size != data)
		{
			sync_blockdev(bdev);
			bdev->bd_block_size = data;
			bdev->bd_inode->i_blkbits = blksize_bits(data);
			kill_bdev(bdev);
		}

		return 0;
}

static int fbd_set_tgt_size(struct block_device *bdev, struct fbd_device *fbd, int data)
{
		fbd->bytesize = data & ~(fbd->blocksize-1);
		bdev->bd_inode->i_size = fbd->bytesize;
//		set_blocksize(bdev, fbd->blocksize);
		set_capacity(fbd->disk, fbd->bytesize >> 9);
		return 0;

}

static int fbd_process_cluster_conn(struct block_device *bdev, struct fbd_device *fbd, int data)
{

printk("fbd_process_cluster_conn:  received:%x fbd:%pfbd_dev:%p proto:%d fbd_con:%p\n ",data,fbd,fbd_dev,fbd->proto_type,fbd_con);
	if(fbd->proto_type  == FBD_PROTO_TCP)
		fbd_dev->tcp_destAddr =  data;
	else if (fbd->proto_type == FBD_PROTO_UDP)
		fbd_dev->udp_destAddr =  data;
	else
	{	
		printk(" Error: Wrong proto type received \n");
		return -1;
	}

	/* init the sockets */

	fbd_dev->sock = fds_cluster_conn(fbd_con); 
	if (fbd_dev->sock == NULL){
		printk(" Error DM: socket is not created  yet .. \n");
		return -1;
	}

	return 0;

}

static int fbd_process_cluster_dconn(struct block_device *bdev, struct fbd_device *fbd, int data)
{
	printk("fbd_process_cluster_dconn:  received \n ");

	/* close  the sockets */
	close_cluster_con(fbd_con);

		return 0;

}

static void fbd_end_request(struct request *req)
{
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;


	spin_lock_irqsave(q->queue_lock, flags);
	__blk_end_request_all(req, error);
	spin_unlock_irqrestore(q->queue_lock, flags);
}


static void sock_shutdown(struct fbd_device *fbd, int lock)
{
printk(" inside sock_shutdown \n");
	/* Forcibly shutdown the socket causing all listeners
	 * to error
	 *
	 * FIXME: This code is duplicated from sys_shutdown, but
	 * there should be a more generic interface rather than
	 * calling socket ops directly here */
	if (lock)
		mutex_lock(&fbd->tx_lock);

	if (fbd->sock){
		dev_warn(disk_to_dev(fbd->disk), "DM shutting down socket\n");
		kernel_sock_shutdown(fbd->sock, SHUT_RDWR);
		fbd->sock = NULL;
	}
	if (lock)
		mutex_unlock(&fbd->tx_lock);
}

static void fbd_xmit_timeout(unsigned long arg)
{
	struct task_struct *task = (struct task_struct *)arg;

	printk(KERN_WARNING "fbd: killing hung xmit (%s, pid: %d)\n",
		task->comm, task->pid);
	force_sig(SIGKILL, task);
}



static int send_msg_sm(struct fbd_device *fbd, int send, void *buf, int size,
		int msg_flags)
{
	int result;
	struct timer_list ti;
	struct sockaddr_in dAddr;
	struct socket *sock = fbd_con->sock;
	struct msghdr msg;
	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;
	fbd->xmit_timeout = 1000;



	if (!sock)
	{
		printk("Trying to send  on closed  SM socket \n");
		return -EINVAL;
	}

	if ( fbd_dev->proto_type != FBD_PROTO_TCP)
	{
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(fbd_dev->udp_destAddr);
		dAddr.sin_port = htons(FBD_CLUSTER_UDP_PORT_SM);

		msg.msg_name = &dAddr;
		msg.msg_namelen = sizeof(dAddr);
	}
	else
	{
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}
	
printk(" port: %d \n",FBD_CLUSTER_UDP_PORT_SM);
	/* block the signals interrupting  the  transmission */
	siginitsetinv(&blocked, sigmask(SIGKILL));
	sigprocmask(SIG_SETMASK, &blocked, &oldset);

	current->flags |= PF_MEMALLOC;
	do {
		/* init the  message headr to vec[0] */
//		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
		sock->sk->sk_allocation = GFP_NOIO;
		
		iov.iov_base = (void *)(buf); /* sm  message */
		iov.iov_len = sizeof(fdsp_msg_t);
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = msg_flags | MSG_NOSIGNAL;


		if (fbd->xmit_timeout) {
			init_timer(&ti);
			ti.function = fbd_xmit_timeout;
			ti.data = (unsigned long)current;
			ti.expires = jiffies + fbd->xmit_timeout;
			add_timer(&ti);
		}
		result = kernel_sendmsg(sock, &msg, &iov, 1, size);
		if (fbd->xmit_timeout)
			del_timer_sync(&ti);

		if (signal_pending(current)) {
			siginfo_t info;
			printk(KERN_WARNING "fbd (pid %d: %s) got signal %d\n",
				task_pid_nr(current), current->comm,
				dequeue_signal_lock(current, &current->blocked, &info));
			result = -EINTR;
			sock_shutdown(fbd, !send);
			break;
		}

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
//	tsk_restore_flags(current, pflags, PF_MEMALLOC);
	current->flags &= ~PF_MEMALLOC;
	current->flags |=  pflags & PF_MEMALLOC;

	return result;
}


int send_data_dm(struct fbd_device *fbd, int send, void *buf, int size,
		int msg_flags)
{
	int result;
	struct timer_list ti;
	struct sockaddr_in dAddr;
	struct socket *sock = fbd_con->sock;
	struct msghdr msg;
	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;
	fbd->xmit_timeout = 1000;



	if (!sock)
	{
		printk("Trying to send  on closed  DM socket \n");
		return -EINVAL;
	}

	if ( fbd_dev->proto_type != FBD_PROTO_TCP)
	{
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(fbd_dev->udp_destAddr);
		dAddr.sin_port = htons(FBD_CLUSTER_UDP_PORT_DM);

		msg.msg_name = &dAddr;
		msg.msg_namelen = sizeof(dAddr);
	}
	else
	{
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}
	
printk(" port: %d  Heade size: %d\n",FBD_CLUSTER_UDP_PORT_DM,size);
	/* block the signals interrupting  the  transmission */
	siginitsetinv(&blocked, sigmask(SIGKILL));
	sigprocmask(SIG_SETMASK, &blocked, &oldset);

	current->flags |= PF_MEMALLOC;
	do {
		/* init the  message headr to vec[0] */
//		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
		sock->sk->sk_allocation = GFP_NOIO;
		
		iov.iov_base = (void *)(buf);
		iov.iov_len = size;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = msg_flags | MSG_NOSIGNAL;


		if (fbd->xmit_timeout) {
			init_timer(&ti);
			ti.function = fbd_xmit_timeout;
			ti.data = (unsigned long)current;
			ti.expires = jiffies + fbd->xmit_timeout;
			add_timer(&ti);
		}
		result = kernel_sendmsg(sock, &msg, &iov, 1, size);
		if (fbd->xmit_timeout)
			del_timer_sync(&ti);

		if (signal_pending(current)) {
			siginfo_t info;
			printk(KERN_WARNING "fbd (pid %d: %s) got signal %d\n",
				task_pid_nr(current), current->comm,
				dequeue_signal_lock(current, &current->blocked, &info));
			result = -EINTR;
			sock_shutdown(fbd, !send);
			break;
		}

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
//	tsk_restore_flags(current, pflags, PF_MEMALLOC);
	current->flags &= ~PF_MEMALLOC;
	current->flags |=  pflags & PF_MEMALLOC;

	return result;
}



static int send_data_sm(struct fbd_device *fbd, int send, void *buf, int size,
		int msg_flags,fdsp_msg_t *sm_msg )
{
	struct timer_list ti;
	struct sockaddr_in dAddr;
	struct socket *sock = fbd_con->sock;
	int result;
	struct msghdr msg;
	struct kvec iov[2];
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;
	fbd->xmit_timeout = 1000;

	if (!sock)
	{
		printk("Trying to send  the  I/O's on closed SM socket \n");
		return -EINVAL;
	}

	if ( fbd_dev->proto_type != FBD_PROTO_TCP)
	{
		dAddr.sin_family = AF_INET;
		dAddr.sin_addr.s_addr = htonl(fbd_dev->udp_destAddr);
		dAddr.sin_port = htons(FBD_CLUSTER_UDP_PORT_SM);

		msg.msg_name = &dAddr;
		msg.msg_namelen = sizeof(dAddr);
	}
	else
	{
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}

printk(" port: %d \n",FBD_CLUSTER_UDP_PORT_SM);

	/* block the signals interrupting  the  I/O transmission */
	siginitsetinv(&blocked, sigmask(SIGKILL));
	sigprocmask(SIG_SETMASK, &blocked, &oldset);

	current->flags |= PF_MEMALLOC;
	do {
		/* init the  message headr to vec[0] */
//		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
		sock->sk->sk_allocation = GFP_NOIO;
		
		iov[0].iov_base = (void *)sm_msg;
		iov[0].iov_len = sizeof(fdsp_msg_t);
		iov[1].iov_base = buf;
		iov[1].iov_len = size;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = msg_flags | MSG_NOSIGNAL;

		if (fbd->xmit_timeout) {
			init_timer(&ti);
			ti.function = fbd_xmit_timeout;
			ti.data = (unsigned long)current;
			ti.expires = jiffies + fbd->xmit_timeout;
			add_timer(&ti);
		}

		result = kernel_sendmsg(sock, &msg, iov, 2, size+sizeof(fdsp_msg_t));

		if (fbd->xmit_timeout)
			del_timer_sync(&ti);

		if (signal_pending(current)) {
			siginfo_t info;
			printk(KERN_WARNING "fbd (pid %d: %s) got signal %d\n",
				task_pid_nr(current), current->comm,
				dequeue_signal_lock(current, &current->blocked, &info));
			result = -EINTR;
			sock_shutdown(fbd, !send);
			break;
		}

		if (result <= 0) {
			if (result == 0)
				result = -EPIPE; /* short read */
			break;
		}
		size -= result;
		buf += result;
	} while (size > 0);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
//	tsk_restore_flags(current, pflags, PF_MEMALLOC);
	current->flags &= ~PF_MEMALLOC;
	current->flags |=  pflags & PF_MEMALLOC;

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
//	psm_msg->src_ip_addr[4]
//	psm_msg->dst_ip_addr[4]

//	psm_msg->result;
//	psm_msg->err_msg;
//	psm_msg->err_code;


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
//	pdm_msg->src_ip_addr[4]
//	pdm_msg->dst_ip_addr[4]

//	pdm_msg->result;
//	pdm_msg->err_msg;
//	pdm_msg->err_code;
	pdm_msg->payload.update_catalog.dm_transaction_id = 0;
	pdm_msg->payload.update_catalog.dm_operation = FDS_DMGR_CMD_OPEN_TXN;

	return 0;
}


static int fbd_process_read_request(struct request *req)
{
	int rc, result = 0;
	struct fbd_device *fbd;
	struct bio_vec *bv;
	struct req_iterator iter;
   	int dir = rq_data_dir(req);
  	int n_segments = 0;
	int flag;
	fdsp_msg_t   *sm_msg;
	uint32_t     trans_id;

	struct  DOID {
	u64	doid;
	u64	doid1;
	}doid;

	struct DOID 	*doid_list1;
	struct DOID 	**doid_list;

	fbd = req->rq_disk->private_data;

	rq_for_each_segment(bv, req, iter)	
	{
		flag = 0;

		switch (dir)
		{
		 case READ:
			doid_list1 = &doid;
			doid_list = &doid_list1;
  			rc = vvc_entry_get(fbd->vhdl, fbd->blk_name, &n_segments,doid_list);
			if (rc)
			{
				printk("Error reading the VVC catalog  Error code : %d\n", rc);
			}

			sm_msg = kzalloc(sizeof(*sm_msg), GFP_KERNEL);
			if (!sm_msg)
			{
				printk("Error Read: Allocating the memory for sm message header \n");
				return -ENOMEM;
			}


			fds_init_sm_hdr(sm_msg);
			sm_msg->msg_code = FDSP_MSG_GET_OBJ_REQ;
			sm_msg->msg_id =  1;
			memcpy((void *)&(sm_msg->payload.put_obj.data_obj_id), (void *)&doid, sizeof(doid));
			sm_msg->payload.put_obj.data_obj_len = bv->bv_len;
			sm_msg->payload_len = bv->bv_len;
			sm_msg->payload.put_obj.volume_offset = bv->bv_offset;
			printk("Read Req len: %d  offset: %d  flag:%d sm_msg:%p \
				doid:%llx:%llx",bv->bv_len, bv->bv_offset, flag,sm_msg,doid.doid, doid.doid1);
			/* open transaction  */
			trans_id = get_trans_id();

			rwlog_tbl[trans_id].trans_state = FDS_TRANS_OPEN;
			rwlog_tbl[trans_id].fbd_ptr = (void *)fbd;
			rwlog_tbl[trans_id].write_ctx = (void *)req;
			rwlog_tbl[trans_id].sm_msg = (void *)sm_msg; 
			rwlog_tbl[trans_id].dm_msg = NULL;
			rwlog_tbl[trans_id].sm_ack_cnt = 0;
			rwlog_tbl[trans_id].dm_ack_cnt = 0;

			sm_msg->req_cookie = trans_id;

			mutex_lock(&fbd->tx_lock);

			result = send_msg_sm(fbd, 1, sm_msg, bv->bv_len,flag);
			if ( result < 0)
			{
				printk("  SM:Error %d: Error  sending the data \n ",result);
			}
			break;
		 case WRITE:
			printk(" Error: command mis-match. Not expecting  write  in Read thread \n");
			goto End;
			
		 default:
			printk(" Unknown  command \n");
			goto End;
		}

	}


End:
	printk("\n");
	return result;
}

static int fbd_process_queue_buffers(struct request *req)
{
	int result = 0;
	int flag;
	struct fbd_device *fbd;
	void *kaddr = NULL;
                 
	struct bio_vec *bv;
	struct req_iterator iter;
	fdsp_msg_t   *sm_msg;
	fdsp_msg_t   *dm_msg;
	uint32_t     trans_id;
                           
	sector_t sector_offset = 0;
   	int dir = rq_data_dir(req);
	int sectors;
//	u64	doid;

	struct  DOID {
	u64	doid;
	u64	doid1;
	}doid;

                                        
	int ret = 0;
	fbd = req->rq_disk->private_data;
	
	rq_for_each_segment(bv, req, iter)	
	{
		if (bv->bv_len % 512 != 0)
		{
			printk(" block size %d is not multiple of  sector size %d ",bv->bv_len,512); 
			return - EIO;
		}
		sectors = bv->bv_len / 512;
		
		flag = 0;
#if 0
		/* if you want to  send the all theblokc at once  set this flag  */
		if (!rq_iter_last(req, iter))
			flag = MSG_MORE;
#endif

		switch (dir)
		{

		 case WRITE:

			kaddr = kmap(bv->bv_page);

			/* get the DOID */

			MurmurHash3_x64_128 ( (kaddr + bv->bv_offset), bv->bv_len,0,&doid );

			sm_msg = kzalloc(sizeof(*sm_msg), GFP_KERNEL);
			if (!sm_msg)
			{
				printk(" Error Allocating the memory for sm message header \n");
				return -ENOMEM;
			}

			dm_msg = kzalloc(sizeof(*sm_msg), GFP_KERNEL);
			if (!dm_msg)
			{
				printk(" Error Allocating the memory for dm message header \n");
				return -ENOMEM;
			}
			
			fds_init_sm_hdr(sm_msg);
			memcpy(sm_msg->src_ip_addr, &fbd->src_ip_addr, 4); 
			memcpy(sm_msg->src_ip_addr, &fbd->udp_destAddr, 4);
			memcpy((void *)&(sm_msg->payload.put_obj.data_obj_id), (void *)&doid, sizeof(doid));
			sm_msg->payload.put_obj.data_obj_len = bv->bv_len;
			sm_msg->payload.put_obj.volume_offset = bv->bv_offset;

			memcpy(sm_msg->src_ip_addr, &fbd->src_ip_addr, 4); 
			memcpy(sm_msg->src_ip_addr, &fbd->udp_destAddr, 4);
			fds_init_dm_hdr(dm_msg);
			dm_msg->payload_len = bv->bv_len;
			dm_msg->payload.update_catalog.volume_offset = bv->bv_offset;
			memcpy((void *)&(dm_msg->payload.update_catalog.data_obj_id), (void *)&doid, sizeof(doid));

			trans_id = get_trans_id();

printk("Write Req len: %d  offset: %d  flag:%d sock_buf:%p t_id: %d doid:%llx:%llx",bv->bv_len, bv->bv_offset, flag,kaddr,trans_id,doid.doid, doid.doid1);
			/* open transaction  */

			rwlog_tbl[trans_id].trans_state = FDS_TRANS_OPEN;
			rwlog_tbl[trans_id].write_ctx = (void *)req;
printk(" write ctx: %p: %p \n ",rwlog_tbl[trans_id].write_ctx, req);
			rwlog_tbl[trans_id].sm_msg = (void *)sm_msg; 
			rwlog_tbl[trans_id].dm_msg = (void *)dm_msg;
			rwlog_tbl[trans_id].sm_ack_cnt = 0;
			rwlog_tbl[trans_id].dm_ack_cnt = 0;

			sm_msg->req_cookie = trans_id;
			dm_msg->req_cookie = trans_id;

			mutex_lock(&fbd->tx_lock);
			result = send_data_sm(fbd, 1, kaddr + bv->bv_offset, bv->bv_len,flag, sm_msg);
			if ( result < 0)
			{
				printk("  SM:Error %d: Error  sending the data \n ",result);
			}

			result = send_data_dm(fbd, 1, dm_msg, sizeof(fdsp_msg_t),flag);
			if ( result < 0)
			{
				printk(" DM:Error %d: Error  sending the data \n ",result);
			}

			mutex_unlock(&fbd->tx_lock);
			kunmap(bv->bv_page);

			break;
		case READ:
			printk(" Error: command mis-match. Not expecting  read  in Write thread \n");
			goto End;
			
		default:
			printk(" Unknown  command \n");
			goto End;

		}
		sector_offset += sectors;
	}
  
End:
	printk("\n");
	return ret;
}



static  int fbd_process_io_cmds (struct fbd_device *fbd, struct request *req)
{
	int result = 0;
	int loc_cmd = 0;

	if (req->cmd_flags &  REQ_FLUSH)
	{
		printk(" FLUHS flag is  set, execute the flush command \n");
		(req)->cmd[0] = FBD_CMD_FLUSH;

	}

	if (rq_data_dir(req) == WRITE) {
		if ((req->cmd_flags & REQ_DISCARD)) {
       		(req)->cmd[0] = FBD_CMD_DISCARD;
			loc_cmd = FBD_CMD_DISCARD;
         }
		else {
       		(req)->cmd[0] = FBD_CMD_WRITE;
			loc_cmd = FBD_CMD_WRITE;
		}

    }
	else {
		if(rq_data_dir(req) == READ)
		loc_cmd = FBD_CMD_READ; 
	}

	fbd->active_req = req;

	switch(loc_cmd)
	{
	case FBD_CMD_FLUSH:
		blk_queue_flush(fbd->disk->queue, REQ_FLUSH);
		__blk_end_request_all(req, 0);
		break;

	case FBD_CMD_WRITE:
		result = fbd_process_queue_buffers(req);
//		__blk_end_request_all(req, 0); /* response will get out in rx thread */ 
		break;

	case FBD_CMD_READ:
		printk(" Read request  from the block device \n");
		fbd_process_read_request(req);
		__blk_end_request_all(req, 0);
		break;

	default:
		printk("Error %d: unknown Command  Drop  the IO\n",req->errors);
		req->errors++;
        fbd_end_request(req);
		result = -EIO;
		break;

	}

	fbd->active_req = NULL;
	wake_up_all(&fbd->active_wq);
	return  result;

}


/*
 * Basic thread for processing the IO request 
 */
static int fbd_io_thread(void *data)
{
    struct fbd_device *fbd = data;
    struct request *req;

    set_user_nice(current, -20);
    while (!kthread_should_stop() || !list_empty(&fbd->waiting_queue)) {
        /* wait for something to do */
        wait_event_interruptible(fbd->waiting_wq,
                     kthread_should_stop() ||
                     !list_empty(&fbd->waiting_queue));

        /* extract request */
        if (list_empty(&fbd->waiting_queue))
            continue;

        spin_lock_irq(&fbd->queue_lock);
        req = list_entry(fbd->waiting_queue.next, struct request,
                 queuelist);
        list_del_init(&req->queuelist);
        spin_unlock_irq(&fbd->queue_lock);

        /* handle request */
        fbd_process_io_cmds(fbd, req);
    }
    return 0;
}

static void fbd_process_queue(struct request_queue *q)
{
	struct request *req;
    struct fbd_device *fbd;
	
	while ((req = blk_fetch_request(q)) != NULL) {

//		printk( "%s: request %p: dequeued (flags=%x)\n",
//				req->rq_disk->disk_name, req, req->cmd_type);

		fbd = req->rq_disk->private_data;
		if (req->cmd_type != REQ_TYPE_FS) {
			printk (KERN_NOTICE " Non valid  command request is skipped \n");
			__blk_end_request_all(req, 0);
			continue;
		}

		if (fbd->sock == NULL)
		{
			printk (KERN_NOTICE "Error SM-DM: Socket is not open yet \n");
			__blk_end_request_all(req, 0);
			continue;
		}


		if ((rq_data_dir(req) == WRITE) &&  (fbd->flags & FBD_READ_ONLY))
		 {
			printk("Error: writting read only disk \n");
			__blk_end_request_all(req, -EROFS);
			continue;
         }

		/* for now drop all read */

		if ((rq_data_dir(req) == READ))
		{
			__blk_end_request_all(req, 0);
			continue;

		}
		
		 spin_unlock_irq(q->queue_lock);

		/*
		 * queue  the padckets to the  waiting queue 		
		 */

		spin_lock_irq(&fbd->queue_lock);
		/* we can  maintain per dev queue  for high performance and consistency  */
                list_add_tail(&req->queuelist, &fbd_dev->waiting_queue);
                spin_unlock_irq(&fbd->queue_lock);

                wake_up(&fbd->waiting_wq);
				spin_lock_irq(q->queue_lock);


	}
}

static int __fbd_dev_ioctl(struct block_device *bdev, struct fbd_device *fbd,
		       unsigned int cmd, unsigned long data)
{
	/* just cleanup the queues  for now and  ack */

	switch(cmd) 
	{
	case FBD_OPEN_TARGET_CON_TCP:
		fbd->proto_type = FBD_PROTO_TCP;
		fbd_process_cluster_conn( bdev, fbd, data);
		return 0;
	case FBD_OPEN_TARGET_CON_UDP:
		fbd->proto_type = FBD_PROTO_UDP;
		fbd_process_cluster_conn( bdev, fbd, data);
		return 0;
	case FBD_CLOSE_TARGET_CON:
		fbd_process_cluster_dconn( bdev, fbd, data);
		break;
	case FBD_SET_TGT_SIZE:
		fbd_set_tgt_size(bdev, fbd, data);
		return 0;
	case FBD_SET_TGT_BLK_SIZE:
		fbd_set_tgt_blksize(bdev, fbd, data);
		break;
	case FBD_READ_VVC_CATALOG:
		fbd_read_vvc_catalog(bdev, fbd, data);
		break;
	case FBD_READ_DMT_TBL:
		fbd_read_dmt_tbl(bdev, fbd, data);
		break;
	case FBD_READ_DLT_TBL:
		fbd_read_dlt_tbl(bdev, fbd, data);
		break;
	case FBD_WRITE_DATA:
		break;
	case FBD_READ_DATA:
		break;
	case FBD_ADD_DEV:
		break;
	case  FBD_DEL_DEV:
		break;
	default :
		return -ENOTTY;
	}

	return 0;
}


static int fbd_dev_ioctl(struct block_device *bdev, fmode_t mode,
		     unsigned int cmd, unsigned long arg)
{
	struct fbd_device *fbd = bdev->bd_disk->private_data;
	int error;


	mutex_lock(&fbd->tx_lock);
//	spin_lock_irq(&fbd->queue_lock);
	error = __fbd_dev_ioctl(bdev, fbd, cmd, arg);
//	spin_unlock_irq(&fbd->queue_lock);
	mutex_unlock(&fbd->tx_lock);

	return error;
}

static const struct block_device_operations fbd_dev_ops =
{
	.owner =	THIS_MODULE,
	.ioctl =	fbd_dev_ioctl,
};


/* 
  sys fs interface for  fbd block device 
*/
static void fbd_root_dev_release(struct device *dev)
{
}

static struct device fbd_root_dev = {
	.init_name =    "fbd",
	.release =      fbd_root_dev_release,
};
static struct bus_type fbd_bus_type = {
	.name		= "fbd",
};

static void fbd_dev_release(struct device *dev)
{

#if 0
	struct fbd_device *fbd_dev =
			container_of(dev, struct fbd_device, dev);

/* 
	all the resource will be released  in fbd_cleanup. we can 
        have clean upo done  on both this routine and  fbd_cleanup, kernel
        kernel will crash
*/

	struct gendisk *disk = fbd_dev->disk;
	if (disk) 
	{
		del_gendisk(disk);
		blk_cleanup_queue(disk->queue);
		put_disk(disk);
	}
	unregister_blkdev(FBD_DEV_MAJOR_NUM, "fbd");
	if(fbd_dev)
		kfree(fbd_dev);
 	if(fbd_con)
		kfree(fbd_con);

	/* release module ref */
	module_put(THIS_MODULE);
#endif
}

/* 
	sys fs functions  for  adding and removing the fbd block devices 
*/
static ssize_t fbd_dev_add(struct bus_type *bus, const char *buf, size_t count)
{

	return 0;
}

static ssize_t fbd_dev_remove(struct bus_type *bus, const char *buf, size_t count)
{

	return 0;
}
static struct bus_attribute fbd_bus_attrs[] = {
	__ATTR(add, S_IWUSR, NULL, fbd_dev_add),
	__ATTR(remove, S_IWUSR, NULL, fbd_dev_remove),
	__ATTR_NULL
};


static int fbd_sysfs_init(void)
{
	int ret;

	fbd_bus_type.bus_attrs = fbd_bus_attrs;

	ret = bus_register(&fbd_bus_type);
	 if (ret < 0)
		return ret;

	ret = device_register(&fbd_root_dev);

	return ret;
}

static void fbd_sysfs_cleanup(void)
{
	device_unregister(&fbd_root_dev);
	bus_unregister(&fbd_bus_type);
}


static int fbd_bus_add_device(struct fbd_device *fbd_dev)
{
	int ret = -ENOMEM;
	struct device *dev;
	mutex_lock_nested(&ctl_mutex, SINGLE_DEPTH_NESTING);
	dev = &fbd_dev->dev;

	dev->bus = &fbd_bus_type;
	dev->type = &fbd_device_type;
	dev->parent = &fbd_root_dev;
	dev->release = fbd_dev_release;
	dev_set_name(dev, "%d", fbd_dev->dev_id);
	ret = device_register(dev);
	if (ret < 0)
	{
		goto done_free;
 	}	

	mutex_unlock(&ctl_mutex);
	return 0;

done_free:
	mutex_unlock(&ctl_mutex);
	return ret;
}

static int __init fbd_init(void)
{
	int err = -ENOMEM;
	struct gendisk *disk;
	int rc;

	rc = fbd_sysfs_init();
	if (rc)
	{	
		printk(" Error: Creating the sysfs interface  for fbd  block dev code: %x \n",rc);
		return rc;
	}


	init_fbdtbl(); /* socket transport structures */
	fbd_dev = kzalloc(sizeof(*fbd_dev), GFP_KERNEL);
	if (!fbd_dev)
		return -ENOMEM;

	
	rc = fbd_bus_add_device(fbd_dev);
	if (rc)
	{
		printk(" Error: Adding bus to the  block device failed %x \n",rc);
		kfree(fbd_dev);
		kfree(fbd_con);
		return rc;
	}

	disk = alloc_disk(FBD_MINORS_PER_MAJOR);
	if (!disk)
		goto out;
	fbd_dev->disk = disk;
	disk->queue = blk_init_queue(fbd_process_queue, &fbd_lock);
	if (!disk->queue) {
		printk(" Error: Initializing the  block disk  queues \n");
		put_disk(disk);
		goto out;
	}

	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, disk->queue);
	disk->queue->limits.discard_granularity = 512;
	disk->queue->limits.max_discard_sectors = UINT_MAX;
	disk->queue->limits.discard_zeroes_data = 0;

	if (register_blkdev(FBD_DEV_MAJOR_NUM, "fbd")) {
		err = -EIO;
		goto out;
	}

	printk("fbd: Registered  Formation  Data System  device at major %d\n", FBD_DEV_MAJOR_NUM);


	INIT_LIST_HEAD(&fbd_dev->waiting_queue);
	spin_lock_init(&fbd_dev->queue_lock);
	INIT_LIST_HEAD(&fbd_dev->queue_head);
	mutex_init(&fbd_dev->tx_lock);

	init_waitqueue_head(&fbd_dev->active_wq);
	init_waitqueue_head(&fbd_dev->waiting_wq);

	fbd_dev->blocksize = 4096; /* default value  */
	fbd_dev->bytesize = 0;
	/* we will have to  generate the dev id for multiple device support */
	fbd_dev->dev_id = 0;
	fbd_dev->dev_major = FBD_DEV_MAJOR_NUM;

	disk->major = FBD_DEV_MAJOR_NUM;
	disk->first_minor = 0;
	disk->fops = &fbd_dev_ops;
	disk->private_data = fbd_dev;
	sprintf(disk->disk_name, "fbd%d",disk->first_minor);
	set_capacity(disk, 0);
	add_disk(disk);

	/* vvc vol create */
	fbd_dev->vol_id = 1; /*  this should be comming from OM */
	if((fbd_dev->vhdl = vvc_vol_create(fbd_dev->vol_id, NULL,1024)) == 0 )
	{
		printk(" Error: creating the  vvc  volume \n");
		goto out;	
	}

	/* init  DMT and DLT tables */
	fds_init_dmt();
	fds_init_dlt();
	/* sample  code to populate the DMT and DLT tables  */
	populate_dmt_dlt_tbl();

	/* init the blk name  testing only */
	strncpy(fbd_dev->blk_name,"fds.txt",7);
	fbd_dev->src_ip_addr =  0xc0a80102;

	/* the read write trans  table */
	fds_init_trans_log();
		
	mutex_unlock(&fbd_dev->tx_lock);
	fbd_thread_id = kthread_create(fbd_io_thread, fbd_dev, fbd_dev->disk->disk_name);
	if (IS_ERR(fbd_thread_id)) {
		printk(" Error: creating the  IO thread \n");
		goto out;	
	}
	wake_up_process(fbd_thread_id);

	fbd_thread_id_rx = kthread_create(fds_rx_io_proc, fbd_dev, fbd_dev->disk->disk_name);
	if (IS_ERR(fbd_thread_id_rx)) {
		printk(" Error: creating the   RX IO thread \n");
		goto out;	
	}
	wake_up_process(fbd_thread_id_rx);

	return 0;
out:
	blk_cleanup_queue(fbd_dev->disk->queue);
	put_disk(fbd_dev->disk);
	kfree(fbd_dev);
	kfree(fbd_con);
	return err;
}

static void __exit fbd_cleanup(void)
{
	struct gendisk *disk = fbd_dev->disk;
	if (disk != NULL) 
	{
		del_gendisk(disk);
		blk_cleanup_queue(disk->queue);
		put_disk(disk);
	}
	kthread_stop(fbd_thread_id);
	if(fbd_thread_id_rx)
        {
		atomic_set(&fds_exit, 1);
		wake_up_interruptible(&thread_wait);
		kthread_stop(fbd_thread_id_rx);

	}
	device_unregister(&fbd_dev->dev);
	fbd_sysfs_cleanup();
	unregister_blkdev(FBD_DEV_MAJOR_NUM, "fbd");
	kfree(fbd_dev);
	kfree(fbd_con);
	printk("fbd: Removing the  device  Major Number %d\n", FBD_DEV_MAJOR_NUM);
}


module_init(fbd_init);
module_exit(fbd_cleanup);

MODULE_DESCRIPTION("Formation Data  System Network Block Device");
MODULE_LICENSE("GPL");

