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
#include "fds.h"
#include "fbd.h"
#include "fbd_hash.h"


static DEFINE_SPINLOCK(fbd_lock);
static struct fbd_device *fbd_dev;
struct task_struct *fbd_thread_id;
static DEFINE_MUTEX(ctl_mutex);

struct  fbd_contbl *fbd_con;



/*
  hash functions 
*/

inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
 
  return (x << r) | (x >> (32 - r));
 
}
 
 
inline uint64_t rotl64 ( uint64_t x, int8_t r )
{
 
  return (x << r) | (x >> (64 - r));
 
}
 
#define BIG_CONSTANT(x) (x##LLU)
 
/* 
  Block read - if your platform needs to do endian-swapping or can only
 handle aligned reads, do the conversion here
*/
 
inline uint32_t getblock_32 ( const uint32_t * p, int i )
{
	return p[i];
}
 
inline uint64_t getblock_64 ( const uint64_t * p, int i )
{
	return p[i];
}
 
/*
 Finalization mix - force all bits of a hash block to avalanche
*/
 
inline uint32_t fmix_32 ( uint32_t h )
{
 
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	 
return h;
}
 
 
inline uint64_t fmix_64 ( uint64_t k )
{
 
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xff51afd7ed558ccd);
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
	k ^= k >> 33;

 return k;
}
 

 
void MurmurHash3_x86_32 (const void *key, int len, 
                          uint32_t seed, void *out )
{
	const uint8_t *data = (const uint8_t*)key;
	const int nblocks = len / 4;
	uint32_t h1 = seed;
	uint32_t c1 = 0xcc9e2d51;
	uint32_t c2 = 0x1b873593;
	int i=0;
	uint32_t k1 = 0;
	const uint8_t *tail;
	const uint32_t *blocks = (const uint32_t *)(data + nblocks*4);
 
	for(i = -nblocks; i; i++)
	{
		uint32_t k1;

		k1  = getblock_32(blocks,i);
		k1 *= c1;
		k1 = rotl32(k1,15);
		k1 *= c2;
		h1 ^= k1;
		h1 = rotl32(h1,13); 
		h1 = h1*5+0xe6546b64;
	}	
 
	tail = (const uint8_t*)(data + nblocks*4);

	switch(len & 3)
	{	
	case 3:
		 k1 ^= tail[2] << 16;
	case 2:
		k1 ^= tail[1] << 8;
	case 1:
		 k1 ^= tail[0];
		k1 *= c1; k1 = rotl32(k1,15); k1 *= c2; h1 ^= k1;
	};

	h1 ^= len;
	h1 = fmix_32(h1);
	*(uint32_t*)out = h1;
} 
 

void MurmurHash3_x86_128 ( const void *key, const int len, 
                           uint32_t seed, void *out )
{
	const uint8_t * data = (const uint8_t*)key;
	const int nblocks = len / 16;
	int i =0;
 
	uint32_t h1 = seed;
	uint32_t h2 = seed;
	uint32_t h3 = seed;
	uint32_t h4 = seed;

 
	uint32_t c1 = 0x239b961b; 
	uint32_t c2 = 0xab0e9789;
	uint32_t c3 = 0x38b34ae5; 
	uint32_t c4 = 0xa1e38b93;
	uint32_t k1 = 0;
	uint32_t k2 = 0;
	uint32_t k3 = 0;
	uint32_t k4 = 0;
 
	const uint8_t *tail;
	const uint32_t *blocks = (const uint32_t *)(data + nblocks*16);
 
	for(i = -nblocks; i; i++)
	{
		uint32_t k1 = getblock_32(blocks,i*4+0);
		uint32_t k2 = getblock_32(blocks,i*4+1);
		uint32_t k3 = getblock_32(blocks,i*4+2);
		uint32_t k4 = getblock_32(blocks,i*4+3);

		k1 *= c1; k1  = rotl32(k1,15); k1 *= c2; h1 ^= k1;
		h1 = rotl32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
		k2 *= c2; k2  = rotl32(k2,16); k2 *= c3; h2 ^= k2;
		h2 = rotl32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
		k3 *= c3; k3  = rotl32(k3,17); k3 *= c4; h3 ^= k3;
		h3 = rotl32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
		k4 *= c4; k4  = rotl32(k4,18); k4 *= c1; h4 ^= k4;
		h4 = rotl32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
	}
 
	tail = (const uint8_t*)(data + nblocks*16);
 
	switch(len & 15)
	{
		case 15:
			k4 ^= tail[14] << 16;
		case 14:
			k4 ^= tail[13] << 8;
		case 13:
			k4 ^= tail[12] << 0;
			k4 *= c4; k4  = rotl32(k4,18); k4 *= c1; h4 ^= k4;
 
		case 12:
			k3 ^= tail[11] << 24;
		case 11:
			k3 ^= tail[10] << 16;
		case 10:
			k3 ^= tail[ 9] << 8;
		case  9:
			k3 ^= tail[ 8] << 0;
			k3 *= c3; k3  = rotl32(k3,17); k3 *= c4; h3 ^= k3;
		case  8:
			 k2 ^= tail[ 7] << 24;
		case  7:
		 	k2 ^= tail[ 6] << 16;
		case  6:
			k2 ^= tail[ 5] << 8;
		case  5:
			k2 ^= tail[ 4] << 0;
			k2 *= c2; k2  = rotl32(k2,16); k2 *= c3; h2 ^= k2;
		case  4:
			k1 ^= tail[ 3] << 24;
		case  3:
			k1 ^= tail[ 2] << 16;
		case  2:
			k1 ^= tail[ 1] << 8;
		case  1:k1 ^= tail[ 0] << 0;
			k1 *= c1; k1  = rotl32(k1,15); k1 *= c2; h1 ^= k1;
 
	};

	h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;

	h1 = fmix_32(h1);
	h2 = fmix_32(h2);
	h3 = fmix_32(h3);
	h4 = fmix_32(h4);
 
	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;
 
	((uint32_t*)out)[0] = h1;
	((uint32_t*)out)[1] = h2;
	((uint32_t*)out)[2] = h3;
	((uint32_t*)out)[3] = h4;
}
 

void MurmurHash3_x64_128 ( const void *key, const int len,
                           const uint32_t seed, void *out )
{
	const uint8_t *data = (const uint8_t*)key;
	const int nblocks = len / 16;
	int i = 0;

	uint64_t h1 = seed;
	uint64_t h2 = seed;
	uint64_t k1 = 0;
	uint64_t k2 = 0;
 
	uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
	uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);
 
	const uint8_t *tail;
	const uint64_t *blocks = (const uint64_t *)(data);
 
	for(i = 0; i < nblocks; i++)
	{
		uint64_t k1 = getblock_64(blocks,i*2+0);
		uint64_t k2 = getblock_64(blocks,i*2+1);

		k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
		h1 = rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
		k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;
		h2 = rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
	}
 
	tail = (const uint8_t*)(data + nblocks*16);
 
	switch(len & 15)
	{
	case 15:
		k2 ^= (uint64_t)(tail[14]) << 48;
	case 14:
		k2 ^= (uint64_t)(tail[13]) << 40;
	case 13:
		k2 ^= (uint64_t)(tail[12]) << 32;
	case 12:
		k2 ^= (uint64_t)(tail[11]) << 24;
	case 11:
		k2 ^= (uint64_t)(tail[10]) << 16;
	case 10:
		k2 ^= (uint64_t)(tail[ 9]) << 8;
	case  9:
		k2 ^= (uint64_t)(tail[ 8]) << 0;
		k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;
	case  8:
		k1 ^= (uint64_t)(tail[ 7]) << 56;
	case  7:
		k1 ^= (uint64_t)(tail[ 6]) << 48;
	case  6:
		k1 ^= (uint64_t)(tail[ 5]) << 40;
	case  5:
		k1 ^= (uint64_t)(tail[ 4]) << 32;
	case  4:
		k1 ^= (uint64_t)(tail[ 3]) << 24;
	case  3:
		k1 ^= (uint64_t)(tail[ 2]) << 16;
	case  2:
		k1 ^= (uint64_t)(tail[ 1]) << 8;
	case  1:
		k1 ^= (uint64_t)(tail[ 0]) << 0;
		k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
	};
 
	h1 ^= len; h2 ^= len;
	h1 += h2;
	h2 += h1;
 
	h1 = fmix_64(h1);
	h2 = fmix_64(h2);

	h1 += h2;
	h2 += h1;
 
	((uint64_t*)out)[0] = h1;
	((uint64_t*)out)[1] = h2;
}
 

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

static  void  sock_read_ready(struct sock *sk)
{


}

static  void  sock_write_ready(struct sock *sk)
{


}

static  void  sock_process_state(struct sock *sk)
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

static void set_sock_callbacks(struct socket *sock, struct fbd_contbl *con)
{
	struct sock *sk = sock->sk;
	sk->sk_user_data = (void *)con;
	sk->sk_data_ready = sock_read_ready;
	sk->sk_write_space = sock_write_ready;
	sk->sk_state_change = sock_process_state;
}

/*
 open the connection with fds cluster 
*/
static struct socket *fds_cluster_conn(struct fbd_contbl *pCon)
{
        struct socket *sock;
	struct sockaddr_in *dAddr;
        int ret = 0;

	/* allocate the  common  socket  address buffer for tcp/udp */
	dAddr  = kmalloc( sizeof ( struct sockaddr_in ), GFP_KERNEL);
	if (dAddr == NULL)
		printk(" Error : Allocating the memory for socket address structure \n");
	memset(dAddr,0, sizeof(struct sockaddr_in));

	if ( fbd_dev->proto_type == FBD_PROTO_TCP)
	{
printk(" TCP - socket create  \n");
        	ret = sock_create_kern(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
        	if (ret)
		{
			printk("Error: creating the TCP socket  \n");
                	return ERR_PTR(ret);
		}


		dAddr->sin_family = AF_INET;
		dAddr->sin_addr.s_addr = htonl(fbd_dev->tcp_destAddr);
		dAddr->sin_port = htons(FBD_CLUSTER_TCP_PORT);


        	ret = sock->ops->connect(sock, (struct sockaddr *)dAddr, sizeof(*dAddr),
                                 O_NONBLOCK);
	
		if (ret == -EINPROGRESS) 
		{
			printk(" connection  in progress  sk_state = %u\n", sock->sk->sk_state);
			return NULL;
		}

        	if (ret < 0)
		{
                	printk("Error: TCP connect  error %d\n", ret);
			sock_release(sock);
			pCon->sock = NULL;
			fbd_dev->sock = NULL;
			return NULL;
		}
		set_sock_callbacks(sock,pCon);
	}
	else if (fbd_dev->proto_type == FBD_PROTO_UDP)
	{
printk(" UDP - socket create  \n");
       		ret = sock_create_kern(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
       		if (ret < 0)
		{
			printk("Error: creating the UDP socket  \n");
                	return ERR_PTR(ret);
		}

printk(" Successfully created UDP socket \n");
		dAddr->sin_family = AF_INET;
		dAddr->sin_addr.s_addr = htonl(fbd_dev->udp_destAddr);
		dAddr->sin_port = htons(FBD_CLUSTER_UDP_PORT);

        	ret = sock->ops->connect(sock, (struct sockaddr *)dAddr, sizeof(*dAddr),
                                 O_NONBLOCK);
	
//		if (ret == -EINPROGRESS) 
		if (ret < 0) 
		{
			printk(" connection  in progress  sk_state = %u\n", sock->sk->sk_state);
			return NULL;
		}
	}

printk(" Connection successfull \n");
       	pCon->sock = sock;
	fbd_dev->sock = sock;


        if (ret < 0)
	{
		printk(" Error: creating the cluster connection : %d \n", ret);
                return ERR_PTR(ret);
	}
        return sock;
}

/* socket dis connect */
static int close_cluster_con(struct fbd_contbl *con)
{
	int rc;

	printk(" terminating the cluster connectionon %p sock %p\n", con, con->sock);
	if (!con->sock)
		return 0;
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
		printk(" Error: socket is not created  yet .. \n");
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
	if (fbd->sock) {
		dev_warn(disk_to_dev(fbd->disk), "shutting down socket\n");
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


static int send_data(struct fbd_device *fbd, int send, void *buf, int size,
		int msg_flags)
{
	struct socket *sock = fbd_con->sock;
	int result;
	struct msghdr msg;
	struct kvec iov[2];
//	struct kvec iov;
	sigset_t blocked, oldset;
	unsigned long pflags = current->flags;
	fbd->xmit_timeout = 1000;

	if (!sock)
	{
		printk("Trying to send  the  I/O's on closed socket \n");
		return -EINVAL;
	}

	/* block the signals interrupting  the  I/O transmission */
	siginitsetinv(&blocked, sigmask(SIGKILL));
	sigprocmask(SIG_SETMASK, &blocked, &oldset);

	current->flags |= PF_MEMALLOC;
	do {
		/* init the  message headr to vec[0] */
		sock->sk->sk_allocation = GFP_NOIO | __GFP_MEMALLOC;
		
		iov[0].iov_base = fbd->msg;
		iov[0].iov_len = sizeof(FDS_MSG);
		iov[1].iov_base = buf;
		iov[1].iov_len = size;
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = msg_flags | MSG_NOSIGNAL;

		if (send) {
			struct timer_list ti;

			if (fbd->xmit_timeout) {
				init_timer(&ti);
				ti.function = fbd_xmit_timeout;
				ti.data = (unsigned long)current;
				ti.expires = jiffies + fbd->xmit_timeout;
				add_timer(&ti);
			}
			result = kernel_sendmsg(sock, &msg, iov, 2, size+sizeof(FDS_MSG));
//			result = kernel_sendmsg(sock, &msg, &iov, 1, size);
			if (fbd->xmit_timeout)
				del_timer_sync(&ti);
		} else
			result = kernel_recvmsg(sock, &msg, iov, 2, size, msg.msg_flags);
//			result = kernel_recvmsg(sock, &msg, &iov, 1, size, msg.msg_flags);

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
	tsk_restore_flags(current, pflags, PF_MEMALLOC);

	return result;
}

#if 0
static inline int fbd_transmit_bvec(struct fbd_device *fbd, struct bio_vec *bvec, int flag)
{
	int result;
	void *kaddr = kmap(bvec->bv_page);
	mutex_lock(&fbd->tx_lock);
printk("sending len: %d  offset: %d  flag:%d\n",bvec->bv_len, bvec->bv_offset, flag);
	result = send_data(fbd, 1, kaddr + bvec->bv_offset, bvec->bv_len,flag);
	if ( result < 0)
	{
		printk(" Error: Error  sending the data \n ");
	}
	mutex_unlock(&fbd->tx_lock);
	kunmap(bvec->bv_page);
	return result;
}

#endif

static int fbd_process_queue_buffers(struct request *req)
{
	int result = 0;
	int flag;
	struct fbd_device *fbd;
                 
	struct bio_vec *bv;
	struct req_iterator iter;
                           
	sector_t sector_offset = 0;
//	sector_t nsect = blk_rq_sectors(req);
//	sector_t start_sect = blk_rq_pos(req);
   	int dir = rq_data_dir(req);
	int sectors;
//	BASE_DOID	bDoid;
	u64	doid;
                                        
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

		if ( dir == WRITE)
		{

			void *kaddr = kmap(bv->bv_page);

			/* get the DOID */

			MurmurHash3_x64_128 ( (kaddr + bv->bv_offset), bv->bv_len,0,&doid );
			
			memcpy((void *)&fbd->msg->doid.doid, (void *)&doid, sizeof(doid));

			mutex_lock(&fbd->tx_lock);
			printk("sending len: %d  offset: %d  flag:%d sock_buf:%p  doid:%llx\n",bv->bv_len, bv->bv_offset, flag,kaddr,(u64)doid);
			result = send_data(fbd, 1, kaddr + bv->bv_offset, bv->bv_len,flag);
			if ( result < 0)
			{
				printk(" Error: Error  sending the data \n ");
			}
			mutex_unlock(&fbd->tx_lock);
			kunmap(bv->bv_page);

		}
		sector_offset += sectors;
	}
  
	printk("\n\n");
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
		__blk_end_request_all(req, 0);
		break;

	case FBD_CMD_READ:
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
			printk (KERN_NOTICE "Error: Socket is not open yet \n");
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

	/* allocate message buffer */
	fbd_dev->msg = kzalloc(sizeof(FDS_MSG), GFP_KERNEL);
	memset(fbd_dev->msg,0, sizeof(FDS_MSG));
	if (!fbd_dev->msg)
		return -ENOMEM;
	
	rc = fbd_bus_add_device(fbd_dev);
	if (rc)
	{
		printk(" Error: Adding bus to the  block device failed %x \n",rc);
		kfree(fbd_dev->msg);
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
		
	mutex_unlock(&fbd_dev->tx_lock);
	fbd_thread_id = kthread_create(fbd_io_thread, fbd_dev, fbd_dev->disk->disk_name);
	if (IS_ERR(fbd_thread_id)) {
		printk(" Error: creating the  IO thread \n");
		goto out;	
	}
	wake_up_process(fbd_thread_id);

	return 0;
out:
	blk_cleanup_queue(fbd_dev->disk->queue);
	put_disk(fbd_dev->disk);
	kfree(fbd_dev->msg);
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
	device_unregister(&fbd_dev->dev);
	fbd_sysfs_cleanup();
	unregister_blkdev(FBD_DEV_MAJOR_NUM, "fbd");
	kfree(fbd_dev->msg);
	kfree(fbd_dev);
	kfree(fbd_con);
	printk("fbd: Removing the  device  Major Number %d\n", FBD_DEV_MAJOR_NUM);
}


module_init(fbd_init);
module_exit(fbd_cleanup);

MODULE_DESCRIPTION("Formation Data  System Network Block Device");
MODULE_LICENSE("GPL");

