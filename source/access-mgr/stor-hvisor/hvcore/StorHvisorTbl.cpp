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
#include <NetSession.h>

extern StorHvCtrl *storHvisor;

StorHvDataPlacement::StorHvDataPlacement(dp_mode _mode, 
                                         OMgrClient *omc)
    : test_ip_addr(0), test_sm_port(0), test_dm_port(0),
      mode(_mode), parent_omc(omc) {
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
