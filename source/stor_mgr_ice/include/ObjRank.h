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
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <util/Log.h>
#include "DiskMgr.h"

#include <include/fds_assert.h>
#include <concurrency/RwLock.h>
#include <utility>
#include <atomic>
#include <unordered_map>


#define MAX_RANK_CACHE_SIZE    10485760      /* x 20bytes = 200MB */ 
#define OBJECT_RANK_ALL_SSD    0x00000000    /* highest rank */
#define OBJECT_RANK_ALL_DISK   0xFFFFFFFF    /* lowest rank */

namespace fds {


/* ObjectRankEngine : A class that keeps track of a rank table of all objects that 
 * "Needs" to be in the SSD tier. It is the job of the migrator/placement-tiering engine 
 * to make sure the rank table is reflected in the persistent-Layer
 *
 */ 
class ObjectRankEngine {
 public: 
  ObjectRankEngine(const std::string& _sm_prefix, fds_uint32_t tbl_size, fds_log *log);
   ~ObjectRankEngine();

   typedef enum {
     OBJ_RANK_PROMOTION, // To Flash/SSD
     OBJ_RANK_DEMOTION // To disk
   } rankOperType;

   typedef std::unordered_map<ObjectID, rankOperType, ObjectHash> rank_delta_table_t;

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
   void getRankDeltaChangeTable(rank_delta_table_t* tbl);

   // Persist the Rank Tbl via level-DB for restartability, & restart rank-engine without loosing the prev context
   void  persistRankTbl();
   
   // getNextTblSegment returns the num_objs in the array provided by caller, return false if there are no more to return
   fds_bool_t  getNextTblSegment(fds_uint32_t num_objs, ObjectID *objIdArray);

 private: /* methods */

   /* Object rank (a 32-bit value) is constructed as follows: 
    *  bits 32-17 : object subrank that captures recency and frequency of object accesses
    *  bits 16-1  : volume rank that captures volume policy, consists of 
    *               bits 16-9: policies related to performance 
    *               bits 8-1 : not used yet
    *  Special values:
    *  0x00000000  'all ssd' volume
    *  0xFFFFFFFF  'all disk' volume  
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
     fds_uint16_t frequency = 8; // TODO:get frequency from stats tracker

     /* mask and invert so that the higher ranks have lower values */
     return ( ~(frequency & frequency_mask) );
   }

 private: /* data */

   // ObjectDB* rankDB;

   /* size of the rank table */
   fds_uint32_t rank_tbl_size; 
   /* current number of objects in the rank table, normally will be same as table size */
   fds_uint32_t cur_rank_tbl_size;

   fds_rwlock maps_rwlock;

   /* this is a cache of newly inserted objects (since last ranking process) 
    * that are candidates for promotion (not going to disk or to ssds for sure 
    * because of 'all disk' or 'all ssd" policy */ 
   std::unordered_map<ObjectID, fds_uint32_t, ObjectHash> candidate_map;
   /* max number of objects that we can keep in the candidate map, if we reach
    * this max, we will start ranking process */
   fds_uint32_t max_candidate_size; 

   /* Delta Change table from the last migrator snapshot */
   rank_delta_table_t rankDeltaChgTbl;

   /* True is ranking proces is enabled */
   std::atomic<fds_bool_t> rankingEnabled;

   /* does not own, passed to the constructor */
   fds_log* ranklog;

};

} // namespace fds

#endif
