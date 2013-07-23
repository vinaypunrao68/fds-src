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

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include "vvclib.h"
#include "fds.h"

DM_NODES 	dmt_tbl[FDS_MAC_DM_ENTRIES];
SM_NODES 	dlt_tbl[FDS_MAC_SM_ENTRIES];

int  fds_init_dmt(void)
{
  int i =0;
	
	for(i=0; i<= FDS_MAC_DM_ENTRIES; i++)
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
		printk(" Error: New dlt  Node is empty \n");

	list_add_tail(&newdlt->list, &dlt_tbl[doid_key].list);

  return 0;
}

int add_dmt_entry(DM_NODES  *newdmt, volid_t  vol_id)
{
 	if(newdmt == NULL)
		printk(" Error: New dmt  Node is empty \n");

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
				kfree(tmpdmt);
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
				kfree(tmpdlt);
		}
	} 
  return 0;
}

SM_NODES *get_sm_nodes_for_doid_key(uint32_t doid_key)
{
  return (&dlt_tbl[doid_key]);
}

int show_dlt_entry(uint32_t  doid_key)
{
	SM_NODES  *tmpdmt;

	list_for_each_entry(tmpdmt, &dlt_tbl[doid_key].list, list)
		printk(" ipaddr: %x stor_type:%d node_state:%d node_type:%d \n",tmpdmt->node_ipaddr,tmpdmt->stor_type, \
																	tmpdmt->node_state,tmpdmt->node_type);
  return 0;

}

DM_NODES *get_dm_nodes_for_volume(volid_t vol_id)
{
  return (&dmt_tbl[vol_id]);
}

int show_dmt_entry(volid_t  vol_id)
{
	DM_NODES  *tmpdlt;

	list_for_each_entry(tmpdlt, &dmt_tbl[vol_id].list, list)
		printk(" ipaddr: %x node_state:%d node_type:%d \n",tmpdlt->node_ipaddr, tmpdlt->node_state,tmpdlt->node_type);
  return 0;

}

/* add sample entries to the  DMT and DLT tables */

int populate_dmt_dlt_tbl(void)
{

	DM_NODES  *adddmt;
	SM_NODES  *adddlt;
	volid_t	  vol_id = 0;
	uint32_t  doid_sm_key = 0;
	

	for (vol_id = 0; vol_id < FDS_MAC_DM_ENTRIES; vol_id++) {

	  adddmt = kzalloc(sizeof(*adddmt), GFP_KERNEL);
	  if (!adddmt)
	    {
		printk("Error Allocating the dmt node \n");
		return -ENOMEM;
	    }

	  adddmt->node_ipaddr = 0xc0a8016e;
	  adddmt->node_state = FDS_NODE_ONLINE;
	  adddmt->num_nodes = 1;
	  adddmt->node_type = FDS_NODE_TYPE_PRIM;

	  add_dmt_entry(adddmt, vol_id);

	  adddmt = kzalloc(sizeof(*adddmt), GFP_KERNEL);
	  if (!adddmt)
	    {
	      printk("Error Allocating the dmt node \n");
	      return -ENOMEM;
	    }

	  adddmt->node_ipaddr = 0xc0a80103;
	  adddmt->node_state = FDS_NODE_ONLINE;
	  adddmt->num_nodes = 1;
	  adddmt->node_type = FDS_NODE_TYPE_SEND;

	  add_dmt_entry(adddmt, vol_id);

	}

	for (doid_sm_key = 0; doid_sm_key < FDS_MAC_SM_ENTRIES; doid_sm_key++) {

	  adddlt = kzalloc(sizeof(*adddlt), GFP_KERNEL);
	  if (!adddlt)
	    {
	      printk("Error Allocating the dlt node \n");
	      return -ENOMEM;
	    }

	  adddlt->node_ipaddr = 0xc0a80103;
	  adddlt->stor_type = FDS_STORAGE_TYPE_SSD;
	  adddlt->node_state = FDS_NODE_ONLINE;
	  adddlt->num_nodes = 1;
	  adddlt->node_type =  FDS_NODE_TYPE_PRIM;

	  add_dlt_entry(adddlt, doid_sm_key);

	  adddlt = kzalloc(sizeof(*adddlt), GFP_KERNEL);
	  if (!adddlt)
	    {
	      printk("Error Allocating the dlt node \n");
	      return -ENOMEM;
	    }

	  adddlt->node_ipaddr = 0xc0a8016e;
	  adddlt->stor_type = FDS_STORAGE_TYPE_SSD;
	  adddlt->node_state = FDS_NODE_ONLINE;
	  adddlt->num_nodes = 1;
	  adddlt->node_type =  FDS_NODE_TYPE_SEND;

	  add_dlt_entry(adddlt, doid_sm_key);

	}

  return 0;
}
