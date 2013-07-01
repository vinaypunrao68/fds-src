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

#define  FDS_MAX_DM_NODES_PER_CLST 	16
#define  FDS_MAX_SM_NODES_PER_CLST	16
#define  FDS_

#define  FDS_NODE_OFFLINE		0
#define  FDS_NODE_ONLINE		1

#define  FDS_STORAGE_TYPE_SSD		1
#define  FDS_STORAGE_TYPE_FLASH		2
#define  FDS_STORAGE_TYPE_HARD		3
#define  FDS_STORAGE_TYPE_TAPE		4


#define  FDS_NODE_TYPE_PRIM		1
#define  FDS_NODE_TYPE_SEND		2
#define  FDS_NODE_TYPE_BCKP		3

/* fds mesage command  */
#define FDS_CMD_IO			1

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
	uint8_t  stor_type; 	/* type  of the storage in the node */
	uint8_t  node_state; 	/* state of the node online/offline */
	uint8_t  num_nodes;	/* number of DM nodes in the cluster */
	uint8_t  node_type;	/* primary, secondary .... */

}DM_NODES;

/* storage manager  structure */

typedef  struct sm_nodes {
	uint32_t node_ipaddr;   /* ip address of the node */
	uint8_t  node_state; 	/* state of the node online/offline */
	uint8_t  num_nodes;	/* number of DM nodes in the cluster */
	uint8_t  node_type;	/* primary, secondary .... */

}SM_NODES;


/* hypervisor  cache catalog */

/* message data  structures  */
typedef struct fds_msghdr {
	uint16_t   msg_type;
	uint16_t   msg_cmd;
	uint32_t   hdr_len;
}FDS_MSGHDR;

typedef  struct  fds_iodesc {
	uint32_t   buf_ptr;
	uint32_t   buf_offset;
	uint32_t   blk_len;

}FDS_IODESC;

typedef  struct fds_msg {
	FDS_MSGHDR  hdr;
	FDS_DOID    doid;
	FDS_IODESC  iodesc;	
}FDS_MSG;
