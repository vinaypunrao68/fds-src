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
int storMgrPortNum = 6901;
int dataMgrPortNum = 6900;
    switch(node_state) { 
       case FDSP_Types::FDS_Node_Up : 
           FDS_PLOG(storHvisor->GetLog()) << "StorHvisorTbl - Node UP event NodeId " << node_id << " Node IP Address " <<  node_ip_addr;
          storHvisor->rpcSwitchTbl->Add_RPC_EndPoint(node_ip_addr, storMgrPortNum, FDSP_STOR_MGR);
          storHvisor->rpcSwitchTbl->Add_RPC_EndPoint(node_ip_addr, dataMgrPortNum, FDSP_DATA_MGR); 
         break;

       case FDSP_Types::FDS_Node_Down:
       case FDSP_Types::FDS_Node_Rmvd:
           FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTbl - Node Down event NodeId :" << node_id << " node IP addr" << node_ip_addr ;
          storHvisor->rpcSwitchTbl->Delete_RPC_EndPoint(node_ip_addr,  FDSP_STOR_MGR);
          storHvisor->rpcSwitchTbl->Delete_RPC_EndPoint(node_ip_addr,  FDSP_DATA_MGR);
	break;
    }
}

StorHvDataPlacement::StorHvDataPlacement(dp_mode _mode)
    : test_ip_addr(0), mode(_mode) {
  if (mode == DP_NORMAL_MODE) {
    omClient = new OMgrClient();
    omClient->initialize();
    omClient->registerEventHandlerForNodeEvents((node_event_handler_t )nodeEventHandler);
    omClient->subscribeToOmEvents(omIPAddr, 1, 1);
  }
}

StorHvDataPlacement::StorHvDataPlacement(dp_mode _mode,
                                         fds_uint32_t test_ip)
    : StorHvDataPlacement(_mode) {
  test_ip_addr = test_ip;
}

StorHvDataPlacement::~StorHvDataPlacement() {
}
