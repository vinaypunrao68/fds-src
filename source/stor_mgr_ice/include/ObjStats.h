
/*
 * Copyright 2013 Formation Data Systems, Inc.
 * Object tracker statistics 
 */

#ifndef OBJ_STATS_H
#define OBJ_STATS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fdsp/FDSP.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include "ObjLoc.h"
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <Ice/Ice.h>
#include <util/Log.h>
#include "DiskMgr.h"
#include "StorMgrVolumes.h"
#include <disk-mgr/dm_service.h>
#include <disk-mgr/dm_io.h>

#include <include/fds_qos.h>
#include <include/qos_ctrl.h>
#include <include/fds_assert.h>
#include <utility>
#include <atomic>
#include <unordered_map>

namespace fds {

class StorMgrStatsTracker : public StorMgrStatsTracker { 
  public:
  StorMgrStatsTracker(fds_log *parent_log);
  ~StorMgrStatsTracker();

   /*
    * io Path class
    */ 
   class ioPathStats {
	fds_uint64_t   numObjectsRead;  
	fds_uint64_t   averageObjectsRead; 
	fds_uint64_t   lastAccessTimeR;
	fds_uint64_t   lastAccessTimeW;
   };

  /* lock, may not required */
  fds_mutex *objStatsMapLock;
   /*
    * map  defination for in memory  stats structure 
    */
  typedef std::unordered_map<ObjectID, ioPathStats* > ioPathStatsObj_map_t;
  typedef std::unordered_map<fds_volid_t, ioPathStats* > ioPathStatsVol_map_t;

   /*
    * IO path Interface functions 
    */
    Error updateIOpathStats(fds_volid_t vol_uuid,const ObjectID& objId);

   /*
    * Ranking Engine  Interface functions   define proto type  for bulk objects  
    */
   setAccessSlotTime(fds_uint64_t slotTime);
   lastObjectReadAccessTime(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   lastObjectWriteAccessTime(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   getObjectAccess(fds_volid_t vol_uuid,ObjectID& objId,fds_uint64_t                                                           numAccess);
   /*
    * bulk object interface 
    */
   lastObjectReadAccessTimeBlk(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   lastObjectWriteAccessTimeBlk(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   getObjectAccessBlk(fds_volid_t vol_uuid,ObjectID& objId,fds_uint64_t numAccess);

   
  private:
    Error queryDbStats(const ObjectID &oid,StorMgrStatsTracker *qryStats);
    Error updateDbStats(const ObjectID &oid,StorMgrStatsTracker *updStats); 
    Error deleteDbStats(const ObjectID &oid,StorMgrStatsTracker *delStats); 
};

#endif  // OBJ_STATS_H
