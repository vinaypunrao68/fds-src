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
 *                - dmt and dlt table  data structures amd API's 
 */

#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "list.h"
#include <linux/errno.h>

#include "tapdisk.h"
#include "vvclib.h"
#include "hvisor_lib.h"

#include "fbd.h"
#include "fds.h"


DM_NODES 	dmt_tbl[FDS_MAC_DM_ENTRIES];
SM_NODES 	dlt_tbl[FDS_MAC_SM_ENTRIES];

int  fds_init_dmt(void)
{
  int i =0;
	
	for(i=0; i< FDS_MAC_DM_ENTRIES; i++)
	{
		INIT_LIST_HEAD(&(dmt_tbl[i].list));	

	}
  return 0;
}

int fds_init_dlt(void)
{  
  int i =0;
	
	for(i=0; i<= FDS_MAC_SM_ENTRIES; i++)
	{
		INIT_LIST_HEAD(&(dlt_tbl[i].list));	

	}
  return 0;
}


int  add_dlt_entry(SM_NODES *newdlt, uint32_t doid_key)
{
 	if(newdlt == NULL)
		printf(" Error: New dlt  Node is empty \n");

	list_add_tail(&newdlt->list, &dlt_tbl[doid_key].list);

  return 0;
}

int add_dmt_entry(DM_NODES  *newdmt, volid_t  vol_id)
{
 	if(newdmt == NULL)
		printf(" Error: New dmt  Node is empty \n");

	list_add_tail(&newdmt->list, &dmt_tbl[vol_id].list);
  return 0;
}


int del_dmt_entry( uint32_t ipaddr, volid_t  vol_id)
{
	DM_NODES  *tmpdmt;

	list_for_each_entry(tmpdmt, &dmt_tbl[vol_id].list, list)
	{
		if (tmpdmt->node_ipaddr == ipaddr)
		{
				list_del(&tmpdmt->list);
				free(tmpdmt);
		}
	} 
  return 0;
}

int del_dlt_entry(uint32_t ipaddr, uint32_t doid_key)
{
	SM_NODES  *tmpdlt;

	list_for_each_entry(tmpdlt, &dlt_tbl[doid_key].list, list)
	{
		if (tmpdlt->node_ipaddr == ipaddr)
		{
				list_del(&tmpdlt->list);
				free(tmpdlt);
		}
	} 
  return 0;
}

SM_NODES *get_sm_nodes_for_doid_key(uint32_t doid_key)
{
  return (&dlt_tbl[doid_key]);
}



DM_NODES *get_dm_nodes_for_volume(volid_t vol_id)
{
  return (&dmt_tbl[vol_id]);
}



/* add sample entries to the  DMT and DLT tables */

int populate_dmt_dlt_tbl(void)
{

	DM_NODES  *adddmt;
	SM_NODES  *adddlt;
	volid_t	  vol_id = 0;
	uint32_t  doid_sm_key = 0;
	

	for (vol_id = 0; vol_id < FDS_MAC_DM_ENTRIES; vol_id++) {

	  adddmt = malloc(sizeof(*adddmt));
	  if (!adddmt)
	    {
		printf("Error Allocating the dmt node \n");
		return -ENOMEM;
	    }
	  memset(adddmt, 0, sizeof(*adddmt));
	  adddmt->node_ipaddr = 0xc0a8016e;
	  adddmt->node_state = FDS_NODE_ONLINE;
	  adddmt->num_nodes = 1;
	  adddmt->node_type = FDS_NODE_TYPE_PRIM;

	  add_dmt_entry(adddmt, vol_id);

	  adddmt = malloc(sizeof(*adddmt));
	  if (!adddmt)
	    {
	      printf("Error Allocating the dmt node \n");
	      return -ENOMEM;
	    }
	  memset(adddmt, 0, sizeof(*adddmt));
	  
	  adddmt->node_ipaddr = 0xc0a80103;
	  adddmt->node_state = FDS_NODE_ONLINE;
	  adddmt->num_nodes = 1;
	  adddmt->node_type = FDS_NODE_TYPE_SEND;

	  add_dmt_entry(adddmt, vol_id);

	}

	for (doid_sm_key = 0; doid_sm_key < FDS_MAC_SM_ENTRIES; doid_sm_key++) {

	  adddlt = malloc(sizeof(*adddlt));
	  if (!adddlt)
	    {
	      printf("Error Allocating the dlt node \n");
	      return -ENOMEM;
	    }
	  memset(adddlt, 0, sizeof(*adddlt));

	  adddlt->node_ipaddr = 0xc0a80103;
	  adddlt->stor_type = FDS_STORAGE_TYPE_SSD;
	  adddlt->node_state = FDS_NODE_ONLINE;
	  adddlt->num_nodes = 1;
	  adddlt->node_type =  FDS_NODE_TYPE_PRIM;

	  add_dlt_entry(adddlt, doid_sm_key);

	  adddlt = malloc(sizeof(*adddlt));
	  if (!adddlt)
	    {
	      printf("Error Allocating the dlt node \n");
	      return -ENOMEM;
	    }
	  memset(adddlt, 0, sizeof(*adddlt));
	  
	  adddlt->node_ipaddr = 0xc0a8016e;
	  adddlt->stor_type = FDS_STORAGE_TYPE_SSD;
	  adddlt->node_state = FDS_NODE_ONLINE;
	  adddlt->num_nodes = 1;
	  adddlt->node_type =  FDS_NODE_TYPE_SEND;

	  add_dlt_entry(adddlt, doid_sm_key);

	}

  return 0;
}
