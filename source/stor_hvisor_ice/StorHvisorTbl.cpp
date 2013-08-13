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

#include <Ice/Ice.h>
#include <FDSP.h>
#include <ObjectFactory.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include <StorHvisorNet.h>

#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "list.h"
#include <linux/errno.h>

extern FDSP_NetworkCon  NETPtr;

int FDSP_procTbl::fds_init_dmt(void)
{
  int i =0;
	
	for(i=0; i< FDS_MAC_DM_ENTRIES; i++)
	{
		INIT_LIST_HEAD(&(NETPtr.procIo.dmt_tbl[i].list));	

	}
  return 0;
}

int FDSP_procTbl::fds_init_dlt(void)
{  
  int i =0;
	
	for(i=0; i<= FDS_MAC_SM_ENTRIES; i++)
	{
		INIT_LIST_HEAD(&(NETPtr.procIo.dlt_tbl[i].list));	

	}
  return 0;
}


int FDSP_procTbl::add_dlt_entry(FDSP_SmNode *newdlt, int doid_key)
{
 	if(newdlt == NULL)
		printf(" Error: New dlt  Node is empty \n");

	list_add_tail(&newdlt->list, &NETPtr.procIo.dlt_tbl[doid_key].list);

  return 0;
}

int FDSP_procTbl:: add_dmt_entry(FDSP_DmNode  *newdmt, volid_t  vol_id)
{
 	if(newdmt == NULL)
		printf(" Error: New dmt  Node is empty \n");

	list_add_tail(&newdmt->list, &NETPtr.procIo.dmt_tbl[vol_id].list);
  return 0;
}


int FDSP_procTbl::del_dmt_entry( int ipaddr, volid_t  vol_id)
{
	FDSP_DmNode  *tmpdmt;

	list_for_each_entry(tmpdmt, &NETPtr.procIo.dmt_tbl[vol_id].list, list)
	{
		if (tmpdmt->node_ipaddr == ipaddr)
		{
				list_del(&tmpdmt->list);
				free(tmpdmt);
		}
	} 
  return 0;
}

int FDSP_procTbl::del_dlt_entry(int ipaddr, int doid_key)
{
	FDSP_SmNode  *tmpdlt;

	list_for_each_entry(tmpdlt, &NETPtr.procIo.dlt_tbl[doid_key].list, list)
	{
		if (tmpdlt->node_ipaddr == ipaddr)
		{
				list_del(&tmpdlt->list);
				free(tmpdlt);
		}
	} 
  return 0;
}

FDSP_SmNode* FDSP_procTbl::get_sm_nodes_for_doid_key(int doid_key)
{
  return (&NETPtr.procIo.dlt_tbl[doid_key]);
}



FDSP_DmNode* FDSP_procTbl::get_dm_nodes_for_volume(volid_t vol_id)
{
  return (&NETPtr.procIo.dmt_tbl[vol_id]);
}



/* add sample entries to the  DMT and DLT tables */

int FDSP_procTbl::populate_dmt_dlt_tbl(void)
{

	FDSP_DmNode  *adddmt;
	FDSP_SmNode  *adddlt;
	volid_t	  vol_id = 0;
	int  doid_sm_key = 0;
	

	for (vol_id = 0; vol_id < FDS_MAC_DM_ENTRIES; vol_id++) {

	  adddmt = (FDSP_DmNode *)malloc(sizeof(*adddmt));
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

	  adddmt = (FDSP_DmNode *)malloc(sizeof(*adddmt));
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

	  adddlt = (FDSP_SmNode *)malloc(sizeof(*adddlt));
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

	  adddlt =(FDSP_SmNode *) malloc(sizeof(*adddlt));
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
