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
#include <linux/list.h>

#include <asm/uaccess.h>
#include <asm/types.h>
// #include "vvclib.h"
//#include "../include/fds_commons.h"
//#include "../include/fdsp.h"
//#include "../include/data_mgr.h"
#include "blktap.h"
#include "fbd.h"
#include "fds.h"
//#include "fbd_hash.h"


static DEFINE_SPINLOCK(fbd_lock);
struct fbd_device *fbd_dev;
struct task_struct *fbd_thread_id;
struct task_struct *fbd_thread_id_rx;
static DEFINE_MUTEX(ctl_mutex);

extern  FDS_JOURN rwlog_tbl[];
atomic_t fds_data_ready;
atomic_t fds_exit;
DECLARE_WAIT_QUEUE_HEAD(thread_wait);


void blktap_device_fail_queue(struct fbd_device *tap);

/*
  Initialise the connection  management table 
*/

#if 0

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
		  fds_process_rx_message(rx_buf, ntohl(cAddr.sin_addr.s_addr));

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

#endif


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

static int fbd_set_base_port(struct block_device *bdev, struct fbd_device *fbd, int data) {
  fbd->stor_mgr_port = data;
  fbd->data_mgr_port = data+1;
  return (0);
}

static int fbd_process_cluster_conn(struct block_device *bdev, struct fbd_device *fbd, int data)
{

printk("fbd_process_cluster_conn:  received:%x fbd:%pfbd_dev:%p proto:%d\n ",data,fbd,fbd_dev,fbd->proto_type);
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

	// fbd_dev->sock = fds_cluster_conn(fbd_con); 
	if (fbd_dev->sock == NULL){
		printk(" Error DM: socket is not created  yet .. \n");
		return -1;
	}

	return 0;

}

#if 0

static int fbd_process_cluster_dconn(struct block_device *bdev, struct fbd_device *fbd, int data)
{
	printk("fbd_process_cluster_dconn:  received \n ");

	/* close  the sockets */
	// close_cluster_con(fbd_con);

	return 0;

}

#endif

static void fbd_end_request(struct request *req)
{
	int error = req->errors ? -EIO : 0;
	struct request_queue *q = req->q;
	unsigned long flags;


	spin_lock_irqsave(q->queue_lock, flags);
	if(req)
	__blk_end_request_all(req, error);
	spin_unlock_irqrestore(q->queue_lock, flags);
}

#if 0

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

#endif

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
	printk(" active  incomming request :%p \n",req);

	switch(loc_cmd)
	{
	case FBD_CMD_FLUSH:
		printk(" flush  command from block \n");
		blk_queue_flush(fbd->disk->queue, REQ_FLUSH);
		if(req)
		__blk_end_request_all(req, 0);
		break;

	case FBD_CMD_WRITE:
	  // result = fbd_process_queue_buffers(req);
		// __blk_end_request_all(req, 0); /* response will get out in rx thread */ 
		break;

	case FBD_CMD_READ:
	  // fbd_process_read_request(req);
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

#if 0

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

#endif



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
		//fbd_process_cluster_dconn( bdev, fbd, data);
		printk("*** FBD: received close target ioctl\n");
		blktap_device_fail_queue(fbd);
		break;
	case FBD_SET_TGT_SIZE:
		fbd_set_tgt_size(bdev, fbd, data);
		return 0;
	case FBD_SET_TGT_BLK_SIZE:
		fbd_set_tgt_blksize(bdev, fbd, data);
		break;
	case FBD_SET_BASE_PORT:
         	fbd_set_base_port(bdev, fbd, data);
		break;
	case FBD_READ_VVC_CATALOG:
	  // fbd_read_vvc_catalog(bdev, fbd, data);
		break;
	case FBD_READ_DMT_TBL:
	  // fbd_read_dmt_tbl(bdev, fbd, data);
		break;
	case FBD_READ_DLT_TBL:
	  // fbd_read_dlt_tbl(bdev, fbd, data);
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

	printk("*** FBD received ioctl cmd %d\n", cmd);

	mutex_lock(&fbd->tx_lock);
	error = __fbd_dev_ioctl(bdev, fbd, cmd, arg);
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

int fbd_device_create(void)
{
	int err = -ENOMEM;
	struct gendisk *disk;
	int rc;

printk("FDS:%s:%d: Initialising the  formation volume \n",__FILE__,__LINE__);
	rc = fbd_sysfs_init();
	if (rc)
	{	
		printk(" Error: Creating the sysfs interface  for fbd  block dev code: %x \n",rc);
		return rc;
	}

	fbd_dev = kzalloc(sizeof(*fbd_dev), GFP_KERNEL);
	if (!fbd_dev)
		return -ENOMEM;

	
	rc = fbd_bus_add_device(fbd_dev);
	if (rc)
	{
		printk(" Error: Adding bus to the  block device failed %x \n",rc);
		kfree(fbd_dev);
		return rc;
	}

	disk = alloc_disk(FBD_MINORS_PER_MAJOR);
	if (!disk)
		goto out;
	fbd_dev->disk = disk;
	fbd_dev->should_stop_accepting_requests = 0;
	disk->queue = blk_init_queue(blktap_device_do_request, &fbd_lock);
	if (!disk->queue) {
		printk(" Error: Initializing the  block disk  queues \n");
		put_disk(disk);
		goto out;
	}
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, disk->queue);
	disk->queue->limits.discard_granularity = 512;
	disk->queue->limits.max_discard_sectors = UINT_MAX;
	disk->queue->limits.discard_zeroes_data = 0;
	disk->queue->queuedata = fbd_dev;

	/* Each segment in a request is up to an aligned page in size. */
	blk_queue_segment_boundary(disk->queue, PAGE_SIZE - 1);
	blk_queue_max_segment_size(disk->queue, PAGE_SIZE);

	/* Ensure a merged request will fit in a single I/O ring slot. */
	blk_queue_max_segments(disk->queue, BLKTAP_SEGMENT_MAX);
	blk_queue_max_segment_size(disk->queue, PAGE_SIZE);


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

	/* init the blk ring  */
	blktap_ring_create(fbd_dev);
#if 0		
	fbd_thread_id = kthread_create(fbd_io_thread, fbd_dev, fbd_dev->disk->disk_name);
	if (IS_ERR(fbd_thread_id)) {
		printk(" Error: creating the  IO thread \n");
		goto out;	
	}
	wake_up_process(fbd_thread_id);
#endif

	return 0;
out:
	blk_cleanup_queue(fbd_dev->disk->queue);
	put_disk(fbd_dev->disk);
	kfree(fbd_dev);
	return err;
}

static int __init fbd_init(void)
{
  fbd_dev = NULL;

  if (register_blkdev(FBD_DEV_MAJOR_NUM, "fbd")) {
    return -EIO;
  }
  printk("fbd: Registered  Formation  Data System  device at major %d\n", FBD_DEV_MAJOR_NUM);

  return blktap_init();

}

static void __exit fbd_cleanup(void)
{
  struct gendisk *disk;

  printk("*** FDS: Uninstall Begin\n");

  if (fbd_dev) {

    printk("FDS: Cleaning up queue and disk associated with fbd device\n");

    disk = fbd_dev->disk;
    if (disk != NULL) 
      {
	del_gendisk(disk);
	blk_cleanup_queue(disk->queue);
	put_disk(disk);
      }

#if 0

    kthread_stop(fbd_thread_id);
    if(fbd_thread_id_rx)
      {
	atomic_set(&fds_exit, 1);
	wake_up_interruptible(&thread_wait);
	kthread_stop(fbd_thread_id_rx);
	
      }
#endif

    device_unregister(&fbd_dev->dev);

  } else {
    printk("FDS: No fbd_dev instantiated\n");
  }

  fbd_sysfs_cleanup();
  printk("FDS: sysfs cleanup complete\n");

  unregister_blkdev(FBD_DEV_MAJOR_NUM, "fbd");
  printk(" unresgister blkdev \n");
  blktap_exit();
  printk(" blktap exit \n");
  if (fbd_dev) {
    kfree(fbd_dev);
    printk("Freed fbd_dev\n");
  } 

  printk("fbd: Removing the  device  Major Number %d\n", FBD_DEV_MAJOR_NUM);
}


module_init(fbd_init);
module_exit(fbd_cleanup);

MODULE_DESCRIPTION("Formation Data  System Network Block Device");
MODULE_LICENSE("GPL");

