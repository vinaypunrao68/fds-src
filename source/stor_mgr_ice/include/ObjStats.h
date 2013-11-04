
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
#include <pthread.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <list>
#include <vector>
#include <Ice/Ice.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>

#include <include/fds_assert.h>
#include <utility>
#include <atomic>
#include <unordered_map>
#include <util/counter.h>
#include <leveldb/db.h>

using namespace std;

namespace fds {

#define  COUNTER_UPDATE_SLOT_TIME    1    /*  second */

class ObjStatsTracker { 

public:
  /*
   * get the start time 
   */
  CounterHist8bit *objStats; 
  /*  per volume lock */
  fds_mutex *objStatsMapLock;

  /*
   * log  derived from parent
   */
   fds_log *stats_log;
 
   /*
    * level db for storing the stats 
    */
   leveldb::DB* db;

  ObjStatsTracker(fds_log *parent_log);
  ~ObjStatsTracker();

   boost::posix_time::ptime startTime; 
   /*
    * io Path class
    */ 
   class ioPathStats {
    public:
   	CounterHist8bit  objStats;
	fds_uint64_t   averageObjectsRead; 
        boost::posix_time::ptime lastAccessTimeR; 
   };
 	
   /*
    *  this for testing only 
   */
   fds_uint64_t   ObjectsReadPerSlot; 

   /*
    * IO path Interface functions 
    */
    Error updateIOpathStats(fds_volid_t vol_uuid,const ObjectID& objId);

   /*
    * Ranking Engine  Interface functions   define proto type  for bulk objects  
    */
   void setAccessSlotTime(fds_uint64_t slotTime);
   void lastObjectReadAccessTime(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   void lastObjectWriteAccessTime(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   fds_uint32_t getObjectAccessVol(fds_volid_t vol_uuid);
   fds_uint32_t getObjectAccess(const ObjectID& objId);
   /*
    * bulk object interface 
    */
   void lastObjectReadAccessTimeBlk(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   void lastObjectWriteAccessTimeBlk(fds_volid_t vol_uuid,ObjectID& objId, fds_uint64_t accessTime);
   fds_uint32_t getObjectAccessBlk(fds_volid_t vol_uuid,ObjectID& objId);

  /*
   * tier   object classification  API's 
   */
   fds_uint32_t hotObjThreshold;
   fds_uint32_t coldObjThreshold;


//   std::list<ObjectID> hotObjList;
//   std::list<ObjectID> coldObjList;

   std::set<ObjectID, ObjectLess> hotObjList;
   std::set<ObjectID, ObjectLess> coldObjList;

   void setHotObjectThreshold(fds_uint32_t hotObjLevel);
   void setColdObjectThreshold(fds_uint32_t coldObjLevel);

   void getHotObjectList(std::set<ObjectID,ObjectLess>& ret_list);
   
  private:

  fds_bool_t objStatsExists(const ObjectID& objId);
  fds_bool_t objExists(const ObjectID& objId);
  /*
   * level db class for   storing the  stats 
   */
   class  levelDbStats {
    public:
      fds_uint64_t      numObjsRead;
      fds_uint64_t      numObjsReadDay;
      fds_uint64_t      numObjsReadWeek;
      fds_uint64_t      numObjsReadMonth;
      boost::posix_time::ptime lastAccessTimeR; 
   };


  Error queryDbStats(const ObjectID &oid,ioPathStats *qryStats);
  Error updateDbStats(const ObjectID &oid,ioPathStats *updStats); 
  Error deleteDbStats(const ObjectID &oid,ioPathStats *delStats); 
   /*
    * map  defination for in memory  stats structure 
    */
  typedef std::unordered_map<ObjectID, ioPathStats*,ObjectHash> ioPathStatsObj_map_t;
  typedef std::unordered_map<fds_volid_t, ioPathStats* > ioPathStatsVol_map_t;

  ioPathStatsObj_map_t  ioPathStatsObj_map;


};

} /* fds namespace */

#endif  // OBJ_STATS_H
