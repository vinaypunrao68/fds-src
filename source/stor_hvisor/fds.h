/* Copyright (c) 2013 - 2014 All Right Reserved
*  Company= Formation  Data Systems. 
*
* This source is subject to the Formation Data systems Permissive License.
* All other rights reserved.
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
* This file contains the  data  structure corresponds to 
*	- data object identifier (DOID) 
*	- Data manager  table  structures 
*	- storage  manager table structures 
*	- glue data structures for  dm/sm look up launch and  processing 
*/

#ifndef _FDS_h
#define _FDS_h

#define  FDS_MAX_DM_NODES_PER_CLST 	16
#define  FDS_MAX_SM_NODES_PER_CLST	16
#define  FDS_MAC_DM_ENTRIES 	        256	
#define  FDS_MAC_SM_ENTRIES 	        256	
#define  FDS_READ_WRITE_LOG_ENTRIES 	256	

#define  FDS_NODE_OFFLINE		0
#define  FDS_NODE_ONLINE		1

#define  FDS_STORAGE_TYPE_SSD		1
#define  FDS_STORAGE_TYPE_FLASH		2
#define  FDS_STORAGE_TYPE_HARD		3
#define  FDS_STORAGE_TYPE_TAPE		4


#define  FDS_NODE_TYPE_PRIM		1
#define  FDS_NODE_TYPE_SEND		2
#define  FDS_NODE_TYPE_BCKP		3

#define  FDS_TRANS_EMPTY		0x00
#define  FDS_TRANS_OPEN			0x1
#define  FDS_TRANS_OPENED               0x2
#define  FDS_TRANS_COMMITTED            0x3
#define  FDS_TRANS_SYNCED		0x4
#define  FDS_TRANS_DONE                 0x5

#define  FDS_SUCCESS			0
#define  FDS_FAILURE			1

#define  FDS_TIMER_TIMEOUT		1

#define  FDS_MIN_ACK			1
#define  FDS_CLS_ACK			0
#define  FDS_SET_ACK			1

#define  FDS_COMMIT_MSG_SENT            1
#define  FDS_COMMIT_MSG_ACKED           2

/* DOID  structure  */
typedef  struct base_doid {
	uint32_t dm_lkupkey; /* data manager key for node locations */ 
	uint32_t sm_lkupkey; /* storage manager  key for data placement */
	uint32_t hash_1;     /* rest of the hash results, total 16 byte */
	uint32_t hash_2;
} BASE_DOID;

typedef struct fds_doid {
	BASE_DOID doid;
	uint8_t hash_colli;
	uint8_t hash_lblock;
	uint8_t hash_sblock;
	uint8_t proc_state;
	uint8_t arch_state;
}FDS_DOID;

/* Data  manager  structures  */

typedef  struct dm_nodes {
	uint32_t node_ipaddr;   /* ip address of the node */
	uint8_t  node_state; 	/* state of the node online/offline */
	uint8_t  num_nodes;	/* number of DM nodes in the cluster */
	uint8_t  node_type;	/* primary, secondary .... */
	uint8_t  node_id;       /* node identifier , this might be overloaded with IP address */
	struct   list_head list;

}DM_NODES;

/* storage manager  structure */

typedef  struct sm_nodes {
	uint32_t node_ipaddr;   /* ip address of the node */
	uint8_t  stor_type; 	/* type  of the storage in the node */
	uint8_t  node_state; 	/* state of the node online/offline */
	uint8_t  num_nodes;	/* number of DM nodes in the cluster */
	uint8_t  node_type;	/* primary, secondary .... */
	uint8_t  node_id;       /* node identifier , this might be overloaded with IP address */
	struct   list_head list;

}SM_NODES;

typedef  struct ip_node {
	uint32_t ipAddr;
	uint8_t  ack_status;
	uint8_t  commit_status;
}IP_NODE;

typedef struct fds_journ {
	uint8_t  replc_cnt;
	uint8_t  sm_ack_cnt;
	uint8_t  dm_ack_cnt;
        uint8_t  dm_commit_cnt;
	uint8_t  trans_state;
	void	 *fbd_ptr;
	void	 *read_ctx;
	void	 *write_ctx;
	void	 *sm_msg;
	void	 *dm_msg;
        struct   timer_list *p_ti;
	uint16_t   lt_flag;
	uint16_t   st_flag;
        uint8_t    num_dm_nodes;
	IP_NODE	   dm_ack[FDS_MAX_DM_NODES_PER_CLST];
        uint8_t    num_sm_nodes;
	IP_NODE	   sm_ack[FDS_MAX_SM_NODES_PER_CLST];
}FDS_JOURN;

/* bit manupulation macros */

#define fds_bit_get(p,m) ((p) & (m))
#define fds_bit_set(p,m) ((p) |= (m))
#define fds_bit_clear(p,m) ((p) &= ~(m))
#define fds_bit_flip(p,m) ((p) ^= (m))
#define fds_bit(x)  (0x01 << (x))


int fds_init_dmt(void);
int fds_init_dlt(void);
int add_dlt_entry(SM_NODES *newdlt, uint32_t doid_key);
int add_dmt_entry(DM_NODES  *newdmt, int  vol_id);
int del_dmt_entry( uint32_t ipaddr, int  vol_id);
int del_dlt_entry(uint32_t ipaddr, uint32_t doid_key);
int populate_dmt_dlt_tbl(void);
int show_dmt_entry(int vol_id);
int show_dlt_entry(uint32_t doid_key);
DM_NODES *get_dm_nodes_for_volume(int vol_id);
SM_NODES *get_sm_nodes_for_doid_key(uint32_t doid_key);
int fds_init_trans_log(void);
int fds_process_rx_message(uint8_t  *rx_buf, uint32_t src_ip);
int get_trans_id(void);
void fbd_process_req_timeout(unsigned long arg);

int fbd_device_create(int minor);
int fbd_device_destroy(struct fbd_device *fb_dev);


/* blk tap  changes */
int blktap_control_destroy_tap(struct fbd_device *);
size_t blktap_control_debug(struct fbd_device *, char *, size_t);

int blktap_ring_init(void);
void blktap_ring_exit(void);
size_t blktap_ring_debug(struct fbd_device *, char *, size_t);
int blktap_ring_create(struct fbd_device *);
int blktap_ring_destroy(struct fbd_device *);
struct blktap_request *blktap_ring_make_request(struct fbd_device *);
void blktap_ring_free_request(struct fbd_device *,struct blktap_request *);
void blktap_ring_submit_request(struct fbd_device *, struct blktap_request *);
int blktap_ring_map_request(struct fbd_device *, struct file *, struct blktap_request *);
void blktap_ring_unmap_request(struct fbd_device *, struct blktap_request *);
void blktap_ring_set_message(struct fbd_device *, int);
void blktap_ring_kick_user(struct fbd_device *);

int blktap_sysfs_init(void);
void blktap_sysfs_exit(void);
int blktap_sysfs_create(struct fbd_device *);
void blktap_sysfs_destroy(struct fbd_device *);

int blktap_device_init(void);
void blktap_device_exit(void);
size_t blktap_device_debug(struct fbd_device *, char *, size_t);
int blktap_device_create(struct fbd_device *, struct blktap_device_info *);
int blktap_device_destroy(struct fbd_device *);
void blktap_device_destroy_sync(struct fbd_device *);
void blktap_device_run_queue(struct request_queue *q);
void blktap_device_do_request(struct request_queue *q);
void blktap_device_end_request(struct fbd_device *, struct blktap_request *, int);

int blktap_page_pool_init(struct kobject *);
void blktap_page_pool_exit(void);
struct blktap_page_pool *blktap_page_pool_get(const char *);

size_t blktap_request_debug(struct fbd_device *, char *, size_t);
struct blktap_request *blktap_request_alloc(struct fbd_device *);
int blktap_request_get_pages(struct fbd_device *, struct blktap_request *, int);
void blktap_request_free(struct fbd_device *, struct blktap_request *);
void blktap_request_bounce(struct fbd_device *, struct blktap_request *, int);
void blktap_exit(void);
int blktap_init(void);

/* hypervisor  cache catalog */

#endif
