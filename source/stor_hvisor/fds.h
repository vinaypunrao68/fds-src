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

#define  FDS_TRANS_OPEN			0x55
#define  FDS_TRANS_DONE			0xAA
#define  FDS_TRANS_EMPTY		0x00

#define  FDS_SUCCESS			0
#define  FDS_FAILURE			1

#define  FDS_TIMER_TIMEOUT		1

#define  FDS_MIN_ACK			1
#define  FDS_SET_ACK			1
#define  FDS_CLS_ACK			0


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
	uint8_t  trans_state;
	void	 *fbd_ptr;
	void	 *read_ctx;
	void	 *write_ctx;
	void	 *sm_msg;
	void	 *dm_msg;
        struct   timer_list *p_ti;
	uint16_t   lt_flag;
	uint16_t   st_flag;
	IP_NODE	   dm_ack[FDS_MAX_DM_NODES_PER_CLST];
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
int add_dmt_entry(DM_NODES  *newdmt, volid_t  vol_id);
int del_dmt_entry( uint32_t ipaddr, volid_t  vol_id);
int del_dlt_entry(uint32_t ipaddr, uint32_t doid_key);
int populate_dmt_dlt_tbl(void);
int show_dmt_entry(volid_t  vol_id);
int show_dlt_entry(uint32_t doid_key);
DM_NODES *get_dm_nodes_for_volume(volid_t vol_id);
SM_NODES *get_sm_nodes_for_doid_key(uint32_t doid_key);
int fds_init_trans_log(void);
int fds_process_rx_message(uint8_t  *rx_buf);
int get_trans_id(void);

/* hypervisor  cache catalog */

#endif
