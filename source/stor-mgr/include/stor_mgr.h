/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_STOR_MGR_H_
#define SOURCE_STOR_MGR_INCLUDE_STOR_MGR_H_

#include "shared/fds_types.h"

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10

typedef struct {
    fds_int32_t   sockfd;
    fds_uint32_t  num_threads;
} stor_mgr_ctrl_blk_t;

#endif  // SOURCE_STOR_MGR_INCLUDE_STOR_MGR_H_
