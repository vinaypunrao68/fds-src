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

#ifndef _BLKTAP_H_
#define _BLKTAP_H_

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/scatterlist.h>
#include <linux/mutex.h>
#include "linux-blktap.h"

//extern int blktap_debug_level;
extern int blktap_ring_major;
extern int blktap_device_major;

#define BTPRINTK(level, tag, force, _f, _a...)		
#if 0
	do {								\
		if (blktap_debug_level > level &&			\
		    (force || printk_ratelimit()))			\
			printk(tag "%s: " _f, __func__, ##_a);		\
	} while (0)
#endif

#define BTDBG(_f, _a...)             BTPRINTK(8, KERN_DEBUG, 1, _f, ##_a)
#define BTINFO(_f, _a...)            BTPRINTK(0, KERN_INFO, 0, _f, ##_a)
#define BTWARN(_f, _a...)            BTPRINTK(0, KERN_WARNING, 0, _f, ##_a)
#define BTERR(_f, _a...)             BTPRINTK(0, KERN_ERR, 0, _f, ##_a)

#define MAX_BLKTAP_DEVICE            1024

#define BLKTAP_DEVICE                4
#define BLKTAP_DEVICE_CLOSED         5
#define BLKTAP_SHUTDOWN_REQUESTED    8

#define BLKTAP_REQUEST_FREE          0
#define BLKTAP_REQUEST_PENDING       1

struct blktap_device {
	spinlock_t                     lock;
	struct gendisk                *gd;
};

struct blktap_request {
	struct blktap                 *tap;
	struct request                *rq;
	int                            usr_idx;

	int                            operation;

	struct scatterlist             sg_table[BLKTAP_SEGMENT_MAX];
	struct page                   *pages[BLKTAP_SEGMENT_MAX];
	int                            nr_pages;
};


struct blktap_ring {
	struct task_struct            *task;

	struct vm_area_struct         *vma;
	struct mutex                   vma_lock;
	struct blktap_front_ring       ring;
	unsigned long                  ring_vstart;
	unsigned long                  user_vstart;

	int                            n_pending;
	struct blktap_request         *pending[BLKTAP_RING_SIZE];

	wait_queue_head_t              poll_wait;

	dev_t                          devno;
	struct device                 *dev;
};

struct blktap_statistics {
	unsigned long                  st_print;
	int                            st_rd_req;
	int                            st_wr_req;
	int                            st_tr_req;
	int                            st_oo_req;
	int                            st_fl_req;
	int                            st_rd_sect;
	int                            st_wr_sect;
	int                            st_tr_sect;
	s64                            st_rd_cnt;
	s64                            st_rd_sum_usecs;
	s64                            st_rd_max_usecs;
	s64                            st_wr_cnt;
	s64                            st_wr_sum_usecs;
	s64                            st_wr_max_usecs;	
};


#define blktap_for_each_sg(_sg, _req, _i)	\
	for (_sg = (_req)->sg_table, _i = 0;	\
	     _i < (_req)->nr_pages;		\
	     (_sg)++, (_i)++)

#if 0
struct blktap {
	int                            minor;
	unsigned long                  dev_inuse;

	struct blktap_ring             ring;
	struct blktap_device           device;
	struct blktap_page_pool       *pool;

	wait_queue_head_t              remove_wait;
	struct work_struct             remove_work;
	char                           name[BLKTAP_NAME_MAX];

	struct blktap_statistics       stats;
};
#endif

struct blktap_page_pool {
	struct mempool_s              *bufs;
	spinlock_t                     lock;
	struct kobject                 kobj;
	wait_queue_head_t              wait;
};

extern struct mutex blktap_lock;
extern struct fbd_device **fbd_devices;
extern int blktap_max_minor;


#endif
