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

#include "StorHvisorNet.h"

#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "list.h"
#include <linux/errno.h>

extern StorHvCtrl *storHvisor;

void StorHvDataPlacement::nodeEventHandler(int node_id,
                                           unsigned int node_ip_addr,
                                           int node_state,
                                           fds_uint32_t node_port,
                                           FDS_ProtocolInterface::FDSP_MgrIdType node_type) {
 int storMgrPortNum = 6901;
 int dataMgrPortNum = 6900;
 bool exists;

    switch(node_state) { 
       case FDS_ProtocolInterface::FDS_Node_Up : 
         FDS_PLOG(storHvisor->GetLog()) << "StorHvisorTbl - Node UP event NodeId "
                                        << node_id << " Node IP Address "
                                        << node_ip_addr << " node port "
                                        << node_port;

         /*
          * Check if we know about this node already
          */

         exists = storHvisor->rpcSessionTbl->\
                 clientSessionExists(node_ip_addr, node_port);
         if (exists) {
           FDS_PLOG(storHvisor->GetLog()) << "Node already exists. No need to add.";
           return;
         }
         
         if (node_type == FDS_ProtocolInterface::FDSP_STOR_MGR) {
           FDS_PLOG(storHvisor->GetLog()) << "Adding SM RPC endpoint";
           storHvisor->rpcSessionTbl->
               startSession<netDataPathClientSession>(node_ip_addr,
                                (fds_int32_t)node_port,
                                FDS_ProtocolInterface::FDSP_STOR_MGR,0,
                                storHvisor->dPathRespCback);
         } else if (node_type == FDS_ProtocolInterface::FDSP_DATA_MGR) {
           FDS_PLOG(storHvisor->GetLog()) << "Adding DM RPC endpoint";
           storHvisor->rpcSessionTbl->
             startSession<netMetaDataPathClientSession>(node_ip_addr,
                              (fds_int32_t)node_port,
                              FDS_ProtocolInterface::FDSP_DATA_MGR,0, 
                              storHvisor->mPathRespCback); 
         }
         FDS_PLOG(storHvisor->GetLog()) << "Added an endpoint";

         break;

       case FDS_ProtocolInterface::FDS_Node_Down:
       case FDS_ProtocolInterface::FDS_Node_Rmvd:
         FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTbl - Node Down event NodeId :"
         << node_id << " node IP addr" << node_ip_addr << " port " << node_port ;
         storHvisor->rpcSessionTbl->endClientSession(node_ip_addr, node_port);
         break;
       default:
         fds_verify("Unkown node state");
         break;
    }
}

StorHvDataPlacement::StorHvDataPlacement(dp_mode _mode, 
                                         OMgrClient *omc)
    : test_ip_addr(0), test_sm_port(0), test_dm_port(0),
      mode(_mode), parent_omc(omc) {
  if (parent_omc) {
    parent_omc->registerEventHandlerForNodeEvents(nodeEventHandler);
//    parent_omc->registerEventHandlerForMigrateEvents(nodeEventHandler);
  }
}

StorHvDataPlacement::StorHvDataPlacement(dp_mode _mode,
                                         fds_uint32_t test_ip,
                                         fds_uint32_t test_sm,
                                         fds_uint32_t test_dm,
                                         OMgrClient *omc)
  : StorHvDataPlacement(_mode, omc) {
  test_ip_addr = test_ip;
  test_sm_port = test_sm;
  test_dm_port = test_dm;
}

StorHvDataPlacement::~StorHvDataPlacement() {
}
