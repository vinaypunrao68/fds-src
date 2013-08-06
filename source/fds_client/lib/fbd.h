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
 *              - shared  defines for fbd driver conffigurations 
 */

#ifndef _FBD_h
#define _FBD_h

/* flags  */
#define FBD_READ_ONLY    (1 << 0)
#define FBD_SEND_FLUSH   (1 << 1)
#define FBD_SEND_TRIM    (1 << 2)

#define  FBD_OPEN_TARGET_CON_TCP	100
#define  FBD_OPEN_TARGET_CON_UDP	101
#define  FBD_CLOSE_TARGET_CON		102
#define  FBD_SET_TGT_SIZE		103
#define  FBD_SET_TGT_BLK_SIZE		104
#define  FBD_WRITE_DATA			105
#define  FBD_READ_DATA			106
#define  FBD_ADD_DEV			107
#define  FBD_DEL_DEV			108
#define  FBD_READ_VVC_CATALOG		109
#define  FBD_READ_DMT_TBL		110
#define  FBD_READ_DLT_TBL		111
#define  FBD_SET_BASE_PORT              112


/* cluster transport structure */

struct fbd_contbl {

	int sock;
	struct sockaddr_storage dest_addr;
	struct sockaddr_storage loc_addr;
	uint64_t out_seq;		 

};


struct fbd_device {

  int dev_id;
	pthread_mutex_t tx_lock;
	int blocksize;
	uint64_t bytesize;
	int xmit_timeout;
	int sock;
	long int   src_ip_addr;
	long int   tcp_destAddr;
	long int   udp_destAddr;
        int stor_mgr_port;
        int data_mgr_port;
	int 	   proto_type;
	/*  vvc  defs */
	volid_t 	vol_id;
	vvc_vhdl_t	vhdl;
  pthread_t rx_thread;

};

#define  FBD_CMD_READ			01
#define  FBD_CMD_WRITE			02
#define  FBD_CMD_FLUSH			03
#define  FBD_CMD_DISCARD		04

/* proto defines  */
#define FBD_PROTO_TCP			01	
#define FBD_PROTO_UDP			02
#define FBD_PROTO_MCAST			03

#define FBD_DEV_MAJOR_NUM		63
#define FBD_MINORS_PER_MAJOR   		16

#define FBD_CLUSTER_TCP_PORT_SM		6900
#define FBD_CLUSTER_TCP_PORT_DM		6901
#define FBD_CLUSTER_UDP_PORT_SM		9600
#define FBD_CLUSTER_UDP_PORT_DM		9601

#define HVISOR_SECTOR_SIZE 512

int send_data_dm(struct fbd_device *fbd, int send, void *buf, int size, int msg_flags, long int node_ipaddr);
#endif
