/*
 *
 * Copyright (C) 2011 Citrix Systems Inc.
 *
 * This file is part of Blktap2.
 *
 * Blktap2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * Blktap2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with Blktap2.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 *
 */

#include <net/sock.h>
#include <linux/net.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
// #include "vvclib.h"
// #include "../include/fds_commons.h"
// #include "../include/fdsp.h"
#include "blktap.h"
#include "fbd.h"
#include "fds.h"


DEFINE_MUTEX(blktap_lock);

struct fbd_device **fbd_devices;
int blktap_max_minor;
static struct blktap_page_pool *default_pool;

extern struct fbd_device *fbd_dev;

static struct fbd_devices *
blktap_control_get_minor(void)
{
	int minor;
	struct fbd_device *tap;

#if 0
	tap = kzalloc(sizeof(*tap), GFP_KERNEL);
	if (unlikely(!tap))
		return NULL;

	mutex_lock(&blktap_lock);

	for (minor = 0; minor < blktap_max_minor; minor++)
		if (!fbd_devices[minor])
			break;

	if (minor == MAX_BLKTAP_DEVICE)
		goto fail;

	if (minor == blktap_max_minor) {
		void *p;
		int n;

		n = min(2 * blktap_max_minor, MAX_BLKTAP_DEVICE);
		p = krealloc(fbd_devices, n * sizeof(fbd_devices[0]), GFP_KERNEL);
		if (!p)
			goto fail;

		fbd_devices          = p;
		minor            = blktap_max_minor;
		blktap_max_minor = n;

		memset(&fbd_devices[minor], 0, (n - minor) * sizeof(fbd_devices[0]));
	}

	tap->dev_id = minor;
	fbd_devices[minor] = tap;

	__module_get(THIS_MODULE);
out:
	mutex_unlock(&blktap_lock);
#endif
	return fbd_dev;

#if 0
fail:
	mutex_unlock(&blktap_lock);
	kfree(tap);
	tap = NULL;
	goto out;
#endif
}

static void
blktap_control_put_minor(struct fbd_device* tap)
{
	fbd_devices[tap->dev_id] = NULL;
//	kfree(tap);

	module_put(THIS_MODULE);
}

static struct fbd_device*
blktap_control_create_tap(void)
{
	struct fbd_device *tap;
	int err;

	tap = blktap_control_get_minor();
	if (!tap)
		return NULL;

	kobject_get(&default_pool->kobj);
	tap->pool = default_pool;

	err = blktap_ring_create(tap);
	if (err)
		goto fail_tap;

#if 0
	err = blktap_sysfs_create(tap);
	if (err)
		goto fail_ring;
#endif

	return tap;

fail_ring:
	blktap_ring_destroy(tap);
fail_tap:
	blktap_control_put_minor(tap);

	return NULL;
}

int
blktap_control_destroy_tap(struct fbd_device *tap)
{
	int err;

	err = blktap_ring_destroy(tap);
	if (err)
		return err;

	kobject_put(&tap->pool->kobj);

//	blktap_sysfs_destroy(tap);

	blktap_control_put_minor(tap);

	return 0;
}

static long
blktap_control_ioctl(struct file *filp,
		     unsigned int cmd, unsigned long arg)
{
	struct fbd_device *tap;

	switch (cmd) {
	case BLKTAP_IOCTL_ALLOC_TAP: {
		struct blktap_info info;
		void __user *ptr = (void __user*)arg;

		tap = blktap_control_create_tap();
		if (!tap)
			return -ENOMEM;

		info.ring_major = blktap_ring_major;
		info.bdev_major = blktap_device_major;
		info.ring_minor = tap->dev_id;

		if (copy_to_user(ptr, &info, sizeof(info))) {
			blktap_control_destroy_tap(tap);
			return -EFAULT;
		}

		return 0;
	}

	case BLKTAP_IOCTL_FREE_TAP: {
		int minor = arg;

		if (minor > MAX_BLKTAP_DEVICE)
			return -EINVAL;

		tap = fbd_dev;
		if (!tap)
			return -ENODEV;

		return blktap_control_destroy_tap(tap);
	}
	}

	return -ENOIOCTLCMD;
}

static struct file_operations blktap_control_file_operations = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = blktap_control_ioctl,
};

static struct miscdevice blktap_control = {
	.minor    = MISC_DYNAMIC_MINOR,
	.name     = "blktap-control",
	.fops     = &blktap_control_file_operations,
};

static struct device *control_device;

static ssize_t
blktap_control_show_default_pool(struct device *device,
				 struct device_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%s", kobject_name(&default_pool->kobj));
}

static ssize_t
blktap_control_store_default_pool(struct device *device,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct blktap_page_pool *pool, *tmp = default_pool;

	pool = blktap_page_pool_get(buf);
	if (IS_ERR(pool))
		return PTR_ERR(pool);

	default_pool = pool;
	kobject_put(&tmp->kobj);

	return size;
}

static DEVICE_ATTR(default_pool, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH,
		   blktap_control_show_default_pool,
		   blktap_control_store_default_pool);

size_t
blktap_control_debug(struct fbd_device *tap, char *buf, size_t size)
{
	char *s = buf, *end = buf + size;

	s += snprintf(s, end - s,
		      "tap %u:%u name:'%s' flags:%#08lx\n",
		      MAJOR(tap->ring.devno), MINOR(tap->ring.devno),
		      tap->blk_name, tap->dev_inuse);

	return s - buf;
}

static int __init
blktap_control_init(void)
{
	int err;

	err = misc_register(&blktap_control);
	if (err)
		return err;

	control_device = blktap_control.this_device;

	blktap_max_minor = min(64, MAX_BLKTAP_DEVICE);
	fbd_devices = kzalloc(blktap_max_minor * sizeof(fbd_devices[0]), GFP_KERNEL);
	if (!fbd_devices) {
		printk("failed to allocate blktap minor map");
		return -ENOMEM;
	}

	err = blktap_page_pool_init(&control_device->kobj);
	if (err)
	{
		printk(" Error allocating  blktap page pool \n");
		return err;
	}

	default_pool = blktap_page_pool_get("default");
	if (!default_pool)
	{
		printk(" Error: default  pool is NULL\n");
		return -ENOMEM;
	}

	err = device_create_file(control_device, &dev_attr_default_pool);
	if (err)
	{
		printk(" device create failed \n");
		return err;
	}

	return 0;
}

static void
blktap_control_exit(void)
{
	if (default_pool) {
		kobject_put(&default_pool->kobj);
		default_pool = NULL;
	}

	blktap_page_pool_exit();

	if (fbd_devices) {
		kfree(fbd_devices);
		fbd_devices = NULL;
	}

	if (control_device) {
		misc_deregister(&blktap_control);
		control_device = NULL;
	}
}

void
blktap_exit(void)
{
	blktap_control_exit();
	blktap_ring_exit();
//	blktap_sysfs_exit();
//	blktap_device_exit();
}

int
blktap_init(void)
{
	int err;

#if 0
	err = blktap_device_init();
	if (err)
		goto fail;
#endif

	err = blktap_ring_init();
	if (err)
		goto fail;
printk( "ring init successful \n ");

#if 0
	err = blktap_sysfs_init();
	if (err)
		goto fail;
#endif

	err = blktap_control_init();
	if (err)
		goto fail;
printk(" blktap control  init successful \n");

	return 0;

fail:
	printk(" blktap init failed \n");
	blktap_exit();
	return err;
}

//module_init(blktap_init);
//module_exit(blktap_exit);
//MODULE_LICENSE("Dual BSD/GPL");
