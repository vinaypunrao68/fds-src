/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef __StorHvisorNet_h__
#define __StorHvisorNet_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>


#include <list>
#include "StorHvDataPlace.h"
#include <fds_error.h>
#include <fds_types.h>

/* TODO: Use API header in include directory. */
#include "StorHvVolumes.h"
#include "fds_qos.h" 
#include <hash/md5.h>
#include <FdsRandom.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <fdsp/FDSP_MetaDataPathReq.h>
#include <fdsp/FDSP_MetaDataPathResp.h>
#include <fdsp/FDSP_ControlPathReq.h>
#include <fdsp/FDSP_ControlPathResp.h>
#include <fdsp/FDSP_ConfigPathReq.h>
#include <net/SvcRequest.h>
#include <am-tx-mgr.h>
#include <AmCache.h>
#include <AmDispatcher.h>
#include <AmProcessor.h>
#include <AmReqHandlers.h>
#include "AmRequest.h"

#include <map>
// #include "util/concurrency/Thread.h"
#include <concurrency/Synchronization.h>
#include <fds_counters.h>
#include "PerfTrace.h"

#undef  FDS_TEST_SH_NOOP              /* IO returns (filled with 0s for read) as soon as SH receives it from ubd */
#undef FDS_TEST_SH_NOOP_DISPATCH     /* IO returns (filled with 0s for read) as soon as dispatcher takes it from the queue */



#define FDS_IO_LONG_TIME 60 // seconds
#define FDS_REPLICATION_FACTOR          1
#define MAX_DM_NODES                    4

#define  FDS_NODE_OFFLINE               0
#define  FDS_NODE_ONLINE                1

#define  FDS_STORAGE_TYPE_SSD           1
#define  FDS_STORAGE_TYPE_FLASH         2
#define  FDS_STORAGE_TYPE_HARD          3
#define  FDS_STORAGE_TYPE_TAPE          4


#define  FDS_NODE_TYPE_PRIM             1
#define  FDS_NODE_TYPE_SEND             2
#define  FDS_NODE_TYPE_BCKP             3


#define  FDS_SUCCESS                    0
#define  FDS_FAILURE                    1

#define  FDS_TIMER_TIMEOUT              1

#define HVISOR_SECTOR_SIZE 		512

typedef unsigned int volid_t;

// Just a couple forward-declarations to satisfy the function
// prototypes below.
namespace fds {
struct CommitBlobTxReq;
struct PutBlobReq;
}

using namespace FDS_ProtocolInterface;
using namespace std;
using namespace fds;


/*
 * Static function for process IO via a threadpool
 */
extern void processBlobReq(AmRequest *amReq);

#endif  // SOURCE_STOR_HVISOR_STORHVISORNET_H_
