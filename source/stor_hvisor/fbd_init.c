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
// static struct fbd_device *fbd_dev;
extern struct fbd_device **fbd_devices;
struct task_struct *fbd_thread_id;
struct task_struct *fbd_thread_id_rx;
static DEFINE_MUTEX(ctl_mutex);

extern  FDS_JOURN rwlog_tbl[];
atomic_t fds_data_ready;
atomic_t fds_exit;
DECLARE_WAIT_QUEUE_HEAD(thread_wait);


void blktap_device_fail_queue(struct fbd_device *tap);



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

static int __fbd_dev_ioctl(struct block_device *bdev, struct fbd_device *fbd,
		       unsigned int cmd, unsigned long data)
{
	/* just cleanup the queues  for now and  ack */

	switch(cmd) 
	{
	case FBD_OPEN_TARGET_CON_TCP:
		fbd->proto_type = FBD_PROTO_TCP;
		//fbd_process_cluster_conn( bdev, fbd, data);
		return 0;
	case FBD_OPEN_TARGET_CON_UDP:
		fbd->proto_type = FBD_PROTO_UDP;
		//fbd_process_cluster_conn( bdev, fbd, data);
		return 0;
	case FBD_CLOSE_TARGET_CON:
		//fbd_process_cluster_dconn( bdev, fbd, data);
		printk("*** FBD: received close target ioctl\n");
		blktap_device_fail_queue(fbd);
		break;
	case FBD_SET_TGT_SIZE:
	  printk("*** FBD: received set capacity ioctl for device %d with value %lu\n", fbd->dev_id, data);
		fbd_set_tgt_size(bdev, fbd, data);
		return 0;
	case FBD_SET_TGT_BLK_SIZE:
		fbd_set_tgt_blksize(bdev, fbd, data);
		break;
	case FBD_SET_BASE_PORT:
	  //fbd_set_base_port(bdev, fbd, data);
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
	  printk("*** FDS: Unknown ioctl cmd %d for device %d\n", cmd, fbd->dev_id);
		return -ENOTTY;
	}

	return 0;
}


static int fbd_dev_ioctl(struct block_device *bdev, fmode_t mode,
		     unsigned int cmd, unsigned long arg)
{
	struct fbd_device *fbd = bdev->bd_disk->private_data;
	int error;

	printk("*** FBD received ioctl cmd %d for device %d\n", cmd, fbd->dev_id);

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

int fbd_device_create(int minor)
{
	int err = -ENOMEM;
	struct gendisk *disk;
	int rc;
	struct fbd_device *fbd_dev;

	printk("FDS:%s:%d: Allocating fbd device with minor %d \n",__FILE__,__LINE__, minor);

#if 0
	rc = fbd_sysfs_init();
	if (rc)
	{	
		printk(" Error: Creating the sysfs interface  for fbd  block dev code: %x \n",rc);
		return rc;
	}
#endif

	fbd_dev = kzalloc(sizeof(*fbd_dev), GFP_KERNEL);
	if (!fbd_dev)
		return -ENOMEM;

	fbd_dev->blocksize = 4096; /* default value  */
	fbd_dev->bytesize = 0;
	/* we will have to  generate the dev id for multiple device support */
	fbd_dev->dev_id = minor;
	fbd_dev->dev_major = FBD_DEV_MAJOR_NUM;
	
	rc = fbd_bus_add_device(fbd_dev);
	if (rc)
	{
	  printk(" Error: Adding block device %d to the bus failed %x \n", minor, rc);
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

	disk->major = FBD_DEV_MAJOR_NUM;
	disk->first_minor = minor;
	disk->fops = &fbd_dev_ops;
	disk->private_data = fbd_dev;
	sprintf(disk->disk_name, "fbd%d",disk->first_minor);
	set_capacity(disk, 0x0);
	add_disk(disk);

	/* init the blk ring  */
	blktap_ring_create(fbd_dev);

	fbd_devices[fbd_dev->dev_id] = fbd_dev; 
	return 0;
out:
	blk_cleanup_queue(fbd_dev->disk->queue);
	put_disk(fbd_dev->disk);
	kfree(fbd_dev);
	return err;
}

static int __init fbd_init(void)
{
  int rc;

  if (register_blkdev(FBD_DEV_MAJOR_NUM, "fbd")) {
    return -EIO;
  }
  printk("fbd: Registered  Formation  Data System  device at major %d\n", FBD_DEV_MAJOR_NUM);

  rc = fbd_sysfs_init();
  if (rc)
    {	
      printk(" Error: Creating the sysfs interface  for fbd  block dev code: %x \n",rc);
      return rc;
    }

  return blktap_init();

}

int fbd_device_destroy(struct fbd_device *fb_dev)
{

  struct device *dev;
  struct gendisk *disk;

  if (!fb_dev) {
    return 0;
  }

  printk("FDS: Cleaning up queue and disk associated with fbd device\n");

  dev = &fb_dev->dev;

  disk = fb_dev->disk;
  if (disk) 
    {
      del_gendisk(disk);
      blk_cleanup_queue(disk->queue);
      put_disk(disk);
    }

  device_unregister(dev);

  fbd_devices[fb_dev->dev_id] = NULL;
  kfree(fb_dev);
  /* release module ref */
  // module_put(THIS_MODULE);
  return (0);

}


static void __exit fbd_cleanup(void)
{
  int i;

  printk("*** FDS: Uninstall Begin\n");

  for (i = 0; i < blktap_max_minor; i++) {
    if (fbd_devices[i]) {
      blktap_control_destroy_tap(fbd_devices[i]);
      fbd_device_destroy(fbd_devices[i]);
    }
  }

  blktap_exit();
  printk("FDS blktap exit complete\n");

  fbd_sysfs_cleanup();
  printk("FDS: sysfs cleanup complete\n");

  unregister_blkdev(FBD_DEV_MAJOR_NUM, "fbd");
  printk(" unresgister blkdev \n");
 
  printk("fbd: Removing the  device  Major Number %d\n", FBD_DEV_MAJOR_NUM);
}


module_init(fbd_init);
module_exit(fbd_cleanup);

MODULE_DESCRIPTION("Formation Data  System Network Block Device");
MODULE_LICENSE("GPL");

