#ifndef __STOR_MGR_H__
#define __STOR_MGR_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fds_commons.h>
#include <pthread.h>
#include <fdsp.h>
#include <stor_mgr_err.h>
#include <obj_loc.h>

#define FDS_STOR_MGR_LISTEN_PORT FDS_CLUSTER_TCP_PORT_SM
#define FDS_STOR_MGR_DGRAM_PORT FDS_CLUSTER_UDP_PORT_SM
#define FDS_MAX_WAITING_CONNS  10
#endif
