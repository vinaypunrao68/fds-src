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

extern StorHvCtrl *storHvisor;

void StorHvDataPlacement::nodeEventHandler(int node_id, unsigned int node_ip_addr, int node_state)
{
char ip_address[64];
    struct sockaddr_in sockaddr;
    sockaddr.sin_addr.s_addr = node_ip_addr;
    inet_ntop(AF_INET, &(sockaddr.sin_addr), ip_address, INET_ADDRSTRLEN);
    switch(node_state) { 
       case FDS_Node_Up : 
          //storHvisor->rpcSwitchTbl->Add_RPC_EndPoint(ip_address, storMgrPortNum, FDSP_STOR_MGR);
          //storHvisor->rpcSwitchTbl->Add_RPC_EndPoint(ip_address, dataMgrPortNum, FDSP_DATA_MGR); 
         break;

       case FDS_Node_Down:
       case FDS_Node_Rmvd:
          storHvisor->rpcSwitchTbl->Delete_RPC_EndPoint(ip_address,  FDSP_STOR_MGR);
          storHvisor->rpcSwitchTbl->Delete_RPC_EndPoint(ip_address,  FDSP_DATA_MGR);
	break;
    }
}

StorHvDataPlacement::StorHvDataPlacement() {
   omClient = new OMgrClient();
   omClient->initialize();
   omClient->registerEventHandlerForNodeEvents((node_event_handler_t )nodeEventHandler);
   omClient->subscribeToOmEvents(omIPAddr, 1, 1);
}


StorHvDataPlacement::~StorHvDataPlacement() {
}
