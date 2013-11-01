/*
 *  * Copyright 2013 Formation Data Systems, Inc.
 *
 */
#ifndef OBJ_RANK_ENGINE_H_
#define OBJ_RANK_ENGINE_H_

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "stor_mgr_err.h"

#include <fdsp/FDSP.h>
#include <hash/MurmurHash3.h>
#include <fds_volume.h>
#include <lib/Catalog.h>
#include <unistd.h>
#include <assert.h>
#include <util/Log.h>
#include "DiskMgr.h"

#include <include/fds_assert.h>
#include <concurrency/Mutex.h>
#include <concurrency/Thread.h>
#include <concurrency/Synchronization.h>
#include <utility>
#include <atomic>
#include <unordered_map>

#include "ObjStats.h"

#define MAX_RANK_CACHE_SIZE    10485760      /* x 20bytes = 200MB */ 
#define OBJECT_RANK_ALL_SSD    0x00000000    /* highest rank */
#define OBJECT_RANK_ALL_DISK   0xFFFFFFF1    /* lowest rank */
#define OBJECT_RANK_INVALID    0xFFFFFFFF    /* invalid id, internally used for deleted objects */
#define RANK_DEMOTION_MASK     0x00000001    /* use this mask to check if rank specifies demotion operation */

namespace fds {


/* ObjectRankEngine : A class that keeps track of a rank table of all objects that 
 * "Needs" to be in the SSD tier. It is the job of the migrator/placement-tiering engine 
 * to make sure the rank table is reflected in the persistent-Layer
 *
 */ 
class ObjectRankEngine {
 public: 
  ObjectRankEngine(const std::string& _sm_prefix, fds_uint32_t tbl_size, ObjStatsTracker *_obj_stats, fds_log *log);
   ~ObjectRankEngine();

   typedef enum {
     OBJ_RANK_PROMOTION, // To Flash/SSD
     OBJ_RANK_DEMOTION // To disk
   } rankOperType;

   typedef std::unordered_map<ObjectID, rankOperType, ObjectHash> rank_delta_table_t;
   typedef std::unordered_map<ObjectID, fds_uint32_t, ObjectHash> obj_rank_cache_t;
   typedef std::unordered_map<ObjectID, fds_uint32_t, ObjectHash>::iterator obj_rank_cache_it_t;
   typedef std::multimap<fds_uint32_t, ObjectID> rank_order_objects_t;
   typedef std::multimap<fds_uint32_t, ObjectID>::iterator rank_order_objects_it_t; 
   typedef std::multimap<fds_uint32_t, ObjectID>::reverse_iterator rank_order_objects_rit_t; 

  // rankAndInsertObject() is called by placement engine on a new objectID
   fds_uint32_t rankAndInsertObject(const ObjectID& objId, const VolumeDesc& voldesc);

  // Delete an objectID from the rank list since it is no longer valid
   void deleteObject(const ObjectID& objId);

   // Obtain the rank assigned to an object
   fds_uint32_t getRank(const ObjectID& objId, const VolumeDesc& voldesc);

   /* check if ranking process is in progress */
   fds_bool_t rankingInProgress();

   // Start the ranking process
   void startObjRanking();

   // Stop ranking process - called for by administrator controlled config, 
   // Admins can provide a scheduled time when ranking proces can run, without interfering with the ingest traffic
   void stopObjRanking();

   // Migrator/Tiering Engine will periodically get the changes to the rankTbl and apply the migrations as needed 
   // consulting the indexTbl
   // Currently: will return 0 if ranking process is in progress
   // The array must be allocated by the caller
   // returns number of actualy objects we put into 'objRankArray'
   fds_uint32_t getDeltaChangeTblSegment(fds_uint32_t num_objs, std::pair<ObjectID, rankOperType>* objRankArray);

   // Migrator/Tiering Engine must call this method when done with RankDeltaChangeTable
   // For now we 'forget' about objects once we return them with getDeltaChangeTblSegment
   //void doneWithRankDeltaChangeTable(void);

   // getNextTblSegment returns the num_objs in the array provided by caller
   // returns actual number of objects that it put to the array
   // if return value < num_objects, means no more objects to return
   // Currently, it will stop providing objects if they are not cached; 
   // also, assumes they get used, so will remove those from cache as soon as they are returned.
   fds_uint32_t getNextTblSegment(fds_uint32_t num_objs, ObjectID *objIdArray);

   // Returns the rank of the last object in the rank table (the lowest-rank)
   // If the rank table has free space, returns OBJECT_RANK_INVALID which is the
   // lowest possible rank;
   // This is the lowest rank as of the last ranking process 
   inline fds_uint32_t getTblTailRank() const { return tail_rank; }

   static void runRankingThread(ObjectRankEngine* self);

 private: /* methods */

   /* Object rank (a 32-bit value) is constructed as follows: 
    *  bits 32-17 : object subrank that captures recency and frequency of object accesses
    *  bits 16-1  : volume rank that captures volume policy, consists of 
    *               bits 16-9: policies related to performance 
    *               bits 8-2 : not used yet
    *               bit 1 : promotion/demotion
    *  Special values:
    *  0x00000000  'all ssd' volume
    *  0xFFFFFFF1  'all disk' volume
    *  0xFFFFFFFF   invalid rank id, used internally for deleted objects
    */

   /* for now a simple rank based on volume's priority and whether volume has min_iops requirement */
   inline fds_uint16_t getVolumeRank(const VolumeDesc& voldesc) const {
     fds_uint8_t bin = (voldesc.iops_min > 0) ? 0 : 1;
     fds_uint16_t rank = (10 * bin + voldesc.relativePrio);
     return (( rank << 8 ) & 0xFF00);
   }

   inline fds_uint16_t getObjectSubrank(const ObjectID& objId) {
     /* we don't need to go exactly with frequency, e.g. if object 
      * was accessed 30 or 31 times recently it should yield the same 
      * rank, so we will reset few least significant bits */ 
     const fds_uint16_t frequency_mask = 0xFFFC; /* last two bits */
     fds_uint16_t frequency = obj_stats->getObjectAccess(objId);

     /* mask and invert so that the higher ranks have lower values */
     return ( ~(frequency & frequency_mask) );
   }

   inline fds_uint32_t updateRank(const ObjectID& objId, fds_uint32_t rank)
   {
     fds_uint32_t new_rank = rank & 0x0000FFFF; /* remove object subrank part */
     if ((rank == OBJECT_RANK_INVALID) || 
	 (rank == OBJECT_RANK_ALL_SSD) ||
	 (rank == OBJECT_RANK_ALL_DISK)) {
       return rank;
     }
     new_rank += ( (getObjectSubrank(objId) << 16) & 0xFFFF0000 );
     return new_rank;
   }

   /* for now will return demotion only if this is 'all disk' volume */
   inline fds_bool_t isRankDemotion(fds_uint32_t rank) const { 
     return (( rank & RANK_DEMOTION_MASK ) != 0x00000000);
   }

   /* methods that actually implement ranking process */
   void runRankingThreadInternal(void);
   Error doRanking(void);

   /* persist change table stored in tmp_chgTbl  and swap to rankDeltaChgTbl*/
   Error persistChgTableAndSwap();

 private: /* data */

   /* persistent rank table */
   Catalog* rankDB;

   /* size of the rank table */
   fds_uint32_t rank_tbl_size; 
   /* current number of objects in the rank table, normally will be same as table size */
   fds_uint32_t cur_rank_tbl_size;


   /* this is a cache of newly inserted/deleted objects (since last ranking process) */
   obj_rank_cache_t cached_obj_map;
   /* max number of objects that we can keep in the cached object map, if we reach
    * this max, we will start ranking process */
   fds_uint32_t max_cached_objects;
   fds_mutex* map_mutex;

   /* Delta Change table from the last migrator snapshot that holds objects ranks (not
    * just promote/demote) since we need to remember some volume info (such as 'all ssd')
    * when we re-compute ranking before migrator gets the delta change table */
   obj_rank_cache_t rankDeltaChgTbl;
   obj_rank_cache_t tmp_chgTbl; /* this is temporary space to work on during ranking process */
   rank_order_objects_t lowrank_objs; /* cache of N lowert rank objects in the rank table */
   fds_mutex* tbl_mutex; /* for both rankDeltaChgTbl and lowrank_objs */

   /* cache lowest rank in the ranking table for queries from the tiering engine 
    * this value is updated only at the end of each ranking process */
   fds_uint32_t tail_rank; /* updated only by the ranking thread */ 

   /* True is ranking proces is enabled */
   std::atomic<fds_bool_t> rankingEnabled;
   /* only used by destructor to make sure we stop ranking thread first */
   fds_bool_t exiting;  
   /* for now one thread doing ranking process */
   boost::thread *rank_thread;
   fds_notification* rank_notify;

   /* does not own, passed to the constructor */
   ObjStatsTracker *obj_stats;

   /* does not own, passed to the constructor */
   fds_log* ranklog;

};

} // namespace fds

#endif
